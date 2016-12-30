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

#ifndef SRC_OIO_BLOB_LOCAL_BLOB_HPP_
#define SRC_OIO_BLOB_LOCAL_BLOB_HPP_

#include <string>
#include <memory>

#include "oio/api/blob.hpp"

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

}  // namespace blob
}  // namespace local
}  // namespace oio

#endif  // SRC_OIO_BLOB_LOCAL_BLOB_HPP_
