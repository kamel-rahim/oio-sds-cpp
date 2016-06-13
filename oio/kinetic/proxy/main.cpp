/** Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#include <cstdlib>
#include <functional>
#include <algorithm>
#include <sstream>
#include <fstream>
#include <iomanip>

#include <netinet/in.h>
#include <signal.h>

#include <glog/logging.h>
#include <libmill.h>
#include <rapidjson/filereadstream.h>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>

#include <http-parser/http_parser.h>
#include <utils/utils.h>
#include <utils/MillSocket.h>
#include <oio/api/Upload.h>
#include <oio/api/Download.h>
#include <oio/api/Removal.h>
#include <oio/kinetic/client/CoroutineClientFactory.h>
#include <oio/kinetic/blob/Upload.h>
#include <oio/kinetic/blob/Download.h>
#include <oio/kinetic/blob/Removal.h>

#include "headers.h"

using oio::blob::Upload;
using oio::blob::Download;
using oio::blob::Removal;

using oio::kinetic::blob::RemovalBuilder;
using oio::kinetic::blob::DownloadBuilder;
using oio::kinetic::blob::UploadBuilder;
using oio::kinetic::client::ClientFactory;
using oio::kinetic::client::CoroutineClientFactory;

static volatile unsigned int flag_running = 1;

static std::vector<MillSocket> SRV;
static std::shared_ptr<ClientFactory> factory;

/* ------------------------------------------------------------------------- */

struct RequestContext;
struct CnxContext;

static int _on_IGNORE(http_parser *p UNUSED) { return 0; }

static int _on_data_IGNORE(http_parser *p UNUSED, const char *b UNUSED,
                           size_t l UNUSED) { return 0; }

static int _on_headers_complete_REMOVAL(http_parser *p);

static int _on_message_complete_REMOVAL(http_parser *p);

static int _on_headers_complete_DOWNLOAD(http_parser *p);

static int _on_message_complete_DOWNLOAD(http_parser *p);

static int _on_message_complete_UPLOAD(http_parser *p);

static int _on_body_UPLOAD(http_parser *p, const char *b, size_t l);

static int _on_chunk_header_UPLOAD(http_parser *p);

static int _on_chunk_complete_UPLOAD(http_parser *p);

static int _on_headers_complete_UPLOAD(http_parser *p);

static int _on_message_begin_COMMON(http_parser *p);

static int _on_url_COMMON(http_parser *p, const char *b, size_t l);

static int _on_header_field_COMMON(http_parser *p, const char *b, size_t l);

static int _on_header_value_COMMON(http_parser *p, const char *b, size_t l);

static int _on_trailer_field_COMMON(http_parser *p, const char *b, size_t l);

static int _on_trailer_value_COMMON(http_parser *p, const char *b, size_t l);

static int _on_headers_complete_COMMON(http_parser *p);

static const struct http_parser_settings default_settings{
        _on_message_begin_COMMON,
        _on_url_COMMON,
        _on_data_IGNORE,
        _on_header_field_COMMON,
        _on_header_value_COMMON,
        _on_headers_complete_COMMON,
        _on_data_IGNORE,
        _on_IGNORE,
        _on_IGNORE,
        _on_IGNORE
};

static const struct http_parser_settings upload_settings{
        _on_message_begin_COMMON,
        _on_url_COMMON,
        _on_data_IGNORE,
        _on_header_field_COMMON,
        _on_header_value_COMMON,
        _on_headers_complete_COMMON,
        _on_body_UPLOAD,
        _on_message_complete_UPLOAD,
        _on_chunk_header_UPLOAD,
        _on_chunk_complete_UPLOAD
};

static const struct http_parser_settings download_settings{
        _on_message_begin_COMMON,
        _on_url_COMMON,
        _on_data_IGNORE,
        _on_header_field_COMMON,
        _on_header_value_COMMON,
        _on_headers_complete_COMMON,
        _on_data_IGNORE,
        _on_message_complete_DOWNLOAD,
        _on_IGNORE,
        _on_IGNORE
};

static const struct http_parser_settings removal_settings{
        _on_message_begin_COMMON,
        _on_url_COMMON,
        _on_data_IGNORE,
        _on_header_field_COMMON,
        _on_header_value_COMMON,
        _on_headers_complete_REMOVAL,
        _on_data_IGNORE,
        _on_message_complete_REMOVAL,
        _on_IGNORE,
        _on_IGNORE
};

