/**
 * Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/
 */

#ifndef SRC_OIO_KINETIC_CORO_BLOB_H_
#define SRC_OIO_KINETIC_CORO_BLOB_H_

#include <string>
#include <map>
#include <set>
#include <memory>

#include "oio/api/blob.h"
#include "oio/kinetic/coro/client/ClientInterface.h"

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
