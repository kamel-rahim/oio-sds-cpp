/** Copyright (c) 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#include <rapidjson/filereadstream.h>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>

#include <fstream>

#include "utils/utils.h"

#include "bin/MillDaemon.h"

namespace blob = oio::api::blob;
using blob::Upload;
using blob::Download;
using blob::Removal;
using blob::Cause;

DEFINE_bool(verbose_daemon, false, "Trace server events");

static int _on_IGNORE(http_parser *p UNUSED) {
    return 0;
}

static int _on_data_IGNORE(http_parser *p UNUSED, const char *b UNUSED,
        size_t l UNUSED) {
    return 0;
}

static int _on_trailer_field_COMMON(http_parser *p, const char *b, size_t l);

static int _on_trailer_value_COMMON(http_parser *p, const char *b, size_t l);

static void _ignore_upload(BlobClient *ctx) {
    ctx->settings.on_body = _on_data_IGNORE;
    ctx->settings.on_chunk_complete = _on_IGNORE;
    ctx->settings.on_chunk_header = _on_IGNORE;
    ctx->settings.on_message_complete = _on_IGNORE;
}

static int _on_headers_complete_UPLOAD(http_parser *p) {
    auto ctx = reinterpret_cast<BlobClient *>(p->data);
    ctx->Reply100();
    ctx->upload = ctx->handler->GetUpload();
    auto rc = ctx->upload->Prepare();
    if (rc.Ok()) {
        ctx->settings.on_header_field = _on_trailer_field_COMMON;
        ctx->settings.on_header_value = _on_trailer_value_COMMON;
        return 0;
    } else {
        switch (rc.Why()) {
            case Cause::Forbidden:
                ctx->ReplyError({403, 403, "Upload forbidden"});
                _ignore_upload(ctx);
                return 1;
            case Cause::Already:
                ctx->ReplyError({406, 421, "blobs found"});
                return 1;
            case Cause::NetworkError:
                ctx->ReplyError({503, 503, "network error to devices"});
                return 1;
            case Cause::ProtocolError:
                ctx->ReplyError({502, 502, "protocol error to devices"});
                return 1;
            default:
                ctx->ReplyError({500, 500, "Internal error"});
                return 1;
        }
    }
    abort();
    return 1;
}

static int _on_body_UPLOAD(http_parser *p, const char *buf, size_t len) {
    auto ctx = reinterpret_cast<BlobClient *>(p->data);

    // We do not manage bodies exceeding 4GB at once
    if (len >= std::numeric_limits<uint32_t>::max()) {
        return 1;
    }

    if (len > 0) {
        if (ctx->checksum_in.get() != nullptr)
            ctx->checksum_in->Update(buf, len);
        ctx->bytes_in += len;
        ctx->upload->Write(reinterpret_cast<const uint8_t *>(buf), len);
    }

    return 0;
}

static int _on_chunk_header_UPLOAD(http_parser *p UNUSED) {
    return 0;
}

static int _on_chunk_complete_UPLOAD(http_parser *p UNUSED) {
    return 0;
}

static int _on_message_complete_UPLOAD(http_parser *p) {
    auto ctx = reinterpret_cast<BlobClient *>(p->data);
    DLOG_IF(INFO, FLAGS_verbose_daemon)
    << __FUNCTION__ << " fd=" << ctx->client->Debug();

    auto rc = ctx->upload->Commit();

    // Trigger a reply to the client
    if (rc.Ok()) {
        rapidjson::StringBuffer buf;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buf);
        writer.StartObject();
        writer.Key("stream");
        writer.StartObject();
        writer.Key("bytes");
        writer.Int64(ctx->bytes_in);
        writer.Key("md5");
        writer.String(ctx->checksum_in->Final().c_str());
        writer.EndObject();
        writer.EndObject();
        ctx->ReplySuccess(201, buf.GetString());
    } else {
        ctx->ReplyError({500, 400, "LocalUpload commit failed"});
    }
    return 0;
}

static int _on_headers_complete_DOWNLOAD(http_parser *p) {
    auto ctx = reinterpret_cast<BlobClient *>(p->data);
    ctx->download = ctx->handler->GetDownload();
    auto rc = ctx->download->Prepare();
    switch (rc.Why()) {
        case Cause::OK:
            ctx->Reply100();
            ctx->ReplyStream();
            return 0;
        case Cause::NotFound:
            ctx->ReplyError({404, 420, "blobs not found"});
            break;
        case Cause::NetworkError:
            ctx->ReplyError({503, 500, "devices unreachable"});
            break;
        case Cause::ProtocolError:
            ctx->ReplyError({502, 500, "invalid reply from device"});
            break;
        default:
            ctx->ReplyError({500, 500, "invalid reply from device"});
            break;
    }
    ctx->settings.on_message_complete = nullptr;
    return 1;
}

static int _on_message_complete_DOWNLOAD(http_parser *p UNUSED) {
    auto ctx = reinterpret_cast<BlobClient *>(p->data);

    while (!ctx->download->IsEof()) {
        std::vector<uint8_t> buf;
        ctx->download->Read(&buf);
        if (buf.size() > 0) {
            std::stringstream ss;
            ss << std::hex << buf.size() << "\r\n";
            auto hdr = ss.str();
            struct iovec iov[] = {
                    BUFLEN_IOV(hdr.data(), hdr.size()),
                    BUFLEN_IOV(buf.data(), buf.size()),
                    BUF_IOV("\r\n")
            };
            ctx->client->send(iov, 3, mill_now() + 1000);
        }
    }

    ctx->ReplyEndOfStream();
    return 0;
}

static int _on_headers_complete_REMOVAL(http_parser *p UNUSED) {
    auto ctx = reinterpret_cast<BlobClient *>(p->data);
    ctx->removal = ctx->handler->GetRemoval();
    auto rc = ctx->removal->Prepare();
    switch (rc.Why()) {
        case Cause::OK:
            ctx->Reply100();
            return 0;
        case Cause::NotFound:
            ctx->ReplyError({404, 402, "no blob found"});
            return 1;
        case Cause::NetworkError:
            ctx->ReplyError({503, 500, "devices unreachable"});
            return 1;
        case Cause::ProtocolError:
            ctx->ReplyError({502, 500, "invalid reply from devices"});
            return 1;
        default:
            ctx->ReplyError({500, 500, "invalid reply from devices"});
            return 1;
    }
}

static int _on_message_complete_REMOVAL(http_parser *p UNUSED) {
    auto ctx = reinterpret_cast<BlobClient *>(p->data);
    assert(ctx->removal.get() != nullptr);

    auto rc = ctx->removal->Commit();
    if (rc.Ok()) {
        ctx->ReplySuccess();
        return 0;
    } else {
        ctx->ReplyError({500, 500, "Removal impossible"});
        return 1;
    }
}

static int _on_message_begin_COMMON(http_parser *p UNUSED) {
    auto ctx = reinterpret_cast<BlobClient *>(p->data);
    DLOG_IF(INFO, FLAGS_verbose_daemon)
    << " REQUEST fd=" << ctx->client->fileno();
    ctx->Reset();
    return 0;
}

int _on_url_COMMON(http_parser *p, const char *buf, size_t len) {
    auto ctx = reinterpret_cast<BlobClient *>(p->data);
    std::string url(buf, len);
    ctx->SaveError(ctx->handler->SetUrl(url));
    return 0;
}

int _on_header_field_COMMON(http_parser *p, const char *buf, size_t len) {
    auto ctx = reinterpret_cast<BlobClient *>(p->data);
    assert(ctx->defered_error.Ok());
    ctx->last_field_name.assign(buf, len);
    return 0;
}

int _on_header_value_COMMON(http_parser *p, const char *buf, size_t len) {
    auto ctx = reinterpret_cast<BlobClient *>(p->data);
    assert(ctx->defered_error.Ok());
    HeaderCommon header;
    header.Parse(ctx->last_field_name);
    if (header.Matched()) {
        if (header.IsCustom()) {
            ctx->handler->SetHeader(ctx->last_field_name,
                                    std::string(buf, len));
        } else {
            DLOG(INFO) << "Header not matched";
        }
    }
    ctx->last_field_name.clear();
    return 0;
}

int _on_trailer_field_COMMON(http_parser *p, const char *buf, size_t len) {
    return _on_header_field_COMMON(p, buf, len);
}

int _on_trailer_value_COMMON(http_parser *p, const char *buf, size_t len) {
    return _on_header_value_COMMON(p, buf, len);
}

int _on_headers_complete_COMMON(http_parser *p) {
    auto ctx = reinterpret_cast<BlobClient *>(p->data);
    DLOG_IF(INFO, FLAGS_verbose_daemon)
    << __FUNCTION__ << " " << ctx->client->Debug();

    if (!ctx->defered_error.Ok()) {
        ctx->ReplyError(ctx->defered_error);
        return 1;
    }

    if (p->method == HTTP_PUT) {
        ctx->settings.on_message_begin = _on_message_begin_COMMON;
        ctx->settings.on_url = _on_url_COMMON;
        ctx->settings.on_status = _on_data_IGNORE;
        ctx->settings.on_header_field = _on_header_field_COMMON;
        ctx->settings.on_header_value = _on_header_value_COMMON;
        ctx->settings.on_headers_complete = _on_headers_complete_COMMON;
        ctx->settings.on_body = _on_body_UPLOAD;
        ctx->settings.on_message_complete = _on_message_complete_UPLOAD;
        ctx->settings.on_chunk_header = _on_chunk_header_UPLOAD;
        ctx->settings.on_chunk_complete = _on_chunk_complete_UPLOAD;
        return _on_headers_complete_UPLOAD(p);
    } else if (p->method == HTTP_GET) {
        ctx->settings.on_message_begin = _on_message_begin_COMMON;
        ctx->settings.on_url = _on_url_COMMON;
        ctx->settings.on_status = _on_data_IGNORE;
        ctx->settings.on_header_field = _on_header_field_COMMON;
        ctx->settings.on_header_value = _on_header_value_COMMON;
        ctx->settings.on_headers_complete = _on_headers_complete_COMMON;
        ctx->settings.on_body = _on_data_IGNORE;
        ctx->settings.on_message_complete = _on_message_complete_DOWNLOAD;
        ctx->settings.on_chunk_header = _on_IGNORE;
        ctx->settings.on_chunk_complete = _on_IGNORE;
        return _on_headers_complete_DOWNLOAD(p);
    } else if (p->method == HTTP_DELETE) {
        ctx->settings.on_message_begin = _on_message_begin_COMMON;
        ctx->settings.on_url = _on_url_COMMON;
        ctx->settings.on_status = _on_data_IGNORE;
        ctx->settings.on_header_field = _on_header_field_COMMON;
        ctx->settings.on_header_value = _on_header_value_COMMON;
        ctx->settings.on_headers_complete = _on_headers_complete_REMOVAL;
        ctx->settings.on_body = _on_data_IGNORE;
        ctx->settings.on_message_complete = _on_message_complete_REMOVAL;
        ctx->settings.on_chunk_header = _on_IGNORE;
        ctx->settings.on_chunk_complete = _on_IGNORE;
        return _on_headers_complete_REMOVAL(p);
    } else {
        ctx->ReplyError({406, 406, "Method not managed"});
        return 1;
    }
}

/* -------------------------------------------------------------------------- */

