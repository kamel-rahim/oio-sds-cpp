/** Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#include <functional>
#include <sstream>
#include <memory>
#include <iomanip>

#include <glog/logging.h>
#include <http-parser/http_parser.h>

#include <utils/net.h>
#include <utils/Http.h>
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
    http::Request request;
    http::Reply reply;
};

HttpUpload::HttpUpload() : request(), reply() {
}

HttpUpload::~HttpUpload() {
}

void HttpUpload::SetXattr(const std::string &k, const std::string &v) {
    request.Field(k, v);
}

bool HttpUpload::Commit() {
    auto rc = request.FinishRequest();

    rc = reply.ReadHeaders();
    while (rc == http::Code::OK) {
        std::vector<uint8_t> out;
        rc = reply.AppendBody(out);
    }

    return reply.Get().parser.status_code / 100 == 2;
}

bool HttpUpload::Abort() {
    return false;
}

Upload::Status HttpUpload::Prepare() {
    auto rc = request.WriteHeaders();
(void) rc;
    return Upload::Status::InternalError;
}

void HttpUpload::Write(const uint8_t *buf, uint32_t len) {
    if (buf == nullptr || len == 0)
        return;
    request.Write(buf, len);
}

UploadBuilder::UploadBuilder() {
}

UploadBuilder::~UploadBuilder() {
}

void UploadBuilder::Name(const std::string &s) {
    name.assign(s);
}

void UploadBuilder::Host(const std::string &s) {
    host.assign(s);
}

void UploadBuilder::Field(const std::string &k, const std::string &v) {
    fields[k] = v;
}

void UploadBuilder::Trailer(const std::string &k) {
    trailers.emplace(k);
}

std::shared_ptr<oio::api::blob::Upload> UploadBuilder::Build(
        std::shared_ptr<net::Socket> socket) {
    auto ul = new HttpUpload;
    ul->request.Socket(socket);
    ul->request.Method("PUT");
    ul->request.Selector(name);
    ul->request.Field("Host", host);
    for (const auto &t: trailers)
        ul->request.Trailer(t);
    for (const auto &e: fields)
        ul->request.Field(e.first, e.second);

    ul->reply.Socket(socket);

    std::shared_ptr<Upload> shared(ul);
    return shared;
}
