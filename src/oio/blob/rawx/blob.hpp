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

#ifndef SRC_OIO_BLOB_RAWX_BLOB_HPP_
#define SRC_OIO_BLOB_RAWX_BLOB_HPP_

#include <string>
#include <memory>
#include <map>

#include "oio/api/blob.hpp"
#include "oio/blob/http/blob.hpp"

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

class DownloadBuilder {
 public:
    DownloadBuilder();

    ~DownloadBuilder();

    void set_param(const RawxCommand &_param);

    std::unique_ptr<oio::api::blob::Download> Build(
            std::shared_ptr<net::Socket> socket);

 private:
    oio::http::imperative::DownloadBuilder inner;
    RawxCommand rawx_param;
};

class UploadBuilder {
 public:
    UploadBuilder();

    ~UploadBuilder();

    void set_param(const RawxCommand &_param);

    void ContainerId(const std::string &s);

    void ContentPath(const std::string &s);

    void ContentId(const std::string &s);

    void MimeType(const std::string &s);

    void ContentVersion(int64_t v);

    void StoragePolicy(const std::string &s);

    void ChunkMethod(const std::string &s);

    void Property(const std::string &k, const std::string &v);

    std::unique_ptr<oio::api::blob::Upload> Build(
            std::shared_ptr<net::Socket> socket);

 private:
    oio::http::imperative::UploadBuilder inner;
    RawxCommand rawx_param;
};

class RemovalBuilder {
 public:
    RemovalBuilder();

    ~RemovalBuilder();

    void set_param(const RawxCommand &_param);

    std::unique_ptr<oio::api::blob::Removal> Build(
            std::shared_ptr<net::Socket> socket);

 private:
    oio::http::imperative::RemovalBuilder inner;
    RawxCommand rawx_param;
};

}  // namespace rawx
}  // namespace blob
}  // namespace oio

#endif  // SRC_OIO_BLOB_RAWX_BLOB_HPP_
