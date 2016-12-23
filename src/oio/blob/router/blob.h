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

#ifndef SRC_OIO_BLOB_ROUTER_BLOB_H_
#define SRC_OIO_BLOB_ROUTER_BLOB_H_

#include <oio/api/blob.h>

#include <string>
#include <map>
#include <memory>
#include <iostream>
#include <set>
#include <sstream>
#include "utils/net.h"

#include "utils/serialize_def.h"
#include "oio/blob/rawx/command.h"
#include "oio/blob/ec/command.h"

#define MAX_DELAY 5000

enum ENCODING_TYPE { ec, rawx };

namespace oio {
namespace router {
namespace blob {

class UploadBuilder {
 public:
    UploadBuilder();

    ~UploadBuilder();

    void set_ec_param(const EcCommand &_param) {
        ec_param = _param;
        type = ec;
    }

    void set_rawx_param(const RawxCommand &_param) {
        rawx_param = _param;
        type = rawx;
    }

    void SetXattr(const std::string &k, const std::string &v) {
        xattrs[k] = v;
    }

    std::unique_ptr<oio::api::blob::Upload> Build();

 private:
    std::map<std::string, std::string> xattrs;
    EcCommand   ec_param;
    RawxCommand rawx_param;
    ENCODING_TYPE type;
};

class DownloadBuilder {
 public:
    DownloadBuilder();

    ~DownloadBuilder();


    void set_ec_param(const EcCommand &_param) {
        ec_param = _param;
        type = ec;
    }

    void set_rawx_param(const RawxCommand &_param) {
        rawx_param = _param;
        type = rawx;
    }

    void SetXattr(const std::string &k, const std::string &v) {
        xattrs[k] = v;
    }

    std::unique_ptr<oio::api::blob::Download> Build();

 private:
    std::map<std::string, std::string> xattrs;
    EcCommand   ec_param;
    RawxCommand rawx_param;
    ENCODING_TYPE type;
};

class RemovalBuilder {
 public:
    RemovalBuilder();

    ~RemovalBuilder();

    inline void BlockSize(uint32_t s) { block_size = s; }

    inline bool Target(const RawxUrlSet &to) {
        targets.insert(to);
        return true;
    }

 private:
    std::set<RawxUrlSet> targets;
    uint32_t block_size;
};

class ListingBuilder {
 public:
    ListingBuilder();

    ~ListingBuilder();

    inline void BlockSize(uint32_t s) { block_size = s; }

    inline bool Target(const RawxUrlSet &to) {
        targets.insert(to);
        return true;
    }

 private:
    std::set<RawxUrlSet> targets;
    uint32_t block_size;
};

}  // namespace blob
}  // namespace router
}  // namespace oio

#endif  // SRC_OIO_BLOB_ROUTER_BLOB_H_
