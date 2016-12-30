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

namespace oio {
namespace blob {
namespace ec {

class EcCommand {
 public:
    using SetOfTargets = std::set<::oio::blob::rawx::RawxUrlSet>;
 public:
    ~EcCommand() {}

    EcCommand() : range(), targets(), req_id(), kVal{0}, mVal{0},
                  encodingMethod{0}, chunkSize{0} {}

    void Clear() {
        range.Clear();
        targets.clear();
        req_id.clear();
        kVal = mVal = nbChunks = encodingMethod = 0;
        chunkSize = 0;
    }

    // Getters

    int K() const { return kVal; }

    int M() const { return mVal; }

    int Encoding() const { return encodingMethod; }

    uint32_t ChunkSize() const { return chunkSize; }

    const ::oio::blob::rawx::Range& GetRange() const { return range; }

    ::oio::blob::rawx::Range& GetRange() { return range; }

    const SetOfTargets &Targets() const { return targets; }

    // Setters

    void SetK(int val) { kVal = val; }

    void SetM(int val) { mVal = val; }

    void SetNbChunks(int val) { nbChunks = val; }

    void SetChunkSize(uint32_t val) { chunkSize = val; }

    void SetReqId(std::string v) { req_id.assign(v); }

    void SetEncoding(int m) { encodingMethod = m; }

    void AddTarget(::oio::blob::rawx::RawxUrlSet us) { targets.insert(us); }

    void Set(const EcCommand &arg) {
        range.Clear();
        range.Set(arg.range);

        targets.clear();
        for (const auto &to : arg.targets)
            targets.insert(to);

        req_id = arg.req_id;
        kVal = arg.kVal;
        mVal = arg.mVal;
        nbChunks = arg.nbChunks;
        encodingMethod = arg.encodingMethod;
        chunkSize = arg.chunkSize;
    }

 private:
    ::oio::blob::rawx::Range range;
    SetOfTargets targets;
    std::string req_id;
    int kVal, mVal, nbChunks, encodingMethod;
    uint32_t chunkSize;
};

}  // namespace ec
}  // namespace blob
}  // namespace oio

#endif  // SRC_OIO_BLOB_EC_COMMAND_H_
