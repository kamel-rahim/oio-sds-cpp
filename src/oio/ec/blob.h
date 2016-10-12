/** Copyright (c) 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#ifndef OIO_KINETIC__OIO_EC__BLOB_H
#define OIO_KINETIC__OIO_EC__BLOB_H

#include <string>
#include <map>
#include <memory>
#include <oio/api/blob.h>
#include <iostream>
#include <set>
#include <sstream>



namespace oio {
namespace ec {
namespace blob {

struct rayxSet {
	std::string target ;
	int chunk_number ;
	int chunk_port ;

	bool operator<(const rayxSet& a) const { return chunk_number < a.chunk_number; }
	bool operator==(const rayxSet& a) const	{ return chunk_number == a.chunk_number; }
};

class UploadBuilder {
  public:
    UploadBuilder();

    ~UploadBuilder();

    bool Target(const rayxSet &to)   						{ targets.insert(to);  return true; } ;
    void SetXattr (const std::string &k, const std::string &v) 	{ xattrs[k] = v; } ;
    void BlockSize(uint32_t s)           { block_size = s; } ;
    void OffsetPos(int64_t s)            { offset_pos = s; } ;
    void M_Val (int s)                   { mVal = s; } ;
    void K_Val (int s)                   { kVal = s; } ;
    void NbChunks (int s)                { nbChunks = s; } ;

    std::unique_ptr<oio::api::blob::Upload> Build();

  private:
    std::set<rayxSet> targets;
    std::map<std::string,std::string> xattrs;
    uint32_t block_size;

    int kVal, mVal , nbChunks ;
    int64_t offset_pos ;
};

class DownloadBuilder {
  public:
    DownloadBuilder();

    ~DownloadBuilder();

    void BlockSize(uint32_t s)           { block_size = s; } ;
    bool Target(const rayxSet &to)   { targets.insert(to);  return true; } ;

  private:
    std::set<rayxSet> targets;
    uint32_t block_size;
};

class RemovalBuilder {
  public:
    RemovalBuilder();

    ~RemovalBuilder();

    void BlockSize(uint32_t s)           { block_size = s; } ;
    bool Target(const rayxSet &to)   { targets.insert(to);  return true; } ;

  private:
    std::set<rayxSet> targets;
    uint32_t block_size;
};

class ListingBuilder {
  public:
    ListingBuilder();

    ~ListingBuilder();

    void BlockSize(uint32_t s)           { block_size = s; } ;
    bool Target(const rayxSet &to)   { targets.insert(to);  return true; } ;

  private:
    std::set<rayxSet> targets;
    uint32_t block_size;
} ;

} // namespace rpc
} // namespace ec
} // namespace oio

#endif //OIO_KINETIC__OIO_EC__BLOB_H