struct SoftError {
    int http;
    int soft;
    const char *why;

    SoftError() noexcept: http{0}, soft{0}, why{nullptr} { }

    SoftError(int http, int soft, const char *why) noexcept:
            http(http), soft(soft), why(why) { }

    void reset() noexcept {
        http = 0;
        soft = 0, why = nullptr;
    }
};

static void pack_error(std::string &dst, int softcode, const char *why) {
    rapidjson::StringBuffer buf;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buf);
    writer.StartObject();
    writer.Key("status");
    writer.Int(softcode);
    writer.Key("message");
    writer.String(why);
    writer.EndObject();
    dst.assign(buf.GetString(), buf.GetSize());
}

struct CnxContext {
    MillSocket *cnx;
    struct http_parser *parser;
    struct http_parser_settings settings;

    // Related to the current request
    std::string chunk_id;
    std::vector<std::string> targets;

    SoftError defered_error;
    std::unique_ptr<Upload> upload;
    std::unique_ptr<Download> download;
    std::unique_ptr<Removal> removal;

    enum http_header_e last_field;
    std::string last_field_name;
    std::map<std::string,std::string> xattrs;
    bool expect_100;

    ~CnxContext() noexcept { }

    CnxContext(MillSocket *c, http_parser *p) noexcept:
            cnx{c}, parser{p}, settings(), chunk_id(), targets(),
            upload{nullptr}, download{nullptr},
            last_field{HDR_none_matched}, last_field_name(),
            xattrs(), expect_100{false} { }

    CnxContext(CnxContext &&o) noexcept = delete;

    CnxContext(const CnxContext &o) noexcept = delete;

    void reset() noexcept {
        settings = default_settings;
        expect_100 = false;
        last_field = HDR_none_matched;
        chunk_id.clear();
        targets.clear();
        defered_error.reset();
        upload.reset(nullptr);
        download.reset(nullptr);
    }

    void save_header_error(SoftError err) noexcept {
        defered_error = err;
        settings.on_header_field = _on_data_IGNORE;
        settings.on_header_value = _on_data_IGNORE;
        settings.on_body = _on_data_IGNORE;
    }

    void reply_error(SoftError err) noexcept {
        return reply_error(err.http, err.soft, err.why);
    }

    void reply_error(int httpcode, int softcode, const char *reason) noexcept {
        char first[64], length[64];
        std::string payload;

        pack_error(payload, softcode, reason);
        snprintf(first, sizeof(first), "HTTP/%1hu.%1hu %03d Error\r\n",
                 parser->http_major, parser->http_minor, httpcode);
        snprintf(length, sizeof(length), "Content-Length: %lu\r\n",
                 payload.size());

        struct iovec iov[] = {
                STR_IOV(first),
                STR_IOV(length),
                BUF_IOV("Connection: close\r\n"),
                BUF_IOV("\r\n"),
                BUFLEN_IOV(payload.data(), payload.size())
        };
        cnx->send(iov, 5, mill_now() + 1000);
    }

    void reply_success() noexcept {
        char first[] = "HTTP/1.0 200 OK\r\n";
        first[5] = '0' + parser->http_major;
        first[7] = '0' + parser->http_minor;
        struct iovec iov[] = {
                STR_IOV(first),
                BUF_IOV("Connection: close\r\n"),
                BUF_IOV("Content-Length: 0\r\n"),
                BUF_IOV("\r\n"),
        };
        cnx->send(iov, 4, mill_now() + 1000);
    }

    void reply_stream() noexcept {
        char first[] = "HTTP/1.0 200 OK\r\n";
        first[5] = '0' + parser->http_major;
        first[7] = '0' + parser->http_minor;
        struct iovec iov[] = {
                STR_IOV(first),
                BUF_IOV("Connection: close\r\n"),
                BUF_IOV("Transfer-Encoding: chunked\r\n"),
                BUF_IOV("\r\n"),
        };
        cnx->send(iov, 4, mill_now() + 1000);
    }

    void reply_end_of_stream() noexcept {
        struct iovec iov = BUF_IOV("0\r\n\r\n");
        cnx->send(&iov, 1, mill_now() + 1000);
    }

