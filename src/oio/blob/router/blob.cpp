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

#include "oio/blob/router/blob.h"

#include <fcntl.h>
#include <libmill.h>
#include <glog/logging.h>

#include <algorithm>
#include <cassert>
#include <iostream>
#include <map>
#include <vector>

#include "utils/utils.h"
#include "oio/blob/ec/blob.h"
#include "oio/blob/rawx/blob.h"
#include "oio/blob/http/socket_map.h"

using oio::api::Status;
using oio::api::Errno;
using oio::api::Cause;
using oio::router::blob::DownloadBuilder;
using oio::router::blob::RemovalBuilder;
using oio::router::blob::UploadBuilder;

namespace blob = ::oio::api::blob;

SocketMap TheScoketMap;

class RouterDownload : public oio::api::blob::Download {
    friend class DownloadBuilder;

 public:
    void set_ec_param(const EcCommand &_param) {
        ec_param = _param;
        type = ec;
    }

    void set_rawx_param(const RawxCommand &_param) {
        rawx_param = _param;
        type = rawx;
    }

    void SetXattr(const std::string &k, const std::string &v) {
        xattr[k] = v;
        DLOG(INFO) << "Received xattr [" << k << "]";
    }

    ~RouterDownload() override { DLOG(INFO) << __FUNCTION__; }

    Status Prepare() override {
        done = false;
        bool bOk = false;

        if (type == ec) {
            oio::ec::blob::DownloadBuilder builder;
            builder.set_param(ec_param);

            auto dl = builder.Build();
            auto rc = dl->Prepare();
            if (rc.Ok()) {
                while (!dl->IsEof()) {
                    dl->Read(&buffer);
                }
                bOk = true;
            }
        } else {
            // read from rawx
            std::shared_ptr<net::Socket> *socket = TheScoketMap.GetSocket(
                    rawx_param.Host_Port());
            if (socket) {
                oio::rawx::blob::DownloadBuilder builder;

                builder.set_param(rawx_param);

                auto dl = builder.Build(*socket);
                auto rc = dl->Prepare();

                if (rc.Ok()) {
                    while (!dl->IsEof()) {
                        dl->Read(&buffer);
                    }
                    bOk = true;
                }
            } else {
                LOG(ERROR) << "Router: failed to connect to rawx port: "
                           << rawx_param.Port();
            }
        }

        CleanUp();

        if (bOk)
            return Status(Cause::OK);
        else
            return Status(Cause::InternalError);
    }

    bool IsEof() override { return done; }

    int32_t Read(std::vector<uint8_t> *buf) override {
        assert(buf != nullptr);
        if (buffer.size() <= 0) {
            return 0;
        }

        buf->swap(buffer);
        buffer.resize(0);
        done = true;
        return buf->size();
    }

    void CleanUp() {
    }

 private:
    FORBID_MOVE_CTOR(RouterDownload);
    FORBID_COPY_CTOR(RouterDownload);

    RouterDownload() {}

 private:
    std::vector<uint8_t> buffer;
    std::map<std::string, std::string> xattr;
    EcCommand   ec_param;
    RawxCommand rawx_param;
    ENCODING_TYPE type;
    bool done;
};

DownloadBuilder::DownloadBuilder() {}

DownloadBuilder::~DownloadBuilder() {}

std::unique_ptr<blob::Download> DownloadBuilder::Build() {
    auto ul = new RouterDownload();
    if (type == ENCODING_TYPE::ec)
        ul->set_ec_param(ec_param);
    else
        ul->set_rawx_param(rawx_param);
    for (const auto &e : xattrs)
        ul->SetXattr(e.first, e.second);
    return std::unique_ptr<RouterDownload>(ul);
}


class RouterRemoval : public oio::api::blob::Removal {
    friend class RemovalBuilder;

 public:
    RouterRemoval() {}

    ~RouterRemoval() {}

    Status Prepare() override { return Status(Cause::OK); }

    Status Commit() override { return Status(); }

