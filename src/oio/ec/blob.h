/**
 * Copyright (c) 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/
 */

#ifndef SRC_OIO_EC_BLOB_H_
#define SRC_OIO_EC_BLOB_H_

#include <oio/api/blob.h>

#include <string>
#include <map>
#include <memory>
#include <iostream>
#include <set>
#include <sstream>
#include "utils/net.h"


namespace oio {
namespace ec {
namespace blob {

#define MAX_DELAY 5000

struct frag_array_set {
    unsigned int num_fragments;
    char **array;

    frag_array_set() : num_fragments{0}, array{nullptr} {}
};

class SocketElement {

public:
	void Close () {
		if (_bopen) {
			_socket->close () ;
			_bopen = false ;
		}
	}

	void ReconnectSocket () {
		if (is_open())
			_bopen = _socket->Reconnect() ;
	}

	std::shared_ptr<net::Socket> *GetSocket (std::string host) {
		if (!is_open()) {
			_socket.reset(new net::MillSocket);
           _bopen = _socket->connect(host) ;
		}
		else
		{
			if (mill_now() > timer )
				_bopen = _socket->Reconnect() ;
		}

		timer = mill_now() + MAX_DELAY ;
		if (is_open())
			return &_socket ;
		else return NULL ;
	}

	bool is_open() { return _bopen; }

private:
	std::shared_ptr<net::Socket> _socket;
	bool _bopen ;
	int64_t timer ;
};

class SocketMap {

public:
	~SocketMap () {
		for (const auto &e : _SocketMap) {
			SocketElement elm = e.second ;
			elm.Close() ;
		}
	}

	std::shared_ptr<net::Socket> *GetSocket (std::string host) {
		return _SocketMap [host].GetSocket (host);
	}

private:
	std::map<std::string, SocketElement> _SocketMap;
};


struct rawxSet {
    std::string filename;
    std::string host;
    std::string chunk_str;
    int chunk_number;
    int chunk_port;

    bool operator<(const rawxSet &a) const {
        return chunk_number < a.chunk_number;
    }

    bool operator==(const rawxSet &a) const {
        return chunk_number == a.chunk_number;
    }
};

class UploadBuilder {
 public:
    UploadBuilder();

    ~UploadBuilder();

    bool Target(const rawxSet &to) {
        targets.insert(to);
        return true;
    }

    void SetXattr(const std::string &k, const std::string &v) {
        xattrs[k] = v;
    }

    void BlockSize(uint32_t s) { block_size = s; }

    void OffsetPos(int64_t s) { offset_pos = s; }

    void M_Val(int s) { mVal = s; }

    void Req_id(const std::string &s) { req_id = s; }

    void K_Val(int s) { kVal = s; }

    void NbChunks(int s) { nbChunks = s; }

    void Encoding_Method(int s) { EncodingMethod = s; }

    std::unique_ptr<oio::api::blob::Upload> Build();

 private:
    std::set<rawxSet> targets;
    std::map<std::string, std::string> xattrs;
    uint32_t block_size;
    std::string req_id;

    int kVal, mVal, nbChunks, EncodingMethod;
    int64_t offset_pos;
};

class DownloadBuilder {
 public:
    DownloadBuilder();

    ~DownloadBuilder();

    bool Target(const rawxSet &to) {
        targets.insert(to);
        return true;
    }

    void SetXattr(const std::string &k, const std::string &v) {
        xattrs[k] = v;
    }

    void ChunkSize(int64_t s) { chunkSize = s; }

    void M_Val(int s) { mVal = s; }

    void K_Val(int s) { kVal = s; }

    void NbChunks(int s) { nbChunks = s; }

    void Encoding_Method(int s) { EncodingMethod = s; }

    void Req_id(const std::string &s) { req_id = s; }

    void Offset(uint64_t s) { offset = s; }

    void SizeExpected(uint64_t s) { size_expected = s; }

    std::unique_ptr<oio::api::blob::Download> Build();

 private:
    std::set<rawxSet> targets;
    std::map<std::string, std::string> xattrs;
    int kVal, mVal, nbChunks, EncodingMethod;
    std::string req_id;
    int64_t chunkSize;
    uint64_t offset, size_expected;
};

class RemovalBuilder {
 public:
    RemovalBuilder();

    ~RemovalBuilder();

    inline void BlockSize(uint32_t s) { block_size = s; }

    inline bool Target(const rawxSet &to) {
        targets.insert(to);
        return true;
    }

 private:
    std::set<rawxSet> targets;
    uint32_t block_size;
};

class ListingBuilder {
 public:
    ListingBuilder();

    ~ListingBuilder();

    inline void BlockSize(uint32_t s) { block_size = s; }

    inline bool Target(const rawxSet &to) {
        targets.insert(to);
        return true;
    }

 private:
    std::set<rawxSet> targets;
    uint32_t block_size;
};

}  // namespace blob
}  // namespace ec
}  // namespace oio

#endif  // SRC_OIO_EC_BLOB_H_
