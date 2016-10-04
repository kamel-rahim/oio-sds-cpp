/** Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#ifndef OIO_KINETIC_PROXY__HEADERS_H
#define OIO_KINETIC_PROXY__HEADERS_H 1

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
	KineticHeader(): value{Value::Unexpected} {}
	~KineticHeader() {}

    void Parse(const std::string &k);
    inline bool Matched() const { return value == Value::Unexpected; }
    inline Value Get() const { return value; }
	std::string Name() const;
};

#endif