    void reply_100() noexcept {

        if (!expect_100)
            return;
        expect_100 = false;

        char first[] = "HTTP/X.X 100 Continue\r\n";
        struct iovec iov[] = {
                BUF_IOV(first),
                BUF_IOV("Content-Length: 0\r\n"),
                BUF_IOV("\r\n"),
        };
        first[5] = '0' + parser->http_major;
        first[7] = '0' + parser->http_minor;
        cnx->send(iov, 3, mill_now() + 1000);
    }
};


/* -------------------------------------------------------------------------- */

int _on_headers_complete_UPLOAD(http_parser *p) {
    CnxContext *ctx = (CnxContext *) p->data;

    ctx->reply_100();
    ctx->settings.on_header_field = _on_trailer_field_COMMON;
    ctx->settings.on_header_value = _on_trailer_value_COMMON;

    // Get an upload obect
    auto builder = UploadBuilder(factory);
    builder.BlockSize(512 * 1024);
    builder.Name(ctx->chunk_id);
    for (const auto &to: ctx->targets)
        builder.Target(to);
    ctx->upload = builder.Build();

    auto rc = ctx->upload->Prepare();
    switch (rc) {
        case oio::blob::Upload::Status::OK:
            for (const auto &e: ctx->xattrs)
                ctx->upload->SetXattr(e.first, e.second);
            return 0;
        case oio::blob::Upload::Status::Already:
            ctx->reply_error({406, 421, "blobs found"});
            return 1;
        case oio::blob::Upload::Status::NetworkError:
            ctx->reply_error({502, 500, "network error to devices"});
            return 1;
        case oio::blob::Upload::Status::ProtocolError:
            ctx->reply_error({500, 500, "protocol error to devices"});
            return 1;
    }

    abort();
    return 1;
}

int _on_body_UPLOAD(http_parser *p, const char *buf, size_t len) {
    // We do not manage bodies exceeding 4GB at once
    if (len >= std::numeric_limits<uint32_t>::max()) {
        return 1;
    }

    auto ctx = (CnxContext *) p->data;
    ctx->upload->Write(reinterpret_cast<const uint8_t *>(buf), len);
    return 0;
}

int _on_chunk_header_UPLOAD(http_parser *p UNUSED) {
    DLOG(INFO) << __FUNCTION__ << "len=" << p->content_length << " r=" <<
    p->nread;
    return 0;
}

int _on_chunk_complete_UPLOAD(http_parser *p UNUSED) {
    DLOG(INFO) << __FUNCTION__;
    return 0;
}

int _on_message_complete_UPLOAD(http_parser *p) {
    auto ctx = (CnxContext *) p->data;
    auto upload_rc = ctx->upload->Commit();

    // Trigger a reply to the client
    if (upload_rc)
        ctx->reply_success();
    else
        ctx->reply_error(500, 400, "Upload commit failed");

    return 0;
}

/* -------------------------------------------------------------------------- */

int _on_headers_complete_DOWNLOAD(http_parser *p) {
    CnxContext *ctx = (CnxContext *) p->data;

    DownloadBuilder builder(factory);
    builder.Name(ctx->chunk_id);
    for (const auto t: ctx->targets)
        builder.Target(t);
    ctx->download = builder.Build();

    auto rc = ctx->download->Prepare();
    switch (rc) {
        case oio::blob::Download::Status::OK:
            ctx->reply_100();
            ctx->reply_stream();
            return 0;
        case oio::blob::Download::Status::NotFound:
            ctx->reply_error({404, 420, "blobs not found"});
            return 1;
        case oio::blob::Download::Status::NetworkError:
            ctx->reply_error({503, 500, "devices unreachable"});
            return 1;
        case oio::blob::Download::Status::ProtocolError:
            ctx->reply_error({502, 500, "invalid reply from device"});
            return 1;
    }

    abort();
    return 1;
}

int _on_message_complete_DOWNLOAD(http_parser *p UNUSED) {
    CnxContext *ctx = (CnxContext *) p->data;

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
            ctx->cnx->send(iov, 3, mill_now() + 1000);
        }
    }

    ctx->reply_end_of_stream();
    return 0;
}

/* -------------------------------------------------------------------------- */

