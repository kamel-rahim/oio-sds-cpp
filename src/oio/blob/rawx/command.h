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

namespace oio {
namespace blob {
namespace rawx {

class Range {
 private:
    uint64_t start;
    uint64_t size;

 public:
    ~Range() { Clear(); }

    Range() : start{0}, size{0} {}

    Range(uint64_t _start, uint64_t _size) : start{_start}, size{_size} {}

    Range(const Range &r) : start{r.start}, size{r.size} {}

    void Set(const Range &arg) { start = arg.start, size = arg.size; }

    void Clear() { start = 0, size = 0; }

    uint64_t Start() const { return start; }

    uint64_t Size() const { return size; }
};

class RawxUrl {
    friend class RawxCommand;

    friend class EcCommand;

 private:
    std::string chunk_id;
    std::string host;
    int port;

 public:
    ~RawxUrl() { Clear(); }

    RawxUrl() : chunk_id(), host(), port{0} {}

    explicit RawxUrl(const std::string &v) : RawxUrl() { Parse(v); }

    RawxUrl(const RawxUrl &o) : chunk_id(o.chunk_id), host{o.host},
                                port{o.port} {}

    void Set(const RawxUrl &u) {
        chunk_id.assign(u.chunk_id);
        host.assign(u.host);
        port = u.port;
    }

    void Parse(const std::string &v) {
        std::size_t pos = v.find("http");
        // parse: http://IP:port/chink_id
        if (pos != std::string::npos) {
            pos = v.find(":");
            if (pos != std::string::npos) {
                std::size_t pos2 = v.find(":", pos + 1);  // we need second one

                if (pos2 != std::string::npos) {
                    std::string str = v.substr(pos + 3, pos2 - pos - 3);
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

    void Clear() {
        chunk_id.clear();
        host.clear();
        port = 0;
    }


    std::string Host_Port() const {
        std::stringstream ss;
        ss << host << ":" << port;
        std::string str = ss.str();
        return str;
    }

    std::string ChunkId() const { return chunk_id; }

    std::string Host() const { return host; }

    int Port() const { return port; }

    std::string GetString() const {
        std::stringstream ss;
        ss << "http://" << host << ":" << port << "/" << chunk_id;
        std::string str = ss.str();
        return str;
    }

    RawxUrl &GetUrl() { return *this; }
};

struct RawxUrlSet : public RawxUrl {
    int chunk_number;

    RawxUrlSet() {}

    bool operator<(const RawxUrlSet &a) const {
        return chunk_number < a.chunk_number;
    }

    void Set(const RawxUrlSet &arg) {
        RawxUrl::Set(arg);
        chunk_number = arg.chunk_number;
    }

    void Set(const RawxUrl &arg) { RawxUrl::Set(arg); }
};

class RawxCommand {
 private:
    RawxUrl url;
    Range range;
    uint32_t chunkSize;

 public:
    void Clear() {
        url.Clear();
        range.Clear();
        chunkSize = 0;
    }

    RawxCommand() : url(), range(), chunkSize{0} {}

    // Getters

    uint32_t ChunkSize() const { return chunkSize; }

    const RawxUrl &Url() const { return url; }

    const Range &GetRange() const { return range; }

    // Setters

    void SetChunkSize(uint32_t v) { chunkSize = v; }

    void SetUrl(const RawxUrl &arg) { url.Set(arg); }

    void SetRange(const Range &arg) { range.Set(arg); }
};

}  // namespace rawx
}  // namespace blob
}  // namespace oio

#endif  // SRC_OIO_BLOB_RAWX_COMMAND_H_
