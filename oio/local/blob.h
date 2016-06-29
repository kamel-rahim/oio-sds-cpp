/** Copyright (c) 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#ifndef OIO_KINETIC__OIO_LOCAL__BLOB_H
#define OIO_KINETIC__OIO_LOCAL__BLOB_H

#include <string>
#include <memory>
#include <oio/api/blob.h>

namespace oio {
namespace local {
namespace blob {

class UploadBuilder {
  public:
    UploadBuilder() noexcept;

    ~UploadBuilder() noexcept;

    void Path(const std::string &path) noexcept;

    std::unique_ptr<oio::api::blob::Upload> Build() noexcept;

  private:
    std::string path;
};

class DownloadBuilder {
  public:
    DownloadBuilder() noexcept;

    ~DownloadBuilder() noexcept;

    void Path(const std::string &path) noexcept;

    std::unique_ptr<oio::api::blob::Download> Build() noexcept;

  private:
    std::string path;
};

class RemovalBuilder {
  public:
    RemovalBuilder() noexcept;

    ~RemovalBuilder() noexcept;

    void Path(const std::string &path) noexcept;

    std::unique_ptr<oio::api::blob::Removal> Build() noexcept;

  private:
    std::string path;
};

class ListingBuilder {
  public:
    ListingBuilder() noexcept;

    ~ListingBuilder() noexcept;

    void Start(const std::string &path) noexcept;

    void End(const std::string &path) noexcept;

    std::unique_ptr<oio::api::blob::Listing> Build() noexcept;

  private:
    std::string start, end;
};

} // namespace rpc
} // namespace local
} // namespace oio

#endif //OIO_KINETIC__OIO_LOCAL__BLOB_H
