/**
 * This file is part of the OpenIO client libraries
 * Copyright (C) 2016 OpenIO SAS
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3.0 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.
 */

#include "utils/http.hpp"

#include <sys/uio.h>

#include <libmill.h>
#include <gflags/gflags.h>

#include <iomanip>
#include <cstring>
#include <cassert>

using http::Request;
using http::Reply;
using http::Call;
using http::Code;

DEFINE_bool(http_trace, false, "Log all calls to the HTTP parser");

#define HTTP_LOG() DLOG_IF(INFO, FLAGS_http_trace) << __FUNCTION__ << ' '

namespace http {

static int _on_data_IGNORE(http_parser *p UNUSED, const char *b UNUSED,
        size_t l UNUSED) {
    return 0;
}

static int _on_reply_begin(http_parser *p) {
    auto ctx = reinterpret_cast<Reply::Context*>(p->data);
    HTTP_LOG();
    assert(ctx->step == Reply::Step::Beginning);
    ctx->step = Reply::Step::Headers;
    return 0;
}

static int _on_header_field(http_parser *p, const char *b, size_t l) {
    auto ctx = reinterpret_cast<Reply::Context*>(p->data);
    HTTP_LOG();
    assert(ctx->step == Reply::Step::Headers);
    ctx->tmp.assign(b, l);
    return 0;
}

static int _on_header_value(http_parser *p, const char *b, size_t l) {
    auto ctx = reinterpret_cast<Reply::Context*>(p->data);
    HTTP_LOG();
    assert(ctx->step == Reply::Step::Headers);
    ctx->fields[ctx->tmp] = std::string(b, l);
    return 0;
}

static int _on_headers_complete(http_parser *p) {
    auto ctx = reinterpret_cast<Reply::Context*>(p->data);
    HTTP_LOG();
    assert(ctx->step == Reply::Step::Headers);

    // The goal is to reduce the number of syscalls (i.e. reading big chunks)
    // but avoiding to allocate to big buffers. These numbers are an arbitrary
    // heuristic but tend to make
    if (ctx->content_length > 32 * 1024 || ctx->content_length < 0)
        ctx->buffer.resize(128 * 1024);
    else if (ctx->content_length > 2048)
        ctx->buffer.resize(8192);

    ctx->step = Reply::Step::Body;

    // The application is written in an imperative style. It pilots the calls to
    // the parser, with calls like ReadHeaders(), ReadBody(). We need to stop
    // once the body has been consumed.
    http_parser_pause(&ctx->parser, true);

    return 0;
}

static int _on_body(http_parser *p, const char *buf, size_t len) {
    auto ctx = reinterpret_cast<Reply::Context*>(p->data);
    HTTP_LOG();
    assert(ctx->step == Reply::Step::Body);

    ctx->body_bytes.push(Reply::Slice(reinterpret_cast<const uint8_t *>(buf),
                                      static_cast<uint32_t>(len)));
    return 0;
}

static int _on_reply_complete(http_parser *p) {
    auto ctx = reinterpret_cast<Reply::Context*>(p->data);
    HTTP_LOG();
    assert(ctx->step == Reply::Step::Body);

    // If several answers are queued, we need to pause the parser for the same
    // reason we pause it after the headers: to let the application manage its
    // logic between two replies.
    http_parser_pause(&ctx->parser, true);

    ctx->step = Reply::Step::Done;
    return 0;
}

static int _on_chunk_header(http_parser *p UNUSED) {
    auto ctx = reinterpret_cast<Reply::Context*>(p->data);
    HTTP_LOG();
    assert(ctx->step == Reply::Step::Body);
    return 0;
}

static int _on_chunk_complete(http_parser *p UNUSED) {
    auto ctx = reinterpret_cast<Reply::Context*>(p->data);
    HTTP_LOG();
    assert(ctx->step == Reply::Step::Body);
    return 0;
}

};  // namespace http

std::ostream& http::operator<<(std::ostream &out, const http::Code code) {
    switch (code) {
        case http::Code::OK:
            out << "OK";
            break;
        case http::Code::ClientError:
            out << "ClientError";
            break;
        case http::Code::ServerError:
            out << "ServerError";
            break;
        case http::Code::NetworkError:
            out << "NetworkError";
            break;
        case http::Code::Done:
            out << "Done";
            break;
        default:
            out << "***invalid***";
            break;
    }
    out << "/" << static_cast<int>(code);
    return out;
}

std::ostream& http::operator<<(std::ostream &out,
                               const http::Reply::Step step) {
    switch (step) {
        case http::Reply::Step::Beginning:
            out << "Beginning";
            break;
        case http::Reply::Step::Headers:
            out << "Headers";
            break;
        case http::Reply::Step::Body:
            out << "Body";
            break;
        case http::Reply::Step::Done:
            out << "Done";
            break;
        default:
            out << "***invalid***";
            break;
    }
    out << "/" << static_cast<int>(step);
    return out;
}

Request::Request()
        : method("GET"), selector("/"), fields(), query(), trailers(),
          socket(nullptr), content_length{-1}, sent{0} {}

Request::Request(std::shared_ptr<net::Socket> s)
        : method("GET"), selector("/"), fields(), query(), trailers(),
          socket(s), content_length{-1}, sent{0} {}

Request::~Request() {}

