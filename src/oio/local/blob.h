/**
 * Copyright (c) 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/
 */

#ifndef SRC_OIO_LOCAL_BLOB_H_
#define SRC_OIO_LOCAL_BLOB_H_

#include <string>
#include <memory>

#include "oio/api/blob.h"

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

    /**
     * The upload internally works with a temporary file, then the method
     * returns the path. For testing purposes.
     * @return the path to the pending file
     */
    std::string PathPending() const;

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

}  // namespace blob
}  // namespace local
}  // namespace oio

#endif  // SRC_OIO_LOCAL_BLOB_H_
