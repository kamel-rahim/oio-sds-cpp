/** Copyright (c) 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#include <cstdint>
#include <fstream>

#include <rapidjson/filereadstream.h>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <glog/logging.h>

#include <utils/utils.h>
#include "MillDaemon.h"

namespace blob = oio::api::blob;
using blob::Upload;
using blob::Download;
using blob::Removal;

static const std::string kinetic_url_prefix("k/");

static int _on_IGNORE(http_parser *p UNUSED) {
    return 0;
}

static int _on_data_IGNORE(http_parser *p UNUSED, const char *b UNUSED,
        size_t l UNUSED) {
    return 0;
}

static int _on_trailer_field_COMMON(http_parser *p, const char *b, size_t l);

static int _on_trailer_value_COMMON(http_parser *p, const char *b, size_t l);

static int _on_headers_complete_UPLOAD(http_parser *p) {
    auto ctx = (BlobClient *) p->data;
    ctx->Reply100();
    ctx->settings.on_header_field = _on_trailer_field_COMMON;
    ctx->settings.on_header_value = _on_trailer_value_COMMON;
    ctx->upload = ctx->repository->GetUpload(*ctx);
    auto rc = ctx->upload->Prepare();
    switch (rc) {
        case Upload::Status::OK:
            for (const auto &e: ctx->xattrs)
                ctx->upload->SetXattr(e.first, e.second);
            return 0;
        case Upload::Status::Already:
            ctx->ReplyError({406, 421, "blobs found"});
            return 1;
        case Upload::Status::NetworkError:
            ctx->ReplyError({503, 503, "network error to devices"});
            return 1;
        case Upload::Status::ProtocolError:
            ctx->ReplyError({502, 502, "protocol error to devices"});
            return 1;
        case Upload::Status::InternalError:
            ctx->ReplyError({500, 500, "Internal error"});
            return 1;
    }

    abort();
    return 1;
}

static int _on_body_UPLOAD(http_parser *p, const char *buf, size_t len) {
    auto ctx = (BlobClient *) p->data;
    DLOG(INFO) << __FUNCTION__ << " fd=" << ctx->client->fileno();

    // We do not manage bodies exceeding 4GB at once
    if (len >= std::numeric_limits<uint32_t>::max()) {
        return 1;
    }

    ctx->upload->Write(reinterpret_cast<const uint8_t *>(buf), len);
    return 0;
}

static int _on_chunk_header_UPLOAD(http_parser *p UNUSED) {
    DLOG(INFO) << __FUNCTION__ << "len=" << p->content_length << " r=" <<
    p->nread;
    return 0;
}

static int _on_chunk_complete_UPLOAD(http_parser *p UNUSED) {
    DLOG(INFO) << __FUNCTION__;
    return 0;
}

static int _on_message_complete_UPLOAD(http_parser *p) {
    auto ctx = (BlobClient *) p->data;
    DLOG(INFO) << __FUNCTION__ << " fd=" << ctx->client->fileno();

    auto upload_rc = ctx->upload->Commit();

    rapidjson::StringBuffer buf;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buf);
    writer.StartObject();
    writer.Key("chunks");
    writer.StartArray();
    writer.EndArray();
    writer.EndObject();

    // Trigger a reply to the client
    if (upload_rc) {
        ctx->ReplySuccess(201, buf.GetString());
    } else {
        ctx->ReplyError({500, 400, "LocalUpload commit failed"});
    }
    return 0;
}

static int _on_headers_complete_DOWNLOAD(http_parser *p) {
    auto ctx = (BlobClient *) p->data;
    ctx->download = ctx->repository->GetDownload(*ctx);
    auto rc = ctx->download->Prepare();
    switch (rc) {
        case Download::Status::OK:
            ctx->Reply100();
            ctx->ReplyStream();
            return 0;
        case Download::Status::NotFound:
            ctx->ReplyError({404, 420, "blobs not found"});
            break;
        case Download::Status::NetworkError:
            ctx->ReplyError({503, 500, "devices unreachable"});
            break;
        case Download::Status::ProtocolError:
            ctx->ReplyError({502, 500, "invalid reply from device"});
            break;
        case Download::Status::InternalError:
            ctx->ReplyError({500, 500, "invalid reply from device"});
            break;
    }
    ctx->settings.on_message_complete = nullptr;
    return 1;
}

static int _on_message_complete_DOWNLOAD(http_parser *p UNUSED) {
    auto ctx = (BlobClient *) p->data;

    while (!ctx->download->IsEof()) {
        std::vector<uint8_t> buf;
        ctx->download->Read(buf);
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
    auto ctx = (BlobClient *) p->data;
    ctx->removal = ctx->repository->GetRemoval(*ctx);
    auto rc = ctx->removal->Prepare();
    switch (rc) {
        case Removal::Status::OK:
            ctx->Reply100();
            return 0;
        case Removal::Status::NotFound:
            ctx->ReplyError({404, 402, "no blob found"});
            return 1;
        case Removal::Status::NetworkError:
            ctx->ReplyError({503, 500, "devices unreachable"});
            return 1;
        case Removal::Status::ProtocolError:
            ctx->ReplyError({502, 500, "invalid reply from devices"});
            return 1;
        case Removal::Status::InternalError:
            ctx->ReplyError({500, 500, "invalid reply from devices"});
            return 1;
    }
    abort();
    return 1;
}

static int _on_message_complete_REMOVAL(http_parser *p UNUSED) {
    auto ctx = (BlobClient *) p->data;
    assert(ctx->removal.get() != nullptr);

    auto rc = ctx->removal->Commit();
    if (rc) {
        ctx->ReplySuccess();
        return 0;
    } else {
        ctx->ReplyError({500, 500, "Removal impossible"});
        return 1;
    }
}

static int _on_message_begin_COMMON(http_parser *p UNUSED) {
    auto ctx = (BlobClient *) p->data;
    DLOG(INFO) << " REQUEST fd=" << ctx->client->fileno();
    ctx->Reset();
    return 0;
}

static bool _http_url_has(const http_parser_url &u, int field) {
    assert(field < UF_MAX);
    return 0 != (u.field_set & (1 << field));
}

static std::string _http_url_field(const http_parser_url &u, int f,
        const char *buf, size_t len) {
    if (!_http_url_has(u, f))
        return std::string("");
    assert(u.field_data[f].off <= len);
    assert(u.field_data[f].len <= len);
    assert(u.field_data[f].off + u.field_data[f].len <= len);
    return std::string(buf + u.field_data[f].off, u.field_data[f].len);
}

int _on_url_COMMON(http_parser *p, const char *buf, size_t len) {
    auto ctx = (BlobClient *) p->data;

    // TODO move this to the BlobClient, this is implementation dependant

    // Get the name, this is common to al the requests
    http_parser_url url;
    if (0 != http_parser_parse_url(buf, len, false, &url)) {
        ctx->SaveError({400, 400, "URL parse error"});
        return 0;
    }
    if (!_http_url_has(url, UF_PATH)) {
        ctx->SaveError({400, 400, "URL has no path"});
        return 0;
    }

    // Get the chunk-id, the last part of the URL path
    auto path = _http_url_field(url, UF_PATH, buf, len);
    auto sep = path.rfind('/');
    if (sep == std::string::npos || sep == path.size() - 1) {
        ctx->SaveError({400, 400, "URL has no/empty basename"});
        return 0;
    }

    ctx->chunk_id.assign(path, sep + 1, std::string::npos);
    return 0;
}

int _on_header_field_COMMON(http_parser *p, const char *buf, size_t len) {
    auto ctx = (BlobClient *) p->data;
    assert(ctx->defered_error.http == 0);
    ctx->last_field = header_parse(buf, len);
    if (ctx->last_field == HDR_OIO_XATTR) {
        if (p->method == HTTP_PUT) {
            const char *_b = buf + sizeof(OIO_HEADER_XATTR_PREFIX) - 1;
            size_t _l = len - sizeof(OIO_HEADER_XATTR_PREFIX) - 1;
            ctx->last_field_name.assign(_b, _l);
        }
    }
    return 0;
}

int _on_header_value_COMMON(http_parser *p, const char *buf, size_t len) {
    auto ctx = (BlobClient *) p->data;
    assert(ctx->defered_error.http == 0);

    if (ctx->last_field == HDR_OIO_TARGET) {
        // TODO move this to the BlobClient, this is implementation dependant
        net::Addr addr;
        if (!addr.parse(std::string(buf, len))) {
            ctx->SaveError({400, 400, "Invalid Kinetic target"});
        } else {
            ctx->targets.emplace_back(buf, len);
        }
    }
    else if (ctx->last_field == HDR_OIO_XATTR) {
        if (p->method == HTTP_PUT) {
            ctx->xattrs[ctx->last_field_name] = std::move(
                    std::string(buf, len));
        }
    }
    else if (ctx->last_field == HDR_EXPECT) {
        ctx->expect_100 = ("100-continue" == std::string(buf, len));
    }
    return 0;
}

int _on_trailer_field_COMMON(http_parser *p, const char *buf, size_t len) {
    return _on_header_field_COMMON(p, buf, len);
}

int _on_trailer_value_COMMON(http_parser *p, const char *buf, size_t len) {
    return _on_header_value_COMMON(p, buf, len);
}

int _on_headers_complete_COMMON(http_parser *p) {
    auto ctx = (BlobClient *) p->data;

    if (ctx->defered_error.http > 0) {
        DLOG(INFO) << __FUNCTION__ << " resuming a defered error";
        ctx->ReplyError(ctx->defered_error);
        return 1;
    }

    if (ctx->targets.size() < 1) {
        DLOG(INFO) << "No target specified";
        ctx->ReplyError({400, 400, "No target specified"});
        return 1;
    }

    DLOG(INFO) << __FUNCTION__ << " fd=" << ctx->client->fileno();
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
}

void BlobService::Start(volatile bool &flag_running) {
    mill_go(Run(flag_running));
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

NOINLINE void BlobService::Run(volatile bool &flag_running) {
    bool input_ready = true;
    while (flag_running) {
        net::MillSocket client;
        if (input_ready) {
            auto cli = front.accept();
            if (cli->fileno() >= 0) {
                mill_go(RunClient(flag_running, std::move(cli)));
                continue;
            }
        }
        input_ready = false;
        auto events = front.PollIn(mill_now() + 1000);
        if (events & MILLSOCKET_ERROR) {
            DLOG(INFO) << "front.poll() error";
            flag_running = false;
        } else {
            input_ready = 0 != (events & MILLSOCKET_EVENT);
        }
    }
    chs(done, uint32_t, 0);
}

NOINLINE void BlobService::RunClient(volatile bool &flag_running,
        std::unique_ptr<net::Socket> s0) {
    BlobClient client(std::move(s0), repository);
    client.Run(flag_running);
}

BlobDaemon::BlobDaemon(std::shared_ptr<BlobRepository> rp)
        : services(), repository_prototype{rp} {
}

BlobDaemon::~BlobDaemon() {
}

void BlobDaemon::Start(volatile bool &flag_running) {
    DLOG(INFO) << __FUNCTION__;
    for (auto srv: services)
        srv->Start(flag_running);
}

void BlobDaemon::Join() {
    DLOG(INFO) << __FUNCTION__;
    for (auto srv: services)
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
        : client(std::move(c)), repository{r},
          expect_100{false}, want_closure{false} {
}

void BlobClient::Run(volatile bool &flag_running) {

    LOG(INFO) << "CLIENT fd=" << client->fileno();

    http_parser_init(&parser, HTTP_REQUEST);
    parser.data = this;
    Reset();

    std::vector<uint8_t> buffer(8192);

    while (flag_running) {

        errno = EAGAIN;
        ssize_t sr = client->read(buffer.data(), buffer.size(),
                                 mill_now() + 1000);

        if (sr == -2) {
            DLOG(INFO) << "CLIENT peer closed";
            break;
        } else if (sr == -1) {
            if (errno != EAGAIN) {
                DLOG(INFO) << "CLIENT error: (" << errno << ") " <<
                strerror(errno);
                break;
            }
        } else if (sr == 0) {
            // TODO manage the data timeout
        } else {
            for (ssize_t done = 0; done < sr;) {
                size_t consumed = http_parser_execute(&parser, &settings,
                                                      reinterpret_cast<const char *>(
                                                              buffer.data() +
                                                              done),
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
    DLOG(INFO) << "CLIENT " << client->fileno() << " done";
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
    last_field = HDR_none_matched;
    chunk_id.clear();
    targets.clear();
    defered_error.Reset();
    upload.reset(nullptr);
    download.reset(nullptr);
}

void BlobClient::SaveError(SoftError err) {
    defered_error = err;
    settings.on_header_field = _on_data_IGNORE;
    settings.on_header_value = _on_data_IGNORE;
    settings.on_body = _on_data_IGNORE;
}

void BlobClient::ReplyError(SoftError err) {
    char first[64], length[64];
    std::string payload;

    err.Pack(payload);
    snprintf(first, sizeof(first), "HTTP/%1hu.%1hu %03d Error\r\n",
             parser.http_major, parser.http_minor, err.http);
    snprintf(length, sizeof(length), "Content-Length: %lu\r\n",
             payload.size());

    struct iovec iov[] = {
            STR_IOV(first),
            STR_IOV(length),
            BUF_IOV("Connection: close\r\n"),
            BUF_IOV("\r\n"),
            BUFLEN_IOV(payload.data(), payload.size())
    };
    client->send(iov, 5, mill_now() + 1000);
}

void BlobClient::ReplySuccess() {
    char first[] = "HTTP/1.0 200 OK\r\n";
    first[5] = '0' + parser.http_major;
    first[7] = '0' + parser.http_minor;
    struct iovec iov[] = {
            STR_IOV(first),
            BUF_IOV("Connection: close\r\n"),
            BUF_IOV("Content-Length: 0\r\n"),
            BUF_IOV("\r\n"),
    };
    client->send(iov, 4, mill_now() + 1000);
}

void BlobClient::ReplySuccess(int code, const std::string &payload) {
    char first[64], length[64];

    snprintf(first, sizeof(first), "HTTP/%1hu.%1hu %03d OK\r\n",
             parser.http_major, parser.http_minor, code);
    snprintf(length, sizeof(length), "Content-Length: %lu\r\n",
             payload.size());

    struct iovec iov[] = {
            STR_IOV(first),
            STR_IOV(length),
            BUF_IOV("Connection: close\r\n"),
            BUF_IOV("\r\n"),
            BUFLEN_IOV(payload.data(), payload.size())
    };
    client->send(iov, 5, mill_now() + 1000);
}

void BlobClient::ReplyStream() {
    char first[] = "HTTP/1.0 200 OK\r\n";
    first[5] = '0' + parser.http_major;
    first[7] = '0' + parser.http_minor;
    struct iovec iov[] = {
            STR_IOV(first),
            BUF_IOV("Connection: close\r\n"),
            BUF_IOV("Transfer-Encoding: chunked\r\n"),
            BUF_IOV("\r\n"),
    };
    client->send(iov, 4, mill_now() + 1000);
}

void BlobClient::ReplyEndOfStream() {
    struct iovec iov = BUF_IOV("0\r\n\r\n");
    client->send(&iov, 1, mill_now() + 1000);
}

void BlobClient::Reply100() {

    if (!expect_100)
        return;
    expect_100 = false;

    char first[] = "HTTP/X.X 100 Continue\r\n";
    struct iovec iov[] = {
            BUF_IOV(first),
            BUF_IOV("Content-Length: 0\r\n"),
            BUF_IOV("\r\n"),
    };
    first[5] = '0' + parser.http_major;
    first[7] = '0' + parser.http_minor;
    client->send(iov, 3, mill_now() + 1000);
}

void SoftError::Pack(std::string &dst) {
    rapidjson::StringBuffer buf;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buf);
    writer.StartObject();
    writer.Key("status");
    writer.Int(soft);
    writer.Key("message");
    writer.String(why);
    writer.EndObject();
    dst.assign(buf.GetString(), buf.GetSize());
}