BlobService::BlobService(std::shared_ptr<BlobRepository> r)
        : front(), repository{r} {
    done = chmake(uint32_t, 1);
}

BlobService::~BlobService() {
    front.close();
    chclose(done);
    done = nullptr;
}

void BlobService::Start(volatile bool *running) {
    mill_go(Run(running));
}

void BlobService::Join() {
    uint32_t s = chr(done, uint32_t);
    (void) s;
}

bool BlobService::Configure(const std::string &cfg) {
    rapidjson::Document doc;
    if (doc.Parse<0>(cfg.c_str()).HasParseError()) {
        LOG(ERROR) << "Invalid JSON";
        return false;
    }

    if (!doc.IsObject()) {
        LOG(ERROR) << "Not a JSON object";
        return false;
    }
    if (!doc.HasMember("bind")) {
        LOG(ERROR) << "Missing 'bind' field";
        return false;
    }
    if (!doc["bind"].IsString()) {
        LOG(ERROR) << "Unexpected 'bind' field: not a string";
        return false;
    }

    const auto url = doc["bind"].GetString();
    if (!front.bind(url)) {
        LOG(ERROR) << "Bind(" << url << ") error";
        return false;
    }
    if (!front.listen(8192)) {
        LOG(ERROR) << "Listen(" << url << ") error";
        return false;
    }

    LOG(INFO) << "Bind(" << url << ") done";
    return true;
}