int _on_headers_complete_REMOVAL(http_parser *p UNUSED) {
    auto ctx = (CnxContext *) p->data;

    RemovalBuilder builder(factory);
    builder.Name(ctx->chunk_id);
    for (const auto t: ctx->targets)
        builder.Target(t);
    ctx->removal = builder.Build();

    auto rc = ctx->removal->Prepare();
    switch (rc) {
        case oio::blob::Removal::Status::OK:
            ctx->reply_100();
            return 0;
        case oio::blob::Removal::Status::NotFound:
            ctx->reply_error({404, 402, "no blob found"});
            return 1;
        case oio::blob::Removal::Status::NetworkError:
            ctx->reply_error({503, 500, "devices unreachable"});
            return 1;
        case oio::blob::Removal::Status::ProtocolError:
            ctx->reply_error({502, 500, "invalid reply from devices"});
            return 1;
    }

    abort();
    return 1;
}

int _on_message_complete_REMOVAL(http_parser *p UNUSED) {
    auto ctx = (CnxContext *) p->data;
    assert(ctx->removal.get() != nullptr);

    auto rc = ctx->removal->Commit();
    if (rc) {
        ctx->reply_success();
        return 0;
    } else {
        ctx->reply_error({500, 500, "Removal impossible"});
        return 1;
    }
}

/* -------------------------------------------------------------------------- */

int _on_message_begin_COMMON(http_parser *p UNUSED) {
    ((CnxContext *) (p->data))->reset();
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
	DLOG(INFO) << "off " << u.field_data[f].off << " len " << u.field_data[f].len;
    assert(u.field_data[f].off <= len);
    assert(u.field_data[f].len <= len);
    assert(u.field_data[f].off + u.field_data[f].len <= len);
    return std::string(buf + u.field_data[f].off, u.field_data[f].len);
}

int _on_url_COMMON(http_parser *p, const char *buf, size_t len) {
    CnxContext *ctx = (CnxContext *) p->data;

    // Get the chunk_id, this is common to al the requests
    http_parser_url url;
    if (0 != http_parser_parse_url(buf, len, false, &url)) {
        ctx->save_header_error({400, 400, "URL parse error"});
        return 0;
    }
    if (!_http_url_has(url, UF_PATH)) {
        ctx->save_header_error({400, 400, "URL has no path"});
        return 0;
    }
    auto path = _http_url_field(url, UF_PATH, buf, len);
    auto last_sep = path.rfind('/');
    if (last_sep == std::string::npos) {
        ctx->save_header_error({400, 400, "URL has no/empty basename"});
        return 0;
    }
    if (last_sep == path.size() - 1) {
        ctx->save_header_error({400, 400, "URL has no/empty basename"});
        return 0;
    }

    ctx->chunk_id.assign(path, last_sep + 1, std::string::npos);
    return 0;
}

int _on_header_field_COMMON(http_parser *p, const char *buf, size_t len) {
    auto ctx = (CnxContext *) p->data;
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
    auto ctx = (CnxContext *) p->data;
    assert(ctx->defered_error.http == 0);
    if (ctx->last_field == HDR_OIO_TARGET)
        ctx->targets.emplace_back(buf, len);
    else if (ctx->last_field == HDR_OIO_XATTR) {
        if (p->method == HTTP_PUT) {
            ctx->xattrs[ctx->last_field_name] = std::move(std::string(buf, len));
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
    CnxContext *ctx = (CnxContext *) p->data;

    if (ctx->defered_error.http > 0) {
        DLOG(INFO) << __FUNCTION__ << " resuming a defered error";
        ctx->reply_error(ctx->defered_error);
        ctx->settings = default_settings;
        return 1;
    }

    if (ctx->targets.size() < 1) {
        DLOG(INFO) << "No target specified";
        ctx->reply_error({400, 400, "No target specified"});
        ctx->settings = default_settings;
        return 1;
    }

    if (p->method == HTTP_PUT) {
        ctx->settings = upload_settings;
        return _on_headers_complete_UPLOAD(p);
    } else if (p->method == HTTP_GET) {
        ctx->settings = download_settings;
        return _on_headers_complete_DOWNLOAD(p);
    } else if (p->method == HTTP_DELETE) {
        ctx->settings = removal_settings;
        return _on_headers_complete_REMOVAL(p);
    } else {
        ctx->reply_error({406, 406, "Method not managed"});
        return 1;
    }
}

/* -------------------------------------------------------------------------- */

/* copy the whole structure on the stack */
coroutine static void task_client(MillSocket *sock) noexcept {

    std::unique_ptr<MillSocket> front(sock);
    LOG(INFO) << "CLIENT fd " << front->fileno();

    struct http_parser parser;
    http_parser_init(&parser, HTTP_REQUEST);
    CnxContext cnx(front.get(), &parser);
    parser.data = &cnx;
    cnx.settings = default_settings;

    std::vector<uint8_t> buffer(8192);

    while (flag_running) {

        errno = EAGAIN;
        ssize_t sr = front->read(buffer.data(), buffer.size(), mill_now() + 1000);

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
                size_t consumed = http_parser_execute(
                        &parser, &(cnx.settings),
                        reinterpret_cast<const char *>(buffer.data() + done),
                        sr - done);
                if (parser.http_errno != 0) {
                    DLOG(INFO)
                    << "HTTP parsing error "
                    << parser.http_errno
                    << "/" << http_errno_name(static_cast<http_errno>(parser.http_errno))
                    << "/" << http_errno_description(static_cast<http_errno>(parser.http_errno));
                    goto out;
                }
                if (consumed > 0)
                    done += consumed;
            }
        }
    }
    out:

    DLOG(INFO) << "CLIENT " << front->fileno() << " done";
    front->close();
}

