/** Copyright (c) 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#include <cstring>
#include <string>
#include <sstream>
#include <sys/uio.h>
#include <libmill.h>
#include <http-parser/http_parser.h>
#include <utils/MillSocket.h>
#include <utils/utils.h>
#include <libmill.h>
#include "oio/http/coro/blob.h"

using oio::api::blob::Download;

struct ReplyCtx {
    enum { BEGIN, HEADERS, BODY, DONE} step = BEGIN;
    std::vector<uint8_t> data;
};

class HttpDownload : public Download {
  public:
    HttpDownload();

    ~HttpDownload();

    bool IsEof() noexcept;

    Download::Status Prepare() noexcept;

    int32_t Read(std::vector<uint8_t> &buf) noexcept;

  private:
    std::shared_ptr<MillSocket> socket;
    http_parser parser;
    http_parser_settings settings;
    std::string host;
    std::string selector;
    std::map<std::string,std::string> query;
    std::map<std::string,std::string> fields;
    ReplyCtx ctx;
};

HttpDownload::HttpDownload() noexcept : socket() {
    memset(&settings, 0, sizeof(settings));
    http_parser_init(&parser, HTTP_RESPONSE);
    parser.data = &ctx;
}

HttpDownload::~HttpDownload() noexcept { }

bool HttpDownload::IsEof() noexcept {
    return ctx.step == ReplyCtx::DONE;
}

Download::Status HttpDownload::Prepare() noexcept {
    std::vector<std::string> headers;

    // first line
    do {
        std::stringstream ss;
        ss << "GET " << selector;
        if (!query.empty()) {
            bool first = true;
            for (const auto &e: query) {
                ss << (first ? "?" : "&") << e.first << "=" << e.second;
                first = false;
            }
        }
        ss << " HTTP/1.1\r\n";
        headers.emplace_back(ss.str());
    } while (0);

    do {
        std::stringstream ss;
        ss << "Host: " << host << "\r\n";
        headers.emplace_back(ss.str());
    } while (0);

    headers.emplace_back("Content-Length: 0\r\n");

    // Headers
    if (!fields.empty()) {
        for (const auto &e: fields) {
            std::stringstream ss;
            ss << RAWX_HDR_PREFIX << e.first << ": " << e.second << "\r\n";
            headers.emplace_back(ss.str());
        }
    }

    headers.emplace_back("\r\n");

    std::vector<iovec> iov;
    for (auto &h: headers) {
        struct iovec item = STRING_IOV(h);
        iov.emplace_back(item);
    }

    bool rc_send = socket->send(iov.data(), iov.size(), mill_now()+1000);
    if (!rc_send)
        return Download::Status::NetworkError;

    return Download::Status::OK;
}

int32_t HttpDownload::Read(std::vector<uint8_t> &buf) noexcept {

}