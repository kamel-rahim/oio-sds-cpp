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

class Upload : public blob::Upload {
    friend class UploadBuilder;

public:
    Upload();

    ~Upload();

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

Upload::~Upload() {
    DLOG(INFO) << __FUNCTION__;
}

Upload::Upload(): clients(), next_client{0}, puts(), syncs() {
    DLOG(INFO) << __FUNCTION__;
}

void Upload::SetXattr(const std::string &k, const std::string &v) {
    xattr[k] = v;
}

void Upload::TriggerUpload(const std::string &suffix) {
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

void Upload::TriggerUpload() {
    std::stringstream ss;
    ss << next_client;
    ss << '-';
    ss << buffer.size();
    return TriggerUpload(ss.str());
}

void Upload::Write(const uint8_t *buf, uint32_t len) {
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

bool Upload::Commit() {

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

bool Upload::Abort() {
    return true;
}

blob::Upload::Status Upload::Prepare() {

    // Send the same listing request to all the clients
    const std::string key_manifest(chunkid + "-#");
    GetKeyRange gkr;
    gkr.Start(key_manifest);
    gkr.End(key_manifest);
    gkr.IncludeStart(true);
    gkr.IncludeEnd(true);
    gkr.MaxItems(1);
    std::vector<std::shared_ptr<Sync>> ops;
    for (auto cli: clients)
        ops.push_back(cli->Start(&gkr));
    for (auto op: ops)
        op->Wait();
    std::vector<std::string> keys;
    gkr.Steal(keys);
    if (!keys.empty())
        return blob::Upload::Status::Already;

    return blob::Upload::Status::OK;
}

UploadBuilder::~UploadBuilder() { }

UploadBuilder::UploadBuilder(std::shared_ptr<ClientFactory> f):
        factory(f), targets(), block_size{512 * 1024} { }

void UploadBuilder::Target(const std::string &to) {
    targets.insert(to);
}

void UploadBuilder::Target(const char *to) {
    assert(to != nullptr);
    return Target(std::string(to));
}

void UploadBuilder::Name(const std::string &n) {
    name.assign(n);
}

void UploadBuilder::Name(const char *n) {
    assert(n != nullptr);
    return Name(std::string(n));
}

void UploadBuilder::BlockSize(uint32_t s) {
    block_size = s;
}

std::unique_ptr<blob::Upload> UploadBuilder::Build() {
    assert(!name.empty());

    Upload *ul = new Upload();
    ul->buffer_limit = block_size;
    ul->chunkid.assign(name);
    for (const auto &to: targets)
        ul->clients.emplace_back(factory->Get(to.c_str()));
    DLOG(INFO) << __FUNCTION__ << " with " << static_cast<int>(ul->clients.size());
    return std::unique_ptr<Upload>(ul);
}
