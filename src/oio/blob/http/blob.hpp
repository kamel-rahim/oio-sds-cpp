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

#ifndef SRC_OIO_BLOB_HTTP_BLOB_HPP__
#define SRC_OIO_BLOB_HTTP_BLOB_HPP__

#include <string>
#include <memory>
#include <map>
#include <set>

#include "utils/net.hpp"
#include "oio/api/blob.hpp"

namespace oio {
namespace http {
namespace imperative {

class DownloadBuilder {
 public:
    DownloadBuilder();

    ~DownloadBuilder();

    void Host(const std::string &s);

    void Name(const std::string &s);

    void Field(const std::string &k, const std::string &v);

    std::unique_ptr<oio::api::blob::Download> Build(
            std::shared_ptr<net::Socket> socket);

 protected:
    std::string host;
    std::string name;
    std::map<std::string, std::string> fields;
};

class UploadBuilder {
 public:
    UploadBuilder();

    ~UploadBuilder();

    void Host(const std::string &s);

    void Name(const std::string &s);

    void Field(const std::string &k, const std::string &v);

    void Trailer(const std::string &k);

    std::unique_ptr<oio::api::blob::Upload> Build(
            std::shared_ptr<net::Socket> socket);

 private:
    std::string host;
    std::string name;
    std::map<std::string, std::string> fields;
    std::set<std::string> trailers;
};

class RemovalBuilder {
 public:
    RemovalBuilder();

    ~RemovalBuilder();

    void Host(const std::string &s);

    void Name(const std::string &s);

    void Field(const std::string &k, const std::string &v);

    void Trailer(const std::string &k);

    std::unique_ptr<oio::api::blob::Removal> Build(
            std::shared_ptr<net::Socket> socket);

 private:
    std::string host;
    std::string name;
    std::map<std::string, std::string> fields;
    std::set<std::string> trailers;
};

}  // namespace imperative
}  // namespace http
}  // namespace oio

#endif  // SRC_OIO_BLOB_HTTP_BLOB_HPP__