NOINLINE void BlobService::Run(volatile bool *flag_running) {
    assert(flag_running != nullptr);
    bool input_ready = true;
    while (flag_running) {
        net::MillSocket client;
        if (input_ready) {
            auto cli = front.accept();
            if (cli->fileno() >= 0) {
                StartClient(flag_running, std::move(cli));
                continue;
            }
        }
        input_ready = false;
        auto events = front.PollIn(mill_now() + 1000);
        if (events & MILLSOCKET_ERROR) {
            DLOG(INFO) << "front.poll() error";
            *flag_running = false;
        } else {
            input_ready = 0 != (events & MILLSOCKET_EVENT);
        }
    }
    chs(done, uint32_t, 0);
}

void BlobService::StartClient(volatile bool *flag_running,
        std::unique_ptr<net::Socket> s0) {
    mill_go(RunClient(flag_running, std::move(s0)));
}

NOINLINE void BlobService::RunClient(volatile bool *flag_running,
        std::unique_ptr<net::Socket> s0) {
    assert(flag_running != nullptr);
    BlobClient client(std::move(s0), repository);
    return client.Run(flag_running);
}

BlobDaemon::BlobDaemon(std::shared_ptr<BlobRepository> rp)
        : services(), repository_prototype{rp} {
}

BlobDaemon::~BlobDaemon() {
}

