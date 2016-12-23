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

#ifndef SRC_OIO_BLOB_RAWX_COMMAND_H_
#define SRC_OIO_BLOB_RAWX_COMMAND_H_

#include <string>
#include <iostream>
#include <sstream>


class _range {
 public:
    uint64_t start;
    uint64_t size;

 public:
    _range()  {  }
    _range(uint64_t _start, uint64_t _size ) {
         start = _start;
         size = _size;
    }

    _range& operator=(const _range& arg) {
       start = arg.start;
       size  = arg.size;
       return *this;
    }

    void Clear() {
        start =
        size  = 0;
    }

    _range& Range() { return *this; }
};

class _rawx {
    friend class rawx_cmd;
    friend class ec_cmd;
 private:
    std::string chunk_id;
    std::string host;
    int port;

 public:
    _rawx() { Clear(); }

    void Clear() {
        chunk_id = host = "";
        port = 0;
    }

    explicit _rawx(std::string v) {
        std::size_t pos = v.find("http");
        // parse: http://IP:port/chink_id
        if (pos != std::string::npos) {
            pos = v.find(":");
            if (pos != std::string::npos) {
                std::size_t pos2 = v.find(":", pos + 1);  // we need second one

                if (pos2 != std::string::npos) {
                    std::string str = v.substr(pos + 3, pos2 - pos -3);
                    host = str;

                    str = v.substr(pos2 + 1, 4);
                    std::stringstream ss2(str);
                    ss2 >> port;

                    str = str = v.substr(pos2 + 6);
                    chunk_id = str;
                }
            }
        } else {
            // parse IP:port
            std::size_t pos = v.find(":");
            if (pos != std::string::npos) {
                std::string str = v.substr(0, pos);
                host = str;

                str = v.substr(pos + 1, 4);
                std::stringstream ss2(str);
                ss2 >> port;
            }
        }
    }

    std::string Host_Port() const {
        std::stringstream ss;
        ss << host << ":" << port;
        std::string str = ss.str();
        return str;
    }

    std::string & ChunkId() { return chunk_id; }
    int Port () { return port; }

    std::string GetString() {
        std::stringstream ss;
        ss << "http://" << host << ":" << port << "/" << chunk_id;
        std::string str = ss.str();
        return str;
    }

    _rawx& operator=(const _rawx& arg) {
        chunk_id     = arg.chunk_id;
        host         = arg.host;
        port         = arg.port;
        return *this;
    }

    _rawx& Rawx() { return *this; }
};

struct rawxSet : public _rawx {
    int chunk_number;

    rawxSet() {}

    bool operator<(const rawxSet &a) const {
        return chunk_number < a.chunk_number;
    }

    bool operator==(const rawxSet &a) const {
        return chunk_number == a.chunk_number;
    }

    rawxSet& operator=(const _rawx& arg) {
        _rawx::operator =(arg);
       return *this;
    }
};

class rawx_cmd : public _rawx, public _range {
 public:
     uint32_t ChunkSize;

 public:
    void Clear() {
        _rawx::Clear();
        _range::Clear();
        ChunkSize = 0;
    }

    rawx_cmd& operator=(const rawx_cmd& arg) {
        _rawx::operator =(arg);
        _range::operator =(arg);
        ChunkSize = arg.ChunkSize;
        return *this;
    }

    rawx_cmd& operator=(const _rawx& arg) {
        _rawx::operator =(arg);
        return *this;
    }

    rawx_cmd& operator=(const rawxSet& arg) {
        _rawx::operator =(arg);
        return *this;
    }

    rawx_cmd& operator=(const _range& arg) {
        _range::operator =(arg);
        return *this;
    }
};

#endif  // SRC_OIO_BLOB_RAWX_COMMAND_H_
