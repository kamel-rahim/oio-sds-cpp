/** Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#ifndef OIO_KINETIC_OIO_KINETIC_CORO_BLOB_H
#define OIO_KINETIC_OIO_KINETIC_CORO_BLOB_H

#include <string>
#include <map>
#include <set>
#include <memory>
#include <oio/api/blob.h>
#include <oio/kinetic/coro/client/ClientInterface.h>

namespace oio {
namespace kinetic {
namespace blob {

class UploadBuilder {
  public:
    UploadBuilder(
            std::shared_ptr<oio::kinetic::client::ClientFactory> f) noexcept;

    ~UploadBuilder() noexcept;

    void Target(const char *to) noexcept;

    void Target(const std::string &to) noexcept;

    void Name(const char *n) noexcept;

    void Name(const std::string &n) noexcept;

    void BlockSize(uint32_t s) noexcept;

    std::unique_ptr<oio::api::blob::Upload> Build() noexcept;

  private:
    std::shared_ptr<oio::kinetic::client::ClientFactory> factory;
    std::set<std::string> targets;
    std::string name;
    uint32_t block_size;
};

class DownloadBuilder {
  public:
    DownloadBuilder(
            std::shared_ptr<oio::kinetic::client::ClientFactory> f) noexcept;

    ~DownloadBuilder() noexcept;

    void Name(const char *n) noexcept;

    void Name(const std::string &n) noexcept;

    void Target(const char *to) noexcept;

    void Target(const std::string &to) noexcept;

    std::unique_ptr<oio::api::blob::Download> Build() noexcept;

  private:
    std::string name;
    std::set<std::string> targets;
    std::shared_ptr<oio::kinetic::client::ClientFactory> factory;
};

class RemovalBuilder {
  public:
    RemovalBuilder(
            std::shared_ptr<oio::kinetic::client::ClientFactory> f) noexcept;

    void Name(const std::string &n) noexcept;

    void Name(const char *n) noexcept;

    void Target(const std::string &to) noexcept;

    void Target(const char *to) noexcept;

    std::unique_ptr<oio::api::blob::Removal> Build() noexcept;

  private:
    std::shared_ptr<oio::kinetic::client::ClientFactory> factory;
    std::set<std::string> targets;
    std::string name;
};

class ListingBuilder {
  public:
    ListingBuilder(
            std::shared_ptr<oio::kinetic::client::ClientFactory> f) noexcept;

    ~ListingBuilder() noexcept;

    void Name(const std::string &n) noexcept;

    void Name(const char *n) noexcept;

    void Target(const std::string &to) noexcept;

    void Target(const char *to) noexcept;

    std::unique_ptr<oio::api::blob::Listing> Build() noexcept;

  private:
    std::shared_ptr<oio::kinetic::client::ClientFactory> factory;
    std::set<std::string> targets;
    std::string name;
};

} // namespace rpc
} // namespace kinetic
} // namespace oio

#endif //OIO_KINETIC_OIO_KINETIC_CORO_BLOB_H
