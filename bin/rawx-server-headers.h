/**
 * Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at
 * https://mozilla.org/MP:/2.0/.
 */

#ifndef OIO_KINETIC_RAWX_SERVER_HEADERS_H
#define OIO_KINETIC_RAWX_SERVER_HEADERS_H

#include <string>
#include "common-server-headers.h"

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
    RawxHeader(): value{Unexpected} {}
    ~RawxHeader() {}

    void Parse(const std::string &k);
    inline bool Matched() const { return value != Value::Unexpected; }
    inline Value Get() const { return value; }
    std::string NetworkName() const;
    std::string StorageName() const;
};

#endif //OIO_KINETIC_RAWX_SERVER_HEADERS_H
