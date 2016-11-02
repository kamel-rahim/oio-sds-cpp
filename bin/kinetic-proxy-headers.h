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
