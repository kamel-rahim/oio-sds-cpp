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
    UploadBuilder();

    ~UploadBuilder();

    /**
     * Mandatory
     * @param path
     */
    void Path(const std::string &path);

    /**
     * Optional
     * @param mode
     */
    void FileMode(unsigned int mode);

    /**
     * Optional
     * @param mode
     */
    void DirMode(unsigned int mode);

    std::unique_ptr<oio::api::blob::Upload> Build();

  private:
    std::string path;
    unsigned int fmode, dmode;
};

class DownloadBuilder {
  public:
    DownloadBuilder();

    ~DownloadBuilder();

    void Path(const std::string &path);

    std::unique_ptr<oio::api::blob::Download> Build();

  private:
    std::string path;
};

class RemovalBuilder {
  public:
    RemovalBuilder();

    ~RemovalBuilder();

    void Path(const std::string &path);

    std::unique_ptr<oio::api::blob::Removal> Build();

  private:
    std::string path;
};

class ListingBuilder {
  public:
    ListingBuilder();

    ~ListingBuilder();

    void Start(const std::string &path);

    void End(const std::string &path);

    std::unique_ptr<oio::api::blob::Listing> Build();

  private:
    std::string start, end;
};

} // namespace rpc
} // namespace local
} // namespace oio

#endif //OIO_KINETIC__OIO_LOCAL__BLOB_H
