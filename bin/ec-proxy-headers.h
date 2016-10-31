/** 
 * Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ 
*/

#ifndef BIN_EC_PROXY_HEADERS_H_
#define BIN_EC_PROXY_HEADERS_H_

#include <string>

#include "./common-server-headers.h"

#define OIO_HEADER_EC_PREFIX "X-oio-chunk-meta-"

class EcHeader {
 public:
    enum Value {
        Unexpected,
        ReqId,
        ContainerId,
        ContentPath,
        ContentVersion,
        ContentId,
        StoragePolicy,
        ChunkMethod,
        MineType,
        ChunkDest,
        Chunks,
        ChunkPos,
        ChunkSize,
        Range,
        TransferEncoding,
    };

 private:
    Value value;

 public:
    inline EcHeader() : value{Value::Unexpected} {}

    inline ~EcHeader() {}

    inline bool Matched() const { return value != Value::Unexpected; }

    inline Value Get() const { return value; }

    void Parse(const std::string &k);

    std::string Name() const;
};

#endif  // BIN_EC_PROXY_HEADERS_H_
