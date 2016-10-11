/** Copyright (c) 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#include <iomanip>
#include <sstream>
#include <cstring>
#include <cassert>
#include <sys/uio.h>

#include <libmill.h>
#include <http-parser/http_parser.h>

#include "macros.h"
#include "utils.h"
#include "Http.h"

using http::Request;
using http::Reply;
using http::Call;
using http::Code;

namespace http {

//static int _on_IGNORE(http_parser *p UNUSED) { return 0; }

static int _on_data_IGNORE(http_parser *p UNUSED, const char *b UNUSED,
        size_t l UNUSED) {
    return 0;
}

static int _on_reply_begin(http_parser *p) {
    auto ctx = reinterpret_cast<Reply::Context *>(p->data);
    ctx->step = Reply::Step::Headers;
    return 0;
}

static int _on_header_field(http_parser *p, const char *b, size_t l) {
    auto ctx = reinterpret_cast<Reply::Context *>(p->data);
    ctx->tmp.assign(b, l);
    return 0;
}

static int _on_header_value(http_parser *p, const char *b, size_t l) {
    auto ctx = reinterpret_cast<Reply::Context *>(p->data);
    ctx->fields[ctx->tmp] = std::string(b, l);
    return 0;
}

static int _on_headers_complete(http_parser *p) {
    auto ctx = reinterpret_cast<Reply::Context *>(p->data);
    // The goal is to reduce the number of syscalls (i.e. reading big chunks)
    // but avoiding to allocate to big buffers. These numbers are an arbitrary
    // heuristic but tend to make
    if (ctx->content_length > 32 * 1024)
        ctx->buffer.resize(128 * 1024);
    else if (ctx->content_length > 2048)
        ctx->buffer.resize(8192);
    ctx->step = Reply::Step::Body;
    return 0;
}

static int _on_body(http_parser *p, const char *buf, size_t len) {
    auto ctx = reinterpret_cast<Reply::Context *>(p->data);
    ctx->body_bytes.push(Reply::Slice(reinterpret_cast<const uint8_t *>(buf),
                                      static_cast<uint32_t>(len)));
    return 0;
}

static int _on_reply_complete(http_parser *p) {
    auto ctx = reinterpret_cast<Reply::Context *>(p->data);
    ctx->step = Reply::Step::Done;
    return 0;
}

static int _on_chunk_header(http_parser *p UNUSED) {
    //auto ctx = reinterpret_cast<Context *>(p->data);
    return 0;
}

static int _on_chunk_complete(http_parser *p UNUSED) {
    //auto ctx = reinterpret_cast<Context *>(p->data);
    return 0;
}

}; // namespace http

Request::Request()
        : method("GET"), selector("/"), fields(), query(), trailers(),
          socket(nullptr), content_length{-1}, sent{0} {
}

Request::Request(std::shared_ptr<net::Socket> s)
        : method("GET"), selector("/"), fields(), query(), trailers(),
          socket(s), content_length{-1}, sent{0} {
}

Request::~Request() {
}

Code Request::WriteHeaders() {
    std::vector<std::string> headers;

    /* first line */
    do {
        std::stringstream ss;
        ss << method << " " << selector;
        if (!query.empty()) {
            bool first = true;
            for (const auto &e: query) {
                ss << (first ? "?" : "&");
                // TODO URL-encode the query string
                ss << e.first << "=" << e.second;
                first = false;
            }
        }
        ss << " HTTP/1.1\r\n";
        headers.emplace_back(ss.str());
    } while (0);

    /* content-length */
    if (content_length >= 0) {
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
                if (!first)
                    ss << ", ";
                first = false;
                ss << t;
            }
            ss << "\r\n";
            headers.emplace_back(ss.str());
        }
    }

    for (const auto &e: fields) {
        std::stringstream ss;
        ss << e.first << ": " << e.second << "\r\n";
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
        return Code::NetworkError;
    return Code::OK;
}

Code Request::Write(const uint8_t *buf, uint32_t len) {
    if (buf == nullptr)
        return Code::ClientError;
    if (len == 0)
        return Code::OK;

    int64_t dl = mill_now() + 1000;
    if (content_length > 0) {
        // Inline Transfer-Encoding
        if (socket->send(buf, len, dl))
            sent += len;
        else
            return Code::NetworkError;
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
        else
            return Code::NetworkError;
    }
    return Code::OK;
}

Code Request::FinishRequest() {
    int64_t dl_send = mill_now() + 2000;
    if (content_length >= 0) {
        // inline Transfer-encoding
        if (sent != content_length) {
            LOG(ERROR) << "Too few bytes have been sent";
            return Code::ClientError;
        }
    } else {
        // chunked encoding, so we first send the final chunk
        if (!socket->send("0\r\n\r\n", 5, dl_send))
            return Code::NetworkError;

        // and there are maybe trailers
        std::vector<std::string> hdr;
        for (const auto &k: trailers) {
            auto it = fields.find(k);
            if (it == fields.end())
                continue;
            std::stringstream ss;
            ss << it->first << ": " << it->second << "\r\n";
            hdr.emplace_back(ss.str());
        }

        std::vector<struct iovec> iov;
        for (const auto &f: hdr) {
            struct iovec item = STRING_IOV(f);
            iov.emplace_back(item);
        }
        if (!socket->send(iov.data(), iov.size(), dl_send))
            return Code::NetworkError;
    }

    return Code::OK;
}

