/** Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#include <sstream>

#include <glog/log_severity.h>
#include <glog/logging.h>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <libmill.h>
#include <oio/kinetic/coro/rpc/Put.h>
#include <oio/kinetic/coro/rpc/GetKeyRange.h>
#include <oio/kinetic/coro/client/ClientInterface.h>
#include <oio/kinetic/coro/blob.h>

using oio::kinetic::blob::UploadBuilder;
using oio::kinetic::client::ClientInterface;
using oio::kinetic::client::ClientFactory;
using oio::kinetic::client::Sync;
using oio::kinetic::rpc::Put;
using oio::kinetic::rpc::GetKeyRange;

namespace blob = ::oio::api::blob;

class KineticUpload : public blob::Upload {
    friend class UploadBuilder;

public:
    KineticUpload();

    ~KineticUpload();

    blob::Upload::Status Prepare() override;

    void SetXattr (const std::string &k, const std::string &v) override;

    bool Commit() override;

    bool Abort() override;

    void Write(const uint8_t *buf, uint32_t len) override;

private:
    void TriggerUpload();

    void TriggerUpload(const std::string &suffix);

private:
    std::vector<std::shared_ptr<ClientInterface>> clients;
    uint32_t next_client;
    std::vector<std::shared_ptr<Put>> puts;
    std::vector<std::shared_ptr<Sync>> syncs;

    std::vector<uint8_t> buffer;
    uint32_t buffer_limit;
    std::string chunkid;
    std::map<std::string,std::string> xattr;
};

KineticUpload::~KineticUpload() {

}

KineticUpload::KineticUpload(): clients(), next_client{0}, puts(), syncs() {

}

void KineticUpload::SetXattr(const std::string &k, const std::string &v) {
    xattr[k] = v;
}

void KineticUpload::TriggerUpload(const std::string &suffix) {
    assert(!chunkid.empty());
    assert(clients.size() > 0);

    auto client = clients[next_client % clients.size()];
    std::stringstream ss;
    ss << chunkid;
    ss << '-';
    ss << suffix;
    next_client++;

    Put *put = new Put;
    put->Key(ss.str());
    put->Value(buffer);
    assert(buffer.size() == 0);

    puts.emplace_back(put);
    syncs.emplace_back(client->Start(put));
}

void KineticUpload::TriggerUpload() {
    std::stringstream ss;
    ss << next_client;
    ss << '-';
    ss << buffer.size();
    return TriggerUpload(ss.str());
}

void KineticUpload::Write(const uint8_t *buf, uint32_t len) {
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

bool KineticUpload::Commit() {

    // Flush the internal buffer so that we don't mix payload with xattr
    if (buffer.size() > 0)
        TriggerUpload();

    // Pack then send the xattr
    rapidjson::StringBuffer buf;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buf);
    writer.StartObject();
    for (const auto &e: xattr) {
        writer.Key(e.first.c_str());
        writer.String(e.second.c_str());
    }
    writer.EndObject();
    this->Write(reinterpret_cast<const uint8_t *>(buf.GetString()), buf.GetSize());
    TriggerUpload("#");

    // Wait for all the single PUT to finish
    for (auto &s: syncs)
        s->Wait();
    return true;
}

bool KineticUpload::Abort() {
    return true;
}

blob::Upload::Status KineticUpload::Prepare() {

    // Send the same listing request to all the clients, GetKeyRange allows this
    // even if it won't cleanly manage mixed errors and successes

    const std::string key_manifest(chunkid + "-#");

    std::vector<std::shared_ptr<GetKeyRange>> ops;
    std::vector<std::shared_ptr<Sync>> syncs;
    for (auto cli: clients) {
        std::shared_ptr<GetKeyRange> gkr(new GetKeyRange);
        gkr->Start(key_manifest);
        gkr->End(key_manifest);
        gkr->IncludeStart(true);
        gkr->IncludeEnd(true);
        gkr->MaxItems(1);
        ops.push_back(gkr);
    }
    int i = 0;
    for (auto cli: clients) {
        auto op = ops[i];
        syncs.push_back(cli->Start(op.get()));
    }
    for (auto sync: syncs)
        sync->Wait();
    for (auto op: ops) {
        if (!op->Ok())
            return blob::Upload::Status::NetworkError;
    }
    for (auto op: ops) {
        std::vector<std::string> keys;
        op->Steal(keys);
        if (!keys.empty())
            return blob::Upload::Status::Already;
    }

    return blob::Upload::Status::OK;
}

UploadBuilder::~UploadBuilder() {

}

UploadBuilder::UploadBuilder(std::shared_ptr<ClientFactory> f):
        factory(f), targets(), block_size{1024 * 1024} {

}

bool UploadBuilder::Target(const std::string &to) {
    targets.insert(to);
    return true;
}

bool UploadBuilder::Target(const char *to) {
    assert(to != nullptr);
    return Target(std::string(to));
}

bool UploadBuilder::Name(const std::string &n) {
    name.assign(n);
    return true;
}

bool UploadBuilder::Name(const char *n) {
    assert(n != nullptr);
    return Name(std::string(n));
}

void UploadBuilder::BlockSize(uint32_t s) {
    block_size = s;
}

std::unique_ptr<blob::Upload> UploadBuilder::Build() {
    assert(!name.empty());

    KineticUpload *ul = new KineticUpload();
    ul->buffer_limit = block_size;
    ul->chunkid.assign(name);
    for (const auto &to: targets)
        ul->clients.emplace_back(factory->Get(to.c_str()));
    return std::unique_ptr<KineticUpload>(ul);
}
