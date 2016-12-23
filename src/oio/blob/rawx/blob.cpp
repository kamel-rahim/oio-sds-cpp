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

#include "oio/blob/rawx/blob.h"

#include <cassert>
#include <string>
#include <functional>
#include <sstream>
#include <memory>
#include <vector>
#include <iomanip>
#include <cstring>

#include "utils/macros.h"
#include "utils/net.h"
#include "utils/Http.h"
#include "oio/api/blob.h"

using oio::api::Cause;
using oio::api::Status;
using oio::rawx::blob::RemovalBuilder;
using oio::rawx::blob::UploadBuilder;
using oio::rawx::blob::DownloadBuilder;
using oio::api::blob::Removal;
using oio::api::blob::Upload;
using oio::api::blob::Download;
using Step = oio::api::blob::TransactionStep;

/**
 *
 */
class RawxRemoval : public Removal {
    friend class RemovalBuilder;

 private:
    std::unique_ptr<Removal> inner;
    Step step_;

 public:
    explicit RawxRemoval(Removal *sub) : inner(sub), step_{Step::Init} {}

    virtual ~RawxRemoval() {}

    Status Prepare() override {
        if (step_ != Step::Init)
            return Status(Cause::InternalError);
        auto rc = inner->Prepare();
        if (rc.Ok())
            step_ = Step::Prepared;
        return rc;
    }

    Status Commit() override {
        if (step_ != Step::Prepared)
            return Status(Cause::InternalError);
        step_ = Step::Done;
        return inner->Commit();
    }

    Status Abort() override {
        if (step_ != Step::Prepared)
            return Status(Cause::InternalError);
        step_ = Step::Done;
        return inner->Abort();
    }
};

RemovalBuilder::RemovalBuilder() {}

RemovalBuilder::~RemovalBuilder() {}

void RemovalBuilder::set_param(const rawx_cmd &_param) {
    rawx_param = _param;
    inner.Host(rawx_param.Host_Port());
    inner.Name("/rawx/" + rawx_param.ChunkId());
}

std::unique_ptr<Removal> RemovalBuilder::Build(
        std::shared_ptr<net::Socket> socket) {
    auto sub = inner.Build(socket);
    auto rem = new RawxRemoval(sub.release());
    return std::unique_ptr<Removal>(rem);
}


/**
 *
 */
class RawxUpload : public Upload {
    friend class UploadBuilder;
 private:
    std::unique_ptr<Upload> inner;
    Step step_;

 public:
    explicit RawxUpload(Upload *sub) : inner(sub), step_{Step::Init} {
        assert(sub != nullptr);
    }

    ~RawxUpload() { }

    void SetXattr(const std::string &k, const std::string &v) override {
        return inner->SetXattr(k, v);
    }

    Status Prepare() override {
        if (step_ != Step::Init)
            return Status(Cause::InternalError);
        auto rc = inner->Prepare();
        if (rc.Ok())
            step_ = Step::Prepared;
        return rc;
    }

    Status Commit() override {
        if (step_ != Step::Prepared)
            return Status(Cause::InternalError);
        step_ = Step::Done;
        return inner->Commit();
    }

    Status Abort() override {
        if (step_ != Step::Prepared)
            return Status(Cause::InternalError);
        step_ = Step::Done;
        return inner->Abort();
    }

    void Write(const uint8_t *buf, uint32_t len) override {
        return inner->Write(buf, len);
    }
};

UploadBuilder::UploadBuilder() {}

UploadBuilder::~UploadBuilder() {}


void UploadBuilder::set_param(const rawx_cmd &_param) {
    rawx_param = _param;
    std::stringstream ss;
    ss << rawx_param.size << '.' << 0;
    inner.Field("X-oio-chunk-meta-chunk-pos", ss.str());

    inner.Host(rawx_param.Host_Port());

    inner.Field("X-oio-chunk-meta-chunk-id", rawx_param.ChunkId());
    inner.Name("/rawx/" + rawx_param.ChunkId());
}

void UploadBuilder::ContainerId(const std::string &s) {
    inner.Field("X-oio-chunk-meta-container-id", s);
}

void UploadBuilder::ContentPath(const std::string &s) {
    inner.Field("X-oio-chunk-meta-content-path", s);
}

void UploadBuilder::ContentId(const std::string &s) {
    inner.Field("X-oio-chunk-meta-content-id", s);
}

void UploadBuilder::MimeType(const std::string &s) {
    inner.Field("X-oio-chunk-meta-content-mime-type", s);
}

void UploadBuilder::StoragePolicy(const std::string &s) {
    inner.Field("X-oio-chunk-meta-content-storage-policy", s);
}

void UploadBuilder::ChunkMethod(const std::string &s) {
    inner.Field("X-oio-chunk-meta-content-chunk-method", s);
}

void UploadBuilder::ContentVersion(int64_t v) {
    std::stringstream ss;
    ss << v;
    inner.Field("X-oio-chunk-meta-content-version", ss.str());
}

void UploadBuilder::Property(const std::string &k, const std::string &v) {
    inner.Field("X-oio-chunk-meta-x-" + k, v);
}

std::unique_ptr<Upload> UploadBuilder::Build(
        std::shared_ptr<net::Socket> socket) {
    auto sub = inner.Build(socket);
    assert(sub.get() != nullptr);
    auto ul = new RawxUpload(sub.release());
    return std::unique_ptr<Upload>(ul);
}


/**
 *
 */
class RawxDownload : public Download {
    friend class DownloadBuilder;

 private:
    std::unique_ptr<Download> inner;
    rawx_cmd rawx_param;
    Step step_;

 public:
    explicit RawxDownload(Download *sub) : inner(sub), step_{Step::Init} {}

    ~RawxDownload() {}

    void set_param(const rawx_cmd &_param) {
        rawx_param = _param;
    }

    bool IsEof() override { return inner->IsEof(); }

    Status Prepare() override {
        if (step_ != Step::Init)
            return Status(Cause::InternalError);
        auto rc = inner->Prepare();
        if (rc.Ok())
            step_ = Step::Prepared;
        return rc;
    }

    int32_t Read(std::vector<uint8_t> *buf) override {
        if (step_ != Step::Prepared)
            return -1;

        std::vector<uint8_t> temp_buf;

        while (!IsEof()) {
            inner->Read(&temp_buf);
        }

        if (temp_buf.size() > rawx_param.size) {
            buf->resize(rawx_param.size);
            memcpy(buf->data(), &temp_buf[rawx_param.start], rawx_param.size);
        } else {
            buf->swap(temp_buf);
        }

        return buf->size();
    }
};

DownloadBuilder::DownloadBuilder() {}

DownloadBuilder::~DownloadBuilder() {}

void DownloadBuilder::set_param(const rawx_cmd &_param) {
    rawx_param = _param;
    inner.Host(rawx_param.Host_Port());
    inner.Name("/rawx/" + rawx_param.ChunkId());
}

std::unique_ptr<Download> DownloadBuilder::Build(
    std::shared_ptr<net::Socket> socket) {
    auto sub = inner.Build(socket);
    auto dl = new RawxDownload(sub.release());
    dl->set_param(rawx_param);
    return std::unique_ptr<Download>(dl);
}
