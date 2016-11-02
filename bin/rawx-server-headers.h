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

#ifndef BIN_RAWX_SERVER_HEADERS_H_
#define BIN_RAWX_SERVER_HEADERS_H_

#include <string>
#include "./common-server-headers.h"

#define OIO_RAWX_HEADER_PREFIX "X-oio-chunk-meta-"

class RawxHeader {
 public:
    enum Value {
        Unexpected,
        ContainerId,
        ContentId,
        ContentPath,
        ContentHash,
        ContentSize,
        ChunkId,
        ChunkHash,
        ChunkSize,
        ChunkPosition,
    };

 private:
    Value value;

 public:
    RawxHeader() : value{Unexpected} {}

    ~RawxHeader() {}

    void Parse(const std::string &k);

    inline bool Matched() const { return value != Value::Unexpected; }

    inline Value Get() const { return value; }

    std::string NetworkName() const;

    std::string StorageName() const;
};

#endif  // BIN_RAWX_SERVER_HEADERS_H_
