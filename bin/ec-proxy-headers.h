/** Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#ifndef OIO_KINETIC_PROXY__HEADERS_H
#define OIO_KINETIC_PROXY__HEADERS_H 1

#include <string>

#define OIO_HEADER_EC_PREFIX "X-oio-"

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
        TransferEncoding,
    };
  private:
	Value value;
  public:
	EcHeader(): value{Value::Unexpected} {}
	~EcHeader() {}

    void Parse(const std::string &k);
    inline bool Matched() const { return value != Value::Unexpected; }
    inline Value Get() const { return value; }
	std::string Name() const;
};

#endif