void BlobDaemon::Start(volatile bool *flag_running) {
    assert(flag_running != nullptr);
    DLOG(INFO) << __FUNCTION__;
    for (auto srv : services)
        srv->Start(flag_running);
}

void BlobDaemon::Join() {
    DLOG(INFO) << __FUNCTION__;
    for (auto srv : services)
        srv->Join();
}

bool BlobDaemon::LoadJsonFile(const std::string &path) {
    std::stringstream ss;
    std::ifstream ifs;
    ifs.open(path, std::ios::binary);
    ss << ifs.rdbuf();
    ifs.close();

    return LoadJson(ss.str());
}

bool BlobDaemon::LoadJson(const std::string &cfg) {
    const char *key_srv = "service";
    const char *key_repo = "repository";
    rapidjson::StringBuffer buf;
    rapidjson::Document doc;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buf);

    if (doc.Parse<0>(cfg.c_str()).HasParseError()) {
        LOG(ERROR) << "Invalid JSON";
        return false;
    }
    if (!doc.HasMember(key_repo)) {
        LOG(INFO) << "Missing [" << key_repo << "] field";
        return false;
    }
    if (!doc.HasMember(key_srv)) {
        LOG(INFO) << "Missing [" << key_srv << "] field";
        return false;
    }

    // Make and configure the repository
    std::shared_ptr<BlobRepository> repo(repository_prototype->Clone());
    buf.Clear();
    writer.Reset(buf);
    doc[key_repo].Accept(writer);
    if (!repo->Configure(buf.GetString())) {
        LOG(ERROR) << "Invalid repository configuration";
        return false;
    }

    // Make a Service and tie it to the repository
    std::shared_ptr<BlobService> srv(new BlobService(repo));
    buf.Clear();
    writer.Reset(buf);
    doc[key_srv].Accept(writer);
    if (!srv->Configure(buf.GetString()))
        return false;

    services.push_back(srv);
    return true;
}

BlobClient::~BlobClient() {
}

BlobClient::BlobClient(std::unique_ptr<net::Socket> c,
        std::shared_ptr<BlobRepository> r)
        : client(std::move(c)), handler{nullptr},
          expect_100{false}, want_closure{false},
          bytes_in{0}, checksum_in{nullptr} {
    checksum_in.reset(checksum_make_MD5());
    handler.reset(r->Handler());
}

void BlobClient::Run(volatile bool *flag_running) {
    assert(flag_running != nullptr);
    DLOG_IF(INFO, FLAGS_verbose_daemon) << "CLIENT fd=" << client->fileno();

    http_parser_init(&parser, HTTP_REQUEST);
    parser.data = this;
    Reset();

    std::vector<uint8_t> buffer(32768);

    while (*flag_running) {
        errno = EAGAIN;
        ssize_t sr = client->read(buffer.data(), buffer.size(),
                                  mill_now() + 1000);

        if (sr == -2) {
            DLOG_IF(INFO, FLAGS_verbose_daemon) << "CLIENT "
                                                << "fd=" << client->fileno()
                                                << " peer closed";
            break;
        } else if (sr == -1) {
            if (errno != EAGAIN) {
                DLOG_IF(INFO, FLAGS_verbose_daemon) << "CLIENT "
                                                    << "fd=" << client->fileno()
                                                    << " error: (" << errno
                                                    << ") " << strerror(errno);
                break;
            }
        } else if (sr == 0) {
            // TODO(jfs) manage the data timeout
        } else {
            for (ssize_t done = 0; done < sr;) {
                auto buf = reinterpret_cast<const char *>(buffer.data() + done);
                size_t consumed = http_parser_execute(&parser, &settings, buf,
                                                      sr - done);
                if (parser.http_errno != 0) {
                    DLOG(INFO) << "HTTP parsing error " << parser.http_errno
                               << "/" << http_errno_name(
                            static_cast<http_errno>(parser.http_errno))
                               << "/" << http_errno_description(
                            static_cast<http_errno>(parser.http_errno));
                    goto out;
                }
                if (consumed > 0)
                    done += consumed;
            }
        }
    }
out:
    DLOG_IF(INFO, FLAGS_verbose_daemon)
    << "CLIENT " << client->fileno() << " done";
    DLOG_IF(INFO, FLAGS_verbose_daemon)
    << "CLIENT " << client->fileno() << " done";
    client->close();
}