void Reply::init() {
    memset(&settings, 0, sizeof(settings));
    settings.on_message_begin = http::_on_reply_begin;
    settings.on_url = http::_on_data_IGNORE;
    settings.on_status = http::_on_data_IGNORE;
    settings.on_header_field = http::_on_header_field;
    settings.on_header_value = http::_on_header_value;
    settings.on_headers_complete = http::_on_headers_complete;
    settings.on_body = http::_on_body;
    settings.on_message_complete = http::_on_reply_complete;
    settings.on_chunk_complete = http::_on_chunk_complete;
    settings.on_chunk_header = http::_on_chunk_header;
}

Reply::Reply() : socket(nullptr), ctx() { init(); }

Reply::Reply(std::shared_ptr<net::Socket> s) : socket(s), ctx() { init(); }

Reply::~Reply() {
}

Code Reply::consumeInput(int64_t dl) {

    assert(ctx.body_bytes.empty());

    // TODO the default read timeout must be configurable
    if (dl < 0)
        dl = mill_now() + 8000;

    ssize_t rc = socket->read(ctx.buffer.data(), ctx.buffer.size(), dl);
    if (rc <= 0)
        return Code::NetworkError;

    ssize_t consumed = http_parser_execute(&ctx.parser, &settings,
                                           reinterpret_cast<const char *>(ctx.buffer.data()),
                                           rc);
    if (ctx.parser.http_errno != 0) {
        LOG(INFO) << "Unexpected http error " << ctx.parser.http_errno;
        return Code::ServerError;
    }

    if (consumed != rc)
        return Code::NetworkError;

    return Code::OK;
}

Code Reply::ReadHeaders() {
    int64_t dl = mill_now() + 8000;
    if (ctx.step > Headers) {
        LOG(ERROR) << "Headers already read";
        return Code::ClientError;
    }
    while (ctx.step <= Step::Headers) {
        auto rc = consumeInput(dl);
        if (rc != Code::OK)
            return rc;
    }
    return Code::OK;
}

Code Reply::ReadBody(Reply::Slice &out) {
    int64_t dl = mill_now() + 8000;

    out.buf = nullptr;
    out.len = 0;

    for (;;) {
        // If some data already available, serve it
        if (!ctx.body_bytes.empty()) {
            out = ctx.body_bytes.front();
            ctx.body_bytes.pop();
            return Code::OK;
        }

        // No data available, can we consume new data from the socket?
        if (ctx.step == Step::Done)
            return Code::Done;
        if (ctx.step != Step::Body) {
            LOG(ERROR) << "Unexpected reply step (" << ctx.step << ")";
            return Code::ClientError;
        }

        // Let's try to read some
        auto rc = consumeInput(dl);
        if (rc != Code::OK)
            return rc;
    }
}

Code Reply::AppendBody(std::vector<uint8_t> &out) {
    Reply::Slice slice;
    auto rc = ReadBody(slice);
    if (rc != Code::OK)
        return rc;

    if (slice.buf != nullptr && slice.len > 0) {
        const auto len0 = out.size();
        out.resize(len0 + slice.len);
        memcpy(out.data() + len0, slice.buf, slice.len);
    }
    return Code::OK;
}

Call::Call(): request(nullptr), reply(nullptr) {
}

Call::Call(std::shared_ptr<net::Socket> s) : request(s), reply(s) {
}

Call::~Call() {
}

Code Call::Run(const std::string &in, std::string &out) {

    request.ContentLength(in.length());

    auto rc = request.WriteHeaders();
    if (rc != http::Code::OK) {
        LOG(WARNING) << "Write(headers) error " << rc;
        return rc;
    }

    rc = request.Write((const uint8_t *) in.data(), in.length());
    if (rc != http::Code::OK) {
        LOG(WARNING) << "Write(body) error " << rc;
        return rc;
    }

    rc = request.FinishRequest();
    if (rc != http::Code::OK) {
        LOG(WARNING) << "Write(final) error " << rc;
        return rc;
    }

    std::vector<uint8_t> tmp;

    rc = reply.ReadHeaders();
    LOG_IF(WARNING, rc != http::Code::OK) << "Read(headers) error " << rc;

    while (rc == http::Code::OK) {
        rc = reply.AppendBody(tmp);
        LOG_IF(WARNING, rc != http::Code::OK && rc != http::Code::Done)
        << "Read(body) error " << rc;
    }

    out.assign(reinterpret_cast<const char *>(tmp.data()), tmp.size());
    return http::Code::OK;
}