coroutine static void task_server(MillSocket &srv, chan out) noexcept {
    volatile bool input_ready = 1;

    while (flag_running) {

        /* poll a client and fire a coroutine */
        if (input_ready) {
            MillSocket cli;
            if (srv.accept(cli)) {
                mill_go(task_client(new MillSocket(cli)));
                continue;
            }
        }

        int rc = fdwait(srv.fileno(), FDW_IN, mill_now() + 1000);
        if (rc & FDW_ERR)
            break;
        input_ready = 0 != (rc & FDW_IN);
    }
	LOG(INFO) << "Exiting server " << srv.debug_string();
    chs(out, int, 1);
}

static void _sighandler_stop(int s UNUSED) noexcept {
    flag_running = 0;
}

static void _spawn_server(MillSocket &srv, chan out) noexcept {
    mill_go(task_server(srv, out));
}

static bool load_configuration_json(rapidjson::Document &doc) noexcept {
    if (doc.HasMember("bind")) {
        if (doc["bind"].IsArray()) {
            for (auto e = doc["bind"].Begin(); e != doc["bind"].End(); ++e) {
                if (!e->IsString())
                    continue;
                SRV.emplace_back();
                if (!SRV.back().bind(e->GetString()))
                    return false;
            }
        }
    }

    return true;
}

static bool load_configuration(const char *path) noexcept {
    // Read the file at once
    std::stringstream ss;
    std::ifstream ifs;
    ifs.open(path, std::ios::binary);
    ss << ifs.rdbuf(); // 1
    ifs.close();

    rapidjson::Document document;
    if (document.Parse<0>(ss.str().c_str()).HasParseError())
        return false;
    return load_configuration_json(document);
}

int main(int argc, char **argv) noexcept {
    google::InitGoogleLogging(argv[0]);
    FLAGS_logtostderr = true;

    //goprepare(4096, 16384, sizeof(void *));

    signal(SIGINT, _sighandler_stop);
    signal(SIGTERM, _sighandler_stop);
    signal(SIGUSR1, SIG_IGN);
    signal(SIGUSR2, SIG_IGN);
    signal(SIGHUP, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);

    if (argc < 2) {
        LOG(ERROR) << "Usage: " << argv[0] << " FILE [FILE...]";
        return 1;
    }
    for (int i = 1; i < argc; ++i)
        load_configuration(argv[i]);

    factory.reset(new CoroutineClientFactory);

    chan out = chmake(int, 0);

    for (auto &e: SRV) {
        if (!e.listen(default_backlog)) {
            LOG(WARNING) << "listen() error: (" << errno << ") " <<
            strerror(errno);
            goto out;
        }
    }

    /* start one coroutine per server */
    flag_running = true;
    for (auto &e: SRV)
        _spawn_server(e, out);

    /* Wait for the coroutines to exit */
    for (int i = SRV.size(); i > 0; --i) {
        int done = chr(out, int);
        assert(done != 0);
    }

    out:
    LOG(INFO) << "Cleaning";
    for (auto &e: SRV) {
        if (e.fileno() > 0) {
            fdclean(e.fileno());
            e.close();
        }
    }

    LOG(INFO) << "Exiting";
    chclose(out);
    return 0;
}