    Status Abort() override { return Status(); }
};

RemovalBuilder::RemovalBuilder() {}

RemovalBuilder::~RemovalBuilder() {}


class RouterUpload : public oio::api::blob::Upload {
    friend class UploadBuilder;

 public:
    void set_ec_param(const EcCommand &_param) {
        ec_param = _param;
        ChunkSize = ec_param.ChunkSize;
        type = ec;
    }

    void set_rawx_param(const RawxCommand &_param) {
        rawx_param = _param;
        ChunkSize = rawx_param.ChunkSize;
        type = rawx;
    }

    void SetXattr(const std::string &k, const std::string &v) override {
        xattrs[k] = v;
        DLOG(INFO) << "Received xattr [" << k << "]";
    }

    Status Prepare() {
        return Status(Cause::OK);
    }

    Status Commit() override {
        if (type == ec) {
            oio::ec::blob::UploadBuilder builder;
            builder.set_param(ec_param);

            auto ul = builder.Build();
            for (const auto &e : xattrs)
                ul->SetXattr(e.first, e.second);
            auto rc = ul->Prepare();
            if (rc.Ok()) {
                ul->Write(buffer.data(), buffer.size());
                ul->Commit();
            } else {
                ul->Abort();
            }
        } else {
            // write to Rawx
            std::shared_ptr<net::Socket> *socket = TheScoketMap.GetSocket(
                    rawx_param.Host_Port());
            if (socket) {
                oio::rawx::blob::UploadBuilder builder;

                builder.set_param(rawx_param);

                builder.ContainerId(xattrs.find("container-id")->second);
                builder.ContentPath(xattrs.find("content-path")->second);
                builder.ContentId(xattrs.find("content-id")->second);
                int64_t v;
                std::istringstream(xattrs.find("content-version")->second) >> v;
                builder.ContentVersion(v);
                builder.StoragePolicy("SINGLE");
                builder.MimeType(xattrs.find("content-mime-type")->second);
                builder.ChunkMethod("plain/nb_copy=1");
                auto ul = builder.Build(*socket);
                auto rc = ul->Prepare();
                if (rc.Ok()) {
                    ul->Write(buffer.data(), buffer.size());
                    ul->Commit();
                } else {
                    ul->Abort();
                }
            } else {
                LOG(ERROR) << "Router: failed to connect to rawx port: "
                           << rawx_param.Port();
            }
        }

        CleanUp();

        return Status(Cause::OK);
    }

    void CleanUp() { }

    Status Abort() override {
        CleanUp();
        return Status(Cause::OK);
    }

    void Write(const uint8_t *buf, uint32_t len) override {
        while (len > 0) {
            const auto oldsize = buffer.size();
            const uint32_t avail = ChunkSize - oldsize;
            const uint32_t local = std::min(avail, len);
            if (local > 0) {
                buffer.resize(oldsize + local);
                memcpy(buffer.data() + oldsize, buf, local);
                buf += local;
                len -= local;
            }
        }
        yield();
    }

    ~RouterUpload() {}

 private:
    FORBID_COPY_CTOR(RouterUpload);

    FORBID_MOVE_CTOR(RouterUpload);

    RouterUpload() {}

 private:
    std::vector<uint8_t> buffer;
    std::map<std::string, std::string> xattrs;
    EcCommand   ec_param;
    RawxCommand rawx_param;
    uint32_t ChunkSize;
    ENCODING_TYPE type;
};

UploadBuilder::UploadBuilder() {}

UploadBuilder::~UploadBuilder() {}

std::unique_ptr<blob::Upload> UploadBuilder::Build() {
    auto ul = new RouterUpload();

    if (type == ENCODING_TYPE::ec)
        ul->set_ec_param(ec_param);
    else
        ul->set_rawx_param(rawx_param);

    for (const auto &e : xattrs)
        ul->SetXattr(e.first, e.second);
    return std::unique_ptr<RouterUpload>(ul);
}
