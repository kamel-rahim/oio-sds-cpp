/**
 * Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/
 */

#ifndef BIN_KINETIC_PROXY_HEADERS_H_
#define BIN_KINETIC_PROXY_HEADERS_H_

#include <string>

#define OIO_HEADER_KINETIC_PREFIX "X-oio-meta-"

class KineticHeader {
 public:
    enum Value {
        Unexpected,
        Target,
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
    KineticHeader() : value{Value::Unexpected} {}

    ~KineticHeader() {}

    inline bool Matched() const { return value == Value::Unexpected; }

    inline Value Get() const { return value; }

    void Parse(const std::string &k);

    std::string Name() const;
};

#endif  // BIN_KINETIC_PROXY_HEADERS_H_
