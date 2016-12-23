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

#ifndef SRC_OIO_KINETIC_CORO_BLOB_H_
#define SRC_OIO_KINETIC_CORO_BLOB_H_

#include <string>
#include <map>
#include <set>
#include <memory>

#include "oio/api/blob.h"
#include "oio/blob/kinetic/coro/client/ClientInterface.h"

namespace oio {
namespace kinetic {
namespace blob {

class UploadBuilder {
 public:
    explicit UploadBuilder(
            std::shared_ptr<oio::kinetic::client::ClientFactory> f);

    ~UploadBuilder();

    bool Target(const char *to);

    bool Target(const std::string &to);

    bool Name(const char *n);

    bool Name(const std::string &n);

    void BlockSize(uint32_t s);

    std::unique_ptr<oio::api::blob::Upload> Build();

 private:
    std::shared_ptr<oio::kinetic::client::ClientFactory> factory;
    std::set<std::string> targets;
    std::string name;
    uint32_t block_size;
};

class DownloadBuilder {
 public:
    explicit DownloadBuilder(
            std::shared_ptr<oio::kinetic::client::ClientFactory> f);

    ~DownloadBuilder();

    bool Name(const char *n);

    bool Name(const std::string &n);

    bool Target(const char *to);

    bool Target(const std::string &to);

    std::unique_ptr<oio::api::blob::Download> Build();

 private:
    std::shared_ptr<oio::kinetic::client::ClientFactory> factory;
    std::set<std::string> targets;
    std::string name;
};

class RemovalBuilder {
 public:
    explicit RemovalBuilder(
            std::shared_ptr<oio::kinetic::client::ClientFactory> f);

    bool Name(const std::string &n);

    bool Name(const char *n);

    bool Target(const std::string &to);

    bool Target(const char *to);

    std::unique_ptr<oio::api::blob::Removal> Build();

 private:
    std::shared_ptr<oio::kinetic::client::ClientFactory> factory;
    std::set<std::string> targets;
    std::string name;
};

class ListingBuilder {
 public:
    explicit ListingBuilder(
            std::shared_ptr<oio::kinetic::client::ClientFactory> f);

    ~ListingBuilder();

    bool Name(const std::string &n);

    bool Name(const char *n);

    bool Target(const std::string &to);

    bool Target(const char *to);

    std::unique_ptr<oio::api::blob::Listing> Build();

 private:
    std::shared_ptr<oio::kinetic::client::ClientFactory> factory;
    std::set<std::string> targets;
    std::string name;
};

}  // namespace blob
}  // namespace kinetic
}  // namespace oio

#endif  // SRC_OIO_KINETIC_CORO_BLOB_H_
