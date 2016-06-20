/** Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#include <sstream>
#include <sys/uio.h>
#include <libmill.h>
#include <utils/MillSocket.h>
#include <utils/utils.h>
#include "Upload.h"

using oio::api::blob::Upload;
using oio::http::coro::UploadBuilder;

class HttpUpload : public Upload {
    friend class UploadBuilder;

  public:
    HttpUpload();

    ~HttpUpload();

    void SetXattr(const std::string &k, const std::string &v);

    Upload::Status Prepare();

    bool Commit();

    bool Abort();

    void Flush();

    void Write(const uint8_t *buf, uint32_t len);

  private:
    int64_t content_length;
    std::string chunk_id;
    std::shared_ptr<MillSocket> socket;
    std::map<std::string, std::string> fields;
    std::set<std::string> trailers;
};

HttpUpload::HttpUpload() : content_length{-1}, chunk_id(), socket(nullptr),
                           fields(), trailers() { }

HttpUpload::~HttpUpload() { }

void HttpUpload::SetXattr(const std::string &k, const std::string &v) {
    fields[k] = v;
}

bool HttpUpload::Commit() {
    return false;
}

void HttpUpload::Flush() {

}

bool HttpUpload::Abort() {
    return false;
}

Upload::Status HttpUpload::Prepare() {
    std::vector<std::string> hdr;

    /* first line */
    do {
        std::stringstream ss;
        ss << "PUT /chunk/" << chunk_id << " HTTP/1.1\r\n";
        hdr.emplace_back(ss.str());
    } while (0);

    /* content-length */
    if (content_length > 0) {
        std::stringstream ss;
        ss << "Content-Length: " << content_length << "\r\n";
        hdr.emplace_back(ss.str());
    } else {
        /* Chunked encoding + trailers */
        hdr.emplace_back("Transfer-encoding: chunked\r\n");
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

    hdr.emplace_back("\r\n");

    std::vector<struct iovec> iov;
    for (const auto &h: hdr) {
        struct iovec item = STRING_IOV(h);
        iov.emplace_back(item);
    }
    socket->send(iov.data(), iov.size(), mill_now() + 5000);

    return Upload::Status::InternalError;
}

void HttpUpload::Write(const uint8_t *buf, uint32_t len) {
    socket->send(buf, len, mill_now() + 1000);
}

UploadBuilder::UploadBuilder() { }

UploadBuilder::~UploadBuilder() { }

void UploadBuilder::Name(const std::string &s) {
    chunkid.assign(s);
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
        std::shared_ptr<MillSocket> socket) {
    auto ul = new HttpUpload;
    ul->socket = socket;
    std::shared_ptr<Upload> shared(ul);
    return shared;
}