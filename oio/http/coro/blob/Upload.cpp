/** Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#include <functional>
#include <sstream>
#include <iomanip>
#include <sys/uio.h>

#include <glog/logging.h>
#include <libmill.h>
#include <http-parser/http_parser.h>

#include <utils/MillSocket.h>
#include <utils/utils.h>
#include "oio/http/coro/blob.h"

using oio::api::blob::Upload;
using oio::http::coro::UploadBuilder;

class HttpUpload : public Upload {
    friend class UploadBuilder;

  public:
    HttpUpload();

    ~HttpUpload();

    void SetXattr(const std::string &k, const std::string &v) override;

    Upload::Status Prepare() override;

    bool Commit() override;

    bool Abort() override;

    void Write(const uint8_t *buf, uint32_t len) override;

  private:
    int64_t content_length;
    int64_t sent;
    std::string chunk_id;
    std::string host;
    std::shared_ptr<MillSocket> socket;
    std::map<std::string, std::string> fields;
    std::set<std::string> trailers;
    bool done;
};

HttpUpload::HttpUpload(): content_length{-1}, sent{0}, chunk_id(),
                           socket(nullptr), fields(), trailers() { }

HttpUpload::~HttpUpload() { }

void HttpUpload::SetXattr(const std::string &k, const std::string &v) {
    fields[k] = v;
}

struct ReplyContext {
    bool done;
    ReplyContext(): done{false} {}
};

static int on_reply_complete (struct http_parser *p) {
    reinterpret_cast<ReplyContext*>(p->data)->done = true;
    return 0;
}

bool HttpUpload::Commit() {
    int64_t dl_send = mill_now() + 2000;
    int64_t dl_recv = mill_now() + 4000;
    if (content_length >= 0) {
        // inline Transfer-encoding
        if (sent != content_length)
            return false;
    } else {
        // chunked
        if (!socket->send("0\r\n\r\n", 5, dl_send))
            return false;

        std::vector<std::string> hdr;
        for (const auto &k: trailers) {
            auto it = fields.find(k);
            if (it == fields.end())
                continue;
            std::stringstream ss;
            ss << RAWX_HDR_PREFIX << it->first << ": " << it->second << "\r\n";
            hdr.emplace_back(ss.str());
        }

        std::vector<struct iovec> iov;
        for (const auto &f: hdr) {
            struct iovec item = STRING_IOV(f);
            iov.emplace_back(item);
        }
        if (!socket->send(iov.data(), iov.size(), dl_send))
            return false;
    }

    // Consume the reply
    struct http_parser parser;
    http_parser_init(&parser, HTTP_RESPONSE);
    std::vector<uint8_t> buffer;
    ReplyContext ctx;
    parser.data = &ctx;

    struct http_parser_settings settings;
    memset(&settings, 0, sizeof(settings));
    settings.on_message_complete = on_reply_complete;
    //settings.on_headers_complete = on_reply_status;

    buffer.resize(8192);
    while (!ctx.done) {
        ssize_t rc = socket->read(buffer.data(), buffer.size(), dl_recv);
        if (rc <= 0)
            return false;

        ssize_t consumed = http_parser_execute(&parser, &settings,
                                           reinterpret_cast<const char *>(buffer.data()),
                                           rc);
        if (parser.http_errno != 0)
            return false;
        if (consumed != rc)
            return false;
    }

    return parser.status_code / 100 == 2;
}

bool HttpUpload::Abort() { return false; }

Upload::Status HttpUpload::Prepare() {
    std::vector<std::string> headers;

    /* first line */
    do {
        std::stringstream ss;
        ss << "PUT /chunk/" << chunk_id << " HTTP/1.1\r\n";
        headers.emplace_back(ss.str());
    } while (0);

    /* hostname */
    do {
        std::stringstream ss;
        ss << "Host: " << host << "\r\n";
        headers.emplace_back(ss.str());
    } while (0);

    /* content-length */
    if (content_length > 0) {
        std::stringstream ss;
        ss << "Content-Length: " << content_length << "\r\n";
        headers.emplace_back(ss.str());
    } else {
        /* Chunked encoding + trailers */
        headers.emplace_back("Transfer-encoding: chunked\r\n");
        if (!trailers.empty()) {
            std::stringstream ss;
            ss << "Trailers: ";
            bool first = true;
            for (const auto t: trailers) {
                if (!first) ss << ", ";
                first = false;
                ss << t;
            }
            ss << "\r\n";
        }
    }

    for (const auto &e: fields) {
        std::stringstream ss;
        ss << RAWX_HDR_PREFIX << e.first << ": " << e.second << "\r\n";
        headers.emplace_back(ss.str());
    }

    headers.emplace_back("\r\n");

    std::vector<struct iovec> iov;
    for (const auto &h: headers) {
        struct iovec item = STRING_IOV(h);
        iov.emplace_back(item);
    }
    bool rc = socket->send(iov.data(), iov.size(), mill_now() + 5000);
    if (!rc)
        return Upload::Status::NetworkError;
    return Upload::Status::OK;
}

void HttpUpload::Write(const uint8_t *buf, uint32_t len) {
    if (buf == nullptr || len == 0)
        return;
    int64_t dl = mill_now() + 1000;
    if (content_length > 0) {
        // Inline Transfer-Encoding
        if (socket->send(buf, len, dl))
            sent += len;
    } else {
        // chunked encoding
        std::stringstream ss;
        ss << std::hex << len << "\r\n";
        std::string hdr(ss.str());
        struct iovec iov[3] = {
                STRING_IOV(hdr),
                BUFLEN_IOV(buf, len),
                BUF_IOV("\r\n")
        };
        if (socket->send(iov, 3, dl))
            sent += len;
    }
}

UploadBuilder::UploadBuilder() { }

UploadBuilder::~UploadBuilder() { }

void UploadBuilder::Name(const std::string &s) { name.assign(s); }

void UploadBuilder::Host(const std::string &s) { host.assign(s); }

void UploadBuilder::Field(const std::string &k, const std::string &v) {
    fields[k] = v;
}

void UploadBuilder::Trailer(const std::string &k) {
    trailers.emplace(k);
}

std::shared_ptr<oio::api::blob::Upload> UploadBuilder::Build(
        std::shared_ptr<MillSocket> socket) {
    auto ul = new HttpUpload;
    ul->chunk_id.assign(name);
    ul->socket = socket;
    ul->host.assign(host);
    for (const auto &t: trailers)
        ul->trailers.insert(t);
    for (const auto &e: fields)
        ul->fields.emplace(e);
    std::shared_ptr<Upload> shared(ul);
    return shared;
}