void BlobClient::Reset() {
    settings.on_message_begin = _on_message_begin_COMMON;
    settings.on_url = _on_url_COMMON;
    settings.on_status = _on_data_IGNORE;
    settings.on_header_field = _on_header_field_COMMON;
    settings.on_header_value = _on_header_value_COMMON;
    settings.on_headers_complete = _on_headers_complete_COMMON;
    settings.on_body = _on_data_IGNORE;
    settings.on_message_complete = _on_IGNORE;
    settings.on_chunk_header = _on_IGNORE;
    settings.on_chunk_complete = _on_IGNORE;
    expect_100 = false;
    last_field_name.clear();
    defered_error.Reset();
    upload.reset(nullptr);
    download.reset(nullptr);
    removal.reset(nullptr);
}

void BlobClient::SaveError(SoftError err) {
    defered_error = err;
    if (!defered_error.Ok()) {
        settings.on_header_field = _on_data_IGNORE;
        settings.on_header_value = _on_data_IGNORE;
        settings.on_body = _on_data_IGNORE;
    }
}

void BlobClient::ReplyPreamble(int code, const char *msg, int64_t l) {
    char first[64], length[64];

    snprintf(first, sizeof(first), "HTTP/%1hu.%1hu %03d %s\r\n",
             parser.http_major, parser.http_minor, code, msg);
    if (l >= 0)
        snprintf(length, sizeof(length), "Content-Length: %lu\r\n", l);
    else
        snprintf(length, sizeof(length), "Transfer-Encoding: chunked\r\n");

    std::vector<struct iovec> iotab;
    iotab.resize(6 + 4 * reply_headers.size());
    struct iovec *iov = iotab.data();
    int i = 0;
    iov[i++] = STR_IOV(first);
    iov[i++] = BUF_IOV("Host: nowhere\r\n");
    iov[i++] = BUF_IOV("User-Agent: OpenIO/oio-kinetic-proxy\r\n");
    for (auto &e : reply_headers) {
        iov[i++] = BUFLEN_IOV(e.first.data(), e.first.size());
        iov[i++] = BUF_IOV(": ");
        iov[i++] = BUFLEN_IOV(e.second.data(), e.second.size());
        iov[i++] = BUF_IOV("\r\n");
    }
    iov[i++] = BUF_IOV("Connection: close\r\n");
    iov[i++] = STR_IOV(length);
    iov[i++] = BUF_IOV("\r\n");

    client->send(iotab.data(), iotab.size(), mill_now() + 1000);
}

void BlobClient::ReplyError(SoftError err) {
    std::string payload;
    err.Pack(&payload);
    ReplyPreamble(err.http, "Error", payload.size());
    client->send(payload.data(), payload.size(), mill_now() + 1000);
}

void BlobClient::ReplySuccess() {
    return ReplyPreamble(200, "OK", 0);
}

void BlobClient::ReplySuccess(int code, const std::string &payload) {
    ReplyPreamble(code, "OK", payload.size());
    client->send(payload.data(), payload.size(), mill_now() + 1000);
}

void BlobClient::ReplyStream() {
    return ReplyPreamble(200, "OK", -1);
}

void BlobClient::ReplyEndOfStream() {
    static const char *tail = "0\r\n\r\n";
    client->send(tail, 5, mill_now() + 1000);
}

void BlobClient::Reply100() {
    if (!expect_100)
        return;
    expect_100 = false;

    return ReplyPreamble(100, "Continue", 0);
}

void SoftError::Pack(std::string *dst) {
    assert(dst != nullptr);
    rapidjson::StringBuffer buf;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buf);
    writer.StartObject();
    writer.Key("status");
    writer.Int(soft);
    writer.Key("message");
    writer.String(why);
    writer.EndObject();
    dst->assign(buf.GetString(), buf.GetSize());
}
