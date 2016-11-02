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

#include <cassert>

#include "oio/mem/blob.h"

using oio::api::blob::Cause;
using Step = oio::api::blob::TransactionStep;
using oio::api::blob::Status;
using oio::api::blob::Upload;
using oio::api::blob::Download;
using oio::api::blob::Removal;
using oio::mem::blob::UploadBuilder;
using oio::mem::blob::DownloadBuilder;
using oio::mem::blob::RemovalBuilder;
using oio::mem::blob::Cache;

void Cache::Item::Write(const uint8_t *buf, size_t len) {
    size_t old_len = data.size();
    data.resize(old_len + len);
    ::memcpy(data.data() + old_len, buf, len);
}

oio::api::blob::Status Cache::Create(const std::string &name) {
    auto it = items.find(name);
    if (it != items.end())
        return Status(Cause::Already);
    items.emplace(std::make_pair(name, Item()));
    return Status();
}

oio::api::blob::Status Cache::Commit(const std::string &name) {
    auto it = items.find(name);
    if (it == items.end())
        return Status(Cause::NotFound);
    if (!it->second.pending)
        return Status(Cause::InternalError);
    it->second.Validate();
    return Status(Cause::OK);
}

oio::api::blob::Status Cache::Abort(const std::string &name) {
    auto it = items.find(name);
    if (it == items.end())
        return Status(Cause::OK);
    if (!it->second.pending)
        return Status(Cause::InternalError);
    items.erase(name);
    return Status(Cause::OK);
}

oio::api::blob::Status Cache::Write(const std::string &name,
        const uint8_t *buf, uint32_t len) {
    auto it = items.find(name);
    if (it != items.end()) {
        if (it->second.pending) {
            it->second.Write(buf, len);
            return Status();
        }
        return Status(Cause::Already);
    }
    return Status(Cause::NotFound);
}

oio::api::blob::Status Cache::Xattr(const std::string &name,
        const std::string &k, const std::string &v) {
    auto it = items.find(name);
    if (it == items.end())
        return Status(Cause::NotFound);
    if (!it->second.pending)
        return Status(Cause::Already);
    it->second.Xattr(k, v);
    return Status();
}

oio::api::blob::Status Cache::Get(const std::string &name,
        std::vector<uint8_t> *out) {
    assert(out != nullptr);
    auto it = items.find(name);
    if (it == items.end())
        return Status(Cause::NotFound);
    out->resize(it->second.data.size());
    memcpy(out->data(), it->second.data.data(), out->size());
    return Status();
}

void Cache::Stat(const std::string &name, bool *present, bool *pending) {
    assert(present != nullptr);
    assert(pending != nullptr);
    auto it = items.find(name);
    if (it == items.end()) {
        *present = false;
        *pending = false;
    } else {
        *present = true;
        *pending = it->second.pending;
    }
}

void Cache::Remove(const std::string &name) {
    items.erase(name);
}


class MemUpload : public Upload {
    friend class UploadBuilder;
 public:
    ~MemUpload() {}

    explicit MemUpload(std::shared_ptr<Cache> cache) : cache_(cache),
                                                       step_{Step::Init} {}

    Status Prepare() override {
        if (step_ != Step::Init)
            return Status(Cause::InternalError);
        auto rc = cache_->Create(name_);
        if (rc.Ok())
            step_ = Step::Prepared;
        return rc;
    }

    Status Commit() override {
        if (step_ != Step::Prepared)
            return Status(Cause::InternalError);
        step_ = Step::Done;
        return cache_->Commit(name_);
    }

    Status Abort() override {
        if (step_ != Step::Prepared)
            return Status(Cause::InternalError);
        step_ = Step::Done;
        return cache_->Abort(name_);
    }

    void Write(const uint8_t *buf, uint32_t len) override {
        cache_->Write(name_, buf, len);
    }

    void SetXattr(const std::string &k, const std::string &v) override {
        cache_->Xattr(name_, k, v);
    }

 private:
    std::shared_ptr<Cache> cache_;
    std::string name_;
    Step step_;
};

std::unique_ptr<oio::api::blob::Upload> UploadBuilder::Build() {
    auto ul = new MemUpload(cache_);
    ul->name_.assign(name_);
    return std::unique_ptr<Upload>(ul);
}


class MemDownload : public Download {
    friend class DownloadBuilder;

 public:
    ~MemDownload() {}

    explicit MemDownload(std::shared_ptr<Cache> cache) : cache_(cache),
                                                         step_{Step::Init} {}

    Status Prepare() override {
        if (step_ != Step::Init)
            return Status(Cause::InternalError);
        auto rc = cache_->Get(name_, &value_);
        if (rc.Ok())
            step_ = Step::Prepared;
        return rc;
    }

    bool IsEof() override {
        return step_ != Step::Prepared;
    }

    int32_t Read(std::vector<uint8_t> *buf) override {
        assert(buf != nullptr);
        if (step_ != Step::Prepared)
            return -1;
        step_ = Step::Done;
        buf->clear();
        buf->swap(value_);
        return buf->size();
    }

 private:
    std::shared_ptr<Cache> cache_;
    std::string name_;
    std::vector<uint8_t> value_;
    Step step_;
};

std::unique_ptr<oio::api::blob::Download> DownloadBuilder::Build() {
    auto dl = new MemDownload(cache_);
    dl->name_.assign(name_);
    return std::unique_ptr<Download>(dl);
}


class MemRemoval : public Removal {
    friend class RemovalBuilder;

 public:
    ~MemRemoval() {}

    explicit MemRemoval(std::shared_ptr<Cache> cache) : cache_(cache),
                                                        step_{Step::Init} {}

    Status Prepare() override {
        if (step_ != Step::Init)
            return Status(Cause::InternalError);
        bool present{false}, pending{false};
        cache_->Stat(name_, &present, &pending);
        if (!present || pending)
            return Status(Cause::NotFound);
        step_ = Step::Prepared;
        return Status();
    }

    Status Commit() override {
        if (step_ != Step::Prepared)
            return Status(Cause::InternalError);

        bool present{false}, pending{false};
        cache_->Stat(name_, &present, &pending);
        if (!present || pending)
            return Status(Cause::NotFound);

        cache_->Remove(name_);

        step_ = Step::Done;
        return Status();
    }

    Status Abort() override {
        if (step_ != Step::Prepared)
            return Status(Cause::InternalError);
        step_ = Step::Done;
        return Status();
    }

 private:
    std::shared_ptr<Cache> cache_;
    std::string name_;
    Step step_;
};

std::unique_ptr<oio::api::blob::Removal> RemovalBuilder::Build() {
    auto dl = new MemRemoval(cache_);
    dl->name_.assign(name_);
    return std::unique_ptr<Removal>(dl);
}