Code Request::WriteHeaders() {
    std::vector<std::string> headers;

    /* first line */
    do {
        std::stringstream ss;
        ss << method << " " << selector;
        if (!query.empty()) {
            bool first = true;
            for (const auto &e : query) {
                ss << (first ? "?" : "&");
                // TODO(jfs): URL-encode the query string
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
            for (const auto t : trailers) {
                if (!first)
                    ss << ", ";
                first = false;
                ss << t;
            }
            ss << "\r\n";
            headers.emplace_back(ss.str());
        }
    }

    for (const auto &e : fields) {
        std::stringstream ss;
        ss << e.first << ": " << e.second << "\r\n";
        headers.emplace_back(ss.str());
    }

    headers.emplace_back("\r\n");

    std::vector<struct iovec> iov;
    for (const auto &h : headers) {
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
        for (const auto &k : trailers) {
            auto it = fields.find(k);
            if (it == fields.end())
                continue;
            std::stringstream ss;
            ss << it->first << ": " << it->second << "\r\n";
            hdr.emplace_back(ss.str());
        }

        std::vector<struct iovec> iov;
        for (const auto &f : hdr) {
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

Reply::~Reply() {}

Code Reply::consumeInput(int64_t dl) {
    assert(dl > 0);
    assert(ctx.body_bytes.empty());

    HTTP_LOG() << "ready";

    // If no data immediately ready, then fill the buffer from the network.
    bool eof = false;
    if (ctx.buffer_offset >= ctx.buffer_length) {
        ssize_t rc = socket->read(ctx.buffer.data(), ctx.buffer.size(), dl);
        if (rc < 0)
            return Code::NetworkError;
        eof = (rc == 0);
        ctx.buffer_offset = 0;
        ctx.buffer_length = static_cast<unsigned int>(rc);
        HTTP_LOG() << rc << " bytes received";
    }

    const char *b = reinterpret_cast<const char *>(ctx.buffer.data())
                        + ctx.buffer_offset;
    const ssize_t l = ctx.buffer_length - ctx.buffer_offset;

    // EOF received? we need to tell the HTTP parser.
    if (eof) {
        auto rc = http_parser_execute(&ctx.parser, &settings, b, 0);
        HTTP_LOG() << "EOF received, http_parser rc=" << rc;
        return Code::OK;
    }

    // Real data available, make it pass through the parser
    HTTP_LOG() << l << " bytes ready [" << std::string(b, l) << ']';
    http_parser_pause(&ctx.parser, 0);
    const ssize_t done = http_parser_execute(&ctx.parser, &settings, b, l);
    HTTP_LOG() << done << '/' << l << " bytes managed";

    if (done > 0) {
        ctx.buffer_offset += done;
        return Code::OK;
    } else {
        LOG(INFO) << "Unexpected http error " << ctx.parser.http_errno
                  << " (" << ::strerror(ctx.parser.http_errno) << ")";
        return Code::ServerError;
    }
}

Code Reply::ReadHeaders() {
    int64_t dl = mill_now() + 8000;
    if (ctx.step > Headers) {
        LOG(ERROR) << "Headers already read";
        return Code::ClientError;
    }
    while (ctx.step <= Step::Headers) {
        auto rc = consumeInput(dl);
        HTTP_LOG() << "step=" << ctx.step << " consumeInput rc=" << rc;
        if (rc != Code::OK)
            return rc;
    }
    return Code::OK;
}

Code Reply::GetHeaders(std::map <std::string, std::string> *data,
                       std::string prefix) {
    for (const auto &e : ctx.fields) {
        size_t pos = e.first.find(prefix);
        if (pos != std::string::npos) {
            pos += prefix.size();
            std::string v = e.first.substr(pos, e.first.size() - pos);
            (*data) [v] = e.second;
        }
    }
    return Code::OK;
}

Code Reply::ReadBody(Reply::Slice *out) {
    assert(out != nullptr);
    int64_t dl = mill_now() + 8000;

    out->buf = nullptr;
    out->len = 0;

    for (;;) {
        // If some data already available, serve it
        if (!ctx.body_bytes.empty()) {
            *out = ctx.body_bytes.front();
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

Code Reply::AppendBody(std::vector<uint8_t> *out) {
    Reply::Slice slice;
    auto rc = ReadBody(&slice);
    if (rc != Code::OK)
        return rc;

    if (slice.buf != nullptr && slice.len > 0) {
        const auto len0 = out->size();
        out->resize(len0 + slice.len);
        memcpy(out->data() + len0, slice.buf, slice.len);
    }
    return Code::OK;
}

void Reply::Skip() {
    HTTP_LOG();
    while (ctx.step < Step::Done) {
        auto rc = consumeInput(mill_now() + 4000);
        if (rc != Code::OK)
            break;
    }
    HTTP_LOG() << "reply skipped!";
    init();
    ctx.Reset();
}

Call::Call() : request(nullptr), reply(nullptr) {}

Call::Call(std::shared_ptr<net::Socket> s) : request(s), reply(s) {}

Call::~Call() {}

Code Call::Run(const std::string &in, std::string *out) {
    assert(out != nullptr);
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
        rc = reply.AppendBody(&tmp);
        LOG_IF(WARNING, rc != http::Code::OK && rc != http::Code::Done)
        << "Read(body) error " << rc;
    }

    out->assign(reinterpret_cast<const char *>(tmp.data()), tmp.size());
    return http::Code::OK;
}

Code Call::GetReplyHeaders(std::map <std::string,
                           std::string> *data, std::string prefix) {
    return reply.GetHeaders(data, prefix);
}
