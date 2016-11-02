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

    void RawxId(const std::string &s);

    void ChunkId(const std::string &s);

    std::unique_ptr<oio::api::blob::Download> Build(
            std::shared_ptr<net::Socket> socket);

 private:
    oio::http::imperative::DownloadBuilder inner;
};

class UploadBuilder {
 public:
    UploadBuilder();

    ~UploadBuilder();

    void RawxId(const std::string &s);

    void ChunkId(const std::string &s);

    void ChunkPosition(int64_t meta, int64_t sub);

    void ContainerId(const std::string &s);

    void ContentPath(const std::string &s);

    void ContentId(const std::string &s);

    void MimeType(const std::string &s);

    void ContentVersion(int64_t v);

    void StoragePolicy(const std::string &s);

    void ChunkMethod(const std::string &s);

    void Property(const std::string &k, const std::string &v);

    std::unique_ptr<oio::api::blob::Upload> Build(
            std::shared_ptr<net::Socket> socket);

 private:
    oio::http::imperative::UploadBuilder inner;
};

class RemovalBuilder {
 public:
    RemovalBuilder();

    ~RemovalBuilder();

    void RawxId(const std::string &s);

    void ChunkId(const std::string &s);

    std::unique_ptr<oio::api::blob::Removal> Build(
            std::shared_ptr<net::Socket> socket);

 private:
    oio::http::imperative::RemovalBuilder inner;
};


}  // namespace blob
}  // namespace rawx
}  // namespace oio

#endif  // SRC_OIO_RAWX_BLOB_H_
