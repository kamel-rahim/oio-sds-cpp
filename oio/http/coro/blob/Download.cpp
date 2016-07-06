/** Copyright (c) 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#include <cstring>
#include <string>
#include <sstream>
#include <utils/utils.h>
#include <utils/Http.h>
#include "oio/http/coro/blob.h"

using oio::api::blob::Download;
using oio::http::coro::DownloadBuilder;

class HttpDownload : public Download {
    friend class DownloadBuilder;

  public:
    HttpDownload();

    ~HttpDownload();

    bool IsEof() override;

    Download::Status Prepare() override;

    int32_t Read(std::vector<uint8_t> &buf) override;

  private:
    http::Request request;
    http::Reply reply;
};

HttpDownload::HttpDownload() : request(), reply() {
}

HttpDownload::~HttpDownload() {
}

bool HttpDownload::IsEof() {
    return reply.Get().step == http::Reply::Step::Done;
}

Download::Status HttpDownload::Prepare() {

    auto rc = request.WriteHeaders();
    if (rc != http::Code::OK && rc != http::Code::Done) {
        if (rc == http::Code::NetworkError)
            return Download::Status::NetworkError;
        return Download::Status::InternalError;
    }

    rc = request.FinishRequest();
    if (rc != http::Code::OK && rc != http::Code::Done) {
        if (rc == http::Code::NetworkError)
            return Download::Status::NetworkError;
        return Download::Status::InternalError;
    }

    rc = reply.ReadHeaders();
    if (rc != http::Code::OK && rc != http::Code::Done) {
        if (rc == http::Code::NetworkError)
            return Download::Status::NetworkError;
        return Download::Status::InternalError;
    }

    return Download::Status::OK;
}

int32_t HttpDownload::Read(std::vector<uint8_t> &buf) {
    buf.clear();
    reply.AppendBody(buf);
    return buf.size();
}

DownloadBuilder::DownloadBuilder() {
}

DownloadBuilder::~DownloadBuilder() {
}

void DownloadBuilder::Field(const std::string &k, const std::string &v) {
    fields[k] = v;
}

void DownloadBuilder::Host(const std::string &s) {
    host.assign(s);
}

void DownloadBuilder::Name(const std::string &s) {
    name.assign(s);
}

std::shared_ptr<oio::api::blob::Download> DownloadBuilder::Build(
        std::shared_ptr<MillSocket> socket) {
    auto dl = new HttpDownload;
    dl->request.Socket(socket);
    dl->request.Method("GET");
    dl->request.Selector(name);
    dl->request.Field("Host", host);
    for (const auto &e: fields)
        dl->request.Field(e.first, e.second);

    dl->reply.Socket(socket);

    std::shared_ptr<Download> shared(dl);
    return shared;
}