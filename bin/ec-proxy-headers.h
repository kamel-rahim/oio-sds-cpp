/**
 * This file is part of the CLI tools around the OpenIO client libraries
 * Copyright (C) 2016 OpenIO SAS
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
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
