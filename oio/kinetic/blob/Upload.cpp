/** Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#include <sstream>

#include <glog/log_severity.h>
#include <glog/logging.h>
#include <libmill.h>

#include <oio/kinetic/rpc/Put.h>
#include <oio/kinetic/client/ClientInterface.h>
#include "Upload.h"

using oio::kinetic::blob::UploadBuilder;
using oio::kinetic::client::ClientInterface;
using oio::kinetic::client::ClientFactory;
using oio::kinetic::client::Sync;
using oio::kinetic::rpc::Put;

class Upload : public oio::blob::Upload {
    friend class UploadBuilder;

public:
    Upload() noexcept;

    ~Upload() noexcept;

    bool Prepare() noexcept;

    bool Commit() noexcept;

    bool Abort() noexcept;

    void Write(const uint8_t *buf, uint32_t len) noexcept;

    void Write(const std::string &s) noexcept;

    void Flush() noexcept;

private:
    void TriggerUpload() noexcept;

    void TriggerUpload(const std::string &suffix) noexcept;

private:
    std::vector<std::shared_ptr<ClientInterface>> clients;
    uint32_t next_client;
    std::vector<std::shared_ptr<Put>> puts;
    std::vector<std::shared_ptr<Sync>> syncs;

    std::vector<uint8_t> buffer;
    uint32_t buffer_limit;
    std::string chunkid;
};

Upload::~Upload() noexcept {
    DLOG(INFO) << __FUNCTION__;
}

Upload::Upload() noexcept: clients(), next_client{0}, puts(), syncs() {
    DLOG(INFO) << __FUNCTION__;
}

void Upload::TriggerUpload(const std::string &suffix) noexcept {
    assert(!chunkid.empty());
    assert(clients.size() > 0);

    auto client = clients[next_client % clients.size()];
    std::stringstream ss;
    ss << chunkid;
    ss << '-';
    ss << suffix;
    next_client++;

    std::shared_ptr<Put> op(new Put);
    op->Key(ss.str());
    op->Value(buffer);
    assert(buffer.size() == 0);

    syncs.push_back(client->Start(op.get()));
    puts.push_back(op);
}

void Upload::TriggerUpload() noexcept {
    std::stringstream ss;
    ss << next_client;
    ss << '-';
    ss << buffer.size();
    return TriggerUpload(ss.str());
}

void Upload::Write(const uint8_t *buf, uint32_t len) noexcept {
    assert(clients.size() > 0);

    while (len > 0) {
        bool action = false;
        const auto oldsize = buffer.size();
        const uint32_t avail = buffer_limit - oldsize;
        const uint32_t local = std::min(avail, len);
        if (local > 0) {
            buffer.resize(oldsize + local);
            memcpy(buffer.data() + oldsize, buf, local);
            buf += local;
            len -= local;
            action = true;
        }
        if (buffer.size() >= buffer_limit) {
            TriggerUpload();
            action = true;
        }
        assert(action);
    }
    yield();
}

void Upload::Write(const std::string &s) noexcept {
    return Write(reinterpret_cast<const uint8_t *>(s.data()), s.size());
}

void Upload::Flush() noexcept {
    DLOG(INFO) << __FUNCTION__ << " of "<< buffer.size() << " bytes";
    if (buffer.size() > 0)
        TriggerUpload();
}

bool Upload::Commit() noexcept {
    Flush();

    DLOG(INFO) << __FUNCTION__ << " sending xattr";
    // Send the attr metadata
    std::string attr("{}");
    Write(attr);

    TriggerUpload("#");

    DLOG(INFO) << __FUNCTION__ << " waiting for sub-op results";
    for (auto &s: syncs)
        s->Wait();
    return true;
}

bool Upload::Abort() noexcept {
    return true;
}

bool Upload::Prepare() noexcept {
    return true;
}

UploadBuilder::~UploadBuilder() noexcept { }

UploadBuilder::UploadBuilder(std::shared_ptr<ClientFactory> f) noexcept:
        factory(f), targets(), block_size{512 * 1024} { }

void UploadBuilder::Target(const std::string &to) noexcept {
    targets.insert(to);
}

void UploadBuilder::Target(const char *to) noexcept {
    assert(to != nullptr);
    return Target(std::string(to));
}

void UploadBuilder::Name(const std::string &n) noexcept {
    name.assign(n);
}

void UploadBuilder::Name(const char *n) noexcept {
    assert(n != nullptr);
    return Name(std::string(n));
}

void UploadBuilder::BlockSize(uint32_t s) noexcept {
    block_size = s;
}

std::unique_ptr<oio::blob::Upload> UploadBuilder::Build() noexcept {
    assert(!name.empty());

    Upload *ul = new Upload();
    ul->buffer_limit = block_size;
    ul->chunkid.assign(name);
    for (const auto &to: targets)
        ul->clients.emplace_back(factory->Get(to.c_str()));
    DLOG(INFO) << __FUNCTION__ << " with " << static_cast<int>(ul->clients.size());
    return std::unique_ptr<Upload>(ul);
}
