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

#ifndef SRC_OIO_BLOB_EC_COMMAND_H_
#define SRC_OIO_BLOB_EC_COMMAND_H_

#include <set>
#include <string>

class EcCommand : public Range {
 public:
    std::set<RawxUrlSet> targets;
    std::string req_id;
    int kVal, mVal, nbChunks, EncodingMethod;
    uint32_t ChunkSize;

 public:
    void Clear() {
        Range::Clear();
        targets.clear();
        req_id = "";
        kVal = mVal = nbChunks = EncodingMethod = ChunkSize = 0;
    }

    EcCommand& operator=(const EcCommand& arg) {
        Range::operator =(arg);
        for (const auto &to : arg.targets)
          targets.insert(to);

        req_id            = arg.req_id;
        kVal              = arg.kVal;
        mVal              = arg.mVal;
        nbChunks          = arg.nbChunks;
        EncodingMethod    = arg.EncodingMethod;
        ChunkSize         = arg.ChunkSize;
        return *this;
    }

    Range GetRange() {
        return Range(start, size);
    }
};

#endif  // SRC_OIO_BLOB_EC_COMMAND_H_
