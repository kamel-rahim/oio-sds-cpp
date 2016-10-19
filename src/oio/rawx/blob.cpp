/** Copyright (c) 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#include "oio/rawx/blob.h"

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

using oio::rawx::blob::RemovalBuilder;
using oio::rawx::blob::UploadBuilder;
using oio::rawx::blob::DownloadBuilder;
using oio::api::blob::Removal;
using oio::api::blob::Upload;
using oio::api::blob::Download;
using oio::api::blob::Status;
using oio::api::blob::Cause;

/**
 *
 */
class RawxRemoval : public Removal {
    friend class RemovalBuilder;

 public:
    explicit RawxRemoval(Removal *sub) : inner(sub) {}

    virtual ~RawxRemoval() {}

    Status Prepare() override { return inner->Prepare(); }

    Status Commit() override { return inner->Commit(); }

    Status Abort() override { return inner->Abort(); }

 private:
    std::unique_ptr<Removal> inner;
};

RemovalBuilder::RemovalBuilder() {}

RemovalBuilder::~RemovalBuilder() {}

void RemovalBuilder::Name(const std::string &s) { return inner.Name(s); }

void RemovalBuilder::Host(const std::string &s) { return inner.Host(s); }

void RemovalBuilder::Field(const std::string &k, const std::string &v) {
    return inner.Field(k, v);
}

void RemovalBuilder::Trailer(const std::string &k) {
    return inner.Trailer(k);
}

std::shared_ptr<Removal> RemovalBuilder::Build(
        std::shared_ptr<net::Socket> socket) {
    auto sub = inner.Build(socket);
    auto rem = new RawxRemoval(sub.get());
    sub.reset();
    return std::shared_ptr<Removal>(rem);
}


/**
 *
 */
class RawxUpload : public Upload {
    friend class UploadBuilder;

 public:
    explicit RawxUpload(Upload *sub) : inner(sub) {}

    ~RawxUpload() {}

    void SetXattr(const std::string &k, const std::string &v) override {
        return inner->SetXattr(k, v);
    }

    Status Prepare() override { return inner->Prepare(); }

    Status Commit() override { return inner->Commit(); }

    Status Abort() override { return inner->Abort(); }

    void Write(const uint8_t *buf, uint32_t len) override {
        return inner->Write(buf, len);
    }

 private:
    std::unique_ptr<Upload> inner;
};

UploadBuilder::UploadBuilder() {}

UploadBuilder::~UploadBuilder() {}

void UploadBuilder::Name(const std::string &s) { return inner.Name(s); }

void UploadBuilder::Host(const std::string &s) { return inner.Host(s); }

void UploadBuilder::Field(const std::string &k, const std::string &v) {
    return inner.Field(k, v);
}

void UploadBuilder::Trailer(const std::string &k) {
    return inner.Trailer(k);
}

std::shared_ptr<Upload> UploadBuilder::Build(
        std::shared_ptr<net::Socket> socket) {
    auto sub = inner.Build(socket);
    auto ul = new RawxUpload(sub.get());
    sub.reset();
    return std::shared_ptr<Upload>(ul);
}


/**
 *
 */
class RawxDownload : public Download {
    friend class DownloadBuilder;

 public:
    explicit RawxDownload(Download *sub) : inner(sub) {}

    ~RawxDownload() {}

    bool IsEof() override { return inner->IsEof(); }

    Status Prepare() override { return inner->Prepare(); }

    int32_t Read(std::vector<uint8_t> *buf) override {
        return inner->Read(buf);
    }

 private:
    std::unique_ptr<Download> inner;
};

DownloadBuilder::DownloadBuilder() {}

DownloadBuilder::~DownloadBuilder() {}

void DownloadBuilder::Field(const std::string &k, const std::string &v) {
    return inner.Field(k, v);
}

void DownloadBuilder::Host(const std::string &s) { return inner.Host(s); }

void DownloadBuilder::Name(const std::string &s) { return inner.Name(s); }

std::shared_ptr<Download> DownloadBuilder::Build(
        std::shared_ptr<net::Socket> socket) {
    auto sub = inner.Build(socket);
    auto dl = new RawxDownload(sub.get());
    sub.reset();
    return std::shared_ptr<Download>(dl);
}
