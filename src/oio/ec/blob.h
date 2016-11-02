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

#define MAX_DELAY 5000

namespace oio {
namespace ec {
namespace blob {


struct frag_array_set {
    unsigned int num_fragments;
    char **array;

    frag_array_set() : num_fragments{0}, array{nullptr} {}
};


class SocketElement {
 public:
    void Close() {
        if (_bopen) {
            _socket->close();
            _bopen = false;
        }
    }

    void ReconnectSocket() {
        if (is_open())
            _bopen = _socket->Reconnect();
    }

    std::shared_ptr<net::Socket> *GetSocket(std::string host) {
        if (!is_open()) {
            _socket.reset(new net::MillSocket);
            _bopen = _socket->connect(host);
        } else {
            if (mill_now() > timer)
                _bopen = _socket->Reconnect();
        }

        timer = mill_now() + MAX_DELAY;
        if (is_open())
            return &_socket;
        else
            return NULL;
    }

    bool is_open() { return _bopen; }

 private:
    std::shared_ptr<net::Socket> _socket;
    bool _bopen;
    int64_t timer;
};


class SocketMap {
 public:
    ~SocketMap() {
        for (const auto &e : _SocketMap) {
            SocketElement elm = e.second;
            elm.Close();
        }
    }

    std::shared_ptr<net::Socket> *GetSocket(std::string host) {
        return _SocketMap[host].GetSocket(host);
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
