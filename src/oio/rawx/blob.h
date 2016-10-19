/**
 * Copyright (c) 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/
 */

#ifndef SRC_OIO_RAWX_BLOB_H_
#define SRC_OIO_RAWX_BLOB_H_

#include <string>
#include <memory>
#include <map>

#include "oio/api/blob.h"
#include "oio/http/blob.h"

namespace oio {
namespace rawx {
namespace blob {

class DownloadBuilder {
 public:
    DownloadBuilder();

    ~DownloadBuilder();

    void Host(const std::string &s);

    void Name(const std::string &s);

    void Field(const std::string &k, const std::string &v);

    std::shared_ptr<oio::api::blob::Download> Build(
            std::shared_ptr<net::Socket> socket);

 private:
    oio::http::imperative::DownloadBuilder inner;
};

class UploadBuilder {
 public:
    UploadBuilder();

    ~UploadBuilder();

    void Host(const std::string &s);

    void Name(const std::string &s);

    void Field(const std::string &k, const std::string &v);

    void Trailer(const std::string &k);

    std::shared_ptr<oio::api::blob::Upload> Build(
            std::shared_ptr<net::Socket> socket);

 private:
    oio::http::imperative::UploadBuilder inner;
};

class RemovalBuilder {
 public:
    RemovalBuilder();

    ~RemovalBuilder();

    void Host(const std::string &s);

    void Name(const std::string &s);

    void Field(const std::string &k, const std::string &v);

    void Trailer(const std::string &k);

    std::shared_ptr<oio::api::blob::Removal> Build(
            std::shared_ptr<net::Socket> socket);

 private:
    oio::http::imperative::RemovalBuilder inner;
};


}  // namespace blob
}  // namespace rawx
}  // namespace oio

#endif  // SRC_OIO_RAWX_BLOB_H_
