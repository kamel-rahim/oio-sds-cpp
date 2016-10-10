/**
 * Copyright (c) 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/
 */

#include <string>
#include <functional>
#include <queue>
#include <forward_list>

#include <libmill.h>
#include <rapidjson/filereadstream.h>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>

#include <utils/macros.h>
#include <oio/api/blob.h>
#include <oio/kinetic/coro/blob.h>
#include <oio/kinetic/coro/client/ClientInterface.h>
#include <oio/kinetic/coro/rpc/Get.h>
#include <oio/kinetic/coro/rpc/Put.h>
#include <oio/kinetic/coro/rpc/Delete.h>
#include <oio/kinetic/coro/rpc/GetKeyRange.h>

using oio::kinetic::blob::ListingBuilder;
using oio::kinetic::blob::DownloadBuilder;
using oio::kinetic::blob::RemovalBuilder;
using oio::kinetic::blob::UploadBuilder;
using oio::kinetic::client::ClientInterface;
using oio::kinetic::client::ClientFactory;
using oio::kinetic::client::Sync;
using oio::kinetic::rpc::Put;
using oio::kinetic::rpc::Get;
using oio::kinetic::rpc::Delete;
using oio::kinetic::rpc::GetKeyRange;

namespace blob = ::oio::api::blob;
using blob::Status;
using blob::Cause;

struct PendingGet {
    uint32_t sequence;
    uint32_t size;
    std::shared_ptr<ClientInterface> client;
    std::shared_ptr<Get> op;
    std::shared_ptr<Sync> sync;

    PendingGet(std::shared_ptr<ClientInterface> c, const std::string &k)
            : sequence{0}, size{0}, client{c}, op(new Get), sync(nullptr) {
        op->Key(k);
    }
};

struct PendingGetSorter {
    bool cmp(const PendingGet &p0, const PendingGet &p1) const {
        return p0.sequence < p1.sequence;
    }
};

class KineticDownload : public blob::Download {
    friend class DownloadBuilder;

  public:
    KineticDownload(const std::string &n, std::shared_ptr<ClientFactory> f,
            std::vector<std::string> t)
            : chunkid{n}, targets(), factory(f), running(), waiting(), done(),
              parallel_factor{4} {
        assert(factory.get() != nullptr);
        targets.swap(t);
    };

    virtual ~KineticDownload() { }

    Status Prepare() override {

        // List the chunks
        ListingBuilder builder(factory);
        builder.Name(chunkid);
        for (const auto &to: targets)
            builder.Target(to);

        auto listing = builder.Build();
        auto rc = listing->Prepare();
        switch (rc.Why()) {
            case Cause::OK:
                break;
            default:
                return rc;
        }

        std::string id, key;
        std::forward_list<PendingGet> chunks;
        while (listing->Next(id, key)) {
            std::string k(key);
            auto dash = k.rfind('-');
            if (dash == std::string::npos) {
                // malformed
                DLOG(INFO) << "Malformed [" << k << "]";
            } else if (k[dash + 1] == '#') {
                // Manifest
                DLOG(INFO) << "Manifest [" << key << "]";
            } else {
                int size = std::stoi(k.substr(dash + 1));
                k.resize(dash);
                dash = k.rfind('-');
                if (dash == std::string::npos) {
                    // malformed
                    DLOG(INFO) << "Malformed [" << k << "]";
                } else {
                    int seq = std::stoi(k.substr(dash + 1));
                    k.resize(dash);
                    PendingGet pg(factory->Get(id), key);
                    pg.size = size;
                    pg.sequence = seq;
                    DLOG(INFO) << "Chunk [" << key << "] seq=" << pg.sequence <<
                               " size=" << pg.size;
                    chunks.push_front(pg);
                }
            }
        }
        chunks.sort([](const PendingGet &p0, const PendingGet &p1) -> bool {
            return p0.sequence < p1.sequence;
        });

        for (auto &p: chunks)
            waiting.push(p);

        return Status();
    }

    bool IsEof() override {
        return waiting.empty() && running.empty();
    }

    int32_t Read(std::vector<uint8_t> &buf) override {
        DLOG(INFO) << "Currently " << running.size() <<
                   " chunks downbloads running";
        while (running.size() < parallel_factor) {
            if (waiting.empty()) {
                DLOG(INFO) << "No chunks in the waiting queue";
                break;
            } else {
                auto pg = waiting.front();
                waiting.pop();
                pg.sync = pg.client->RPC(pg.op.get());
                running.push(pg);
                DLOG(INFO) << "chunk download started";
            }
        }

        if (running.empty())
            return 0;

        auto pg = running.front();
        running.pop();
        pg.sync->Wait();
        pg.op->Steal(buf);
        return buf.size();
    }

  private:
    std::string chunkid;
    std::vector<std::string> targets;
    std::shared_ptr<ClientFactory> factory;

    std::queue<PendingGet> running;
    std::queue<PendingGet> waiting;
    std::queue<PendingGet> done;

    unsigned int parallel_factor;
};

DownloadBuilder::DownloadBuilder(std::shared_ptr<ClientFactory> f):
        factory(f), targets(), name() {
    assert(factory.get() != nullptr);
}

DownloadBuilder::~DownloadBuilder() { }

bool DownloadBuilder::Name(const std::string &n) {
    name.assign(n);
    return true;
}

bool DownloadBuilder::Name(const char *n) {
    assert(n != nullptr);
    return Name(std::string(n));
}

bool DownloadBuilder::Target(const std::string &to) {
    targets.insert(to);
    return true;
}

bool DownloadBuilder::Target(const char *to) {
    assert(to != nullptr);
    return Target(std::string(to));
}

std::unique_ptr<blob::Download> DownloadBuilder::Build() {
    assert(!targets.empty());
    assert(!name.empty());

    std::vector<std::string> v;
    for (const auto &t: targets)
        v.emplace_back(t);
    return std::unique_ptr<KineticDownload>(new KineticDownload(name, factory, std::move(v)));
}


class KineticListing : public blob::Listing {
    friend class ListingBuilder;

  public:
    KineticListing(): clients(), name(), items(), next_item{0} { }

    ~KineticListing() { }

    // Concurrently get the lists on each node
    // TODO limit the parallelism to N (configurable) items
    Status Prepare() override {
        items.clear();

        // Mark clients tat still have key
        std::vector<bool> readyv(clients.size());
        std::vector<std::string> markers;
        for (int i=0,max=readyv.size(); i<max ;++i)
            readyv[i] = true;
        for (int i=0,max=clients.size(); i<max ;++i)
            markers.push_back(name + "-");


        for (;;) {
            // Stop iterating if not any drive reports it still has keys
            // TODO this cann be a one-liner with the help of a lambda
            bool any = false;
            for (int i=0,max=readyv.size(); i<max && !any ;++i)
                any = readyv[i];
            if (!any)
                break;

            std::vector<std::shared_ptr<GetKeyRange>> ops(clients.size());
            std::vector<std::shared_ptr<Sync>> syncs;

            // Initial batch of listings
            for (int i=0,max=readyv.size(); i<max ;++i) {
                if (!readyv[i])
                    continue;
                auto gkr = new GetKeyRange;
                gkr->Start(markers[i]);
                gkr->End(name + "-X");
                gkr->IncludeStart(false);
                gkr->IncludeEnd(false);
                ops[i].reset(gkr);
                syncs.emplace_back(clients[i]->RPC(gkr));
            }
            for (auto sync: syncs)
                sync->Wait();

            for (unsigned int i = 0; i < ops.size(); ++i) {
                if (!readyv[i])
                    continue;
                std::vector<std::string> keys;
                ops[i]->Steal(keys);
                if (keys.empty())
                    readyv[i] = false;
                else {
                    markers[i] = std::string(keys.back());
                    for (auto &k: keys) {
                        // We try here to avoid copies
                        std::string s;
                        s.swap(k);
                        items.emplace_back(i, std::move(s));
                        assert(s.size() == 0);
                        assert(k.size() == 0);
                    }
                }
            }
        }

        if (items.empty())
            return Status(Cause::NotFound);
        return Status();
    }

    bool Next(std::string &id, std::string &key) override {
        if (next_item >= items.size())
            return false;

        const auto &item = items[next_item++];
        id.assign(clients[std::get<0>(item)]->Id());
        key.assign(std::get<1>(item));
        return true;
    }

  private:
    std::vector<std::shared_ptr<ClientInterface>> clients;
    std::string name;

    std::vector<std::pair<int, std::string>> items;
    unsigned int next_item;
};

ListingBuilder::~ListingBuilder() { }

ListingBuilder::ListingBuilder(std::shared_ptr<ClientFactory> f)
        : factory(f), targets(), name() {
    assert(factory.get() != nullptr);
}

bool ListingBuilder::Name(const std::string &n) {
    name.assign(n);
    return true;
}

bool ListingBuilder::Name(const char *n) {
    assert(n != nullptr);
    return Name(std::string(n));
}

bool ListingBuilder::Target(const std::string &to) {
    targets.insert(to);
    return true;
}

bool ListingBuilder::Target(const char *to) {
    assert(to != nullptr);
    return Target(std::string(to));
}

std::unique_ptr<blob::Listing> ListingBuilder::Build() {
    assert(factory.get() != nullptr);
    assert(!targets.empty());
    assert(!name.empty());

    auto listing = new KineticListing;
    listing->name.assign(name);
    for (auto to: targets)
        listing->clients.emplace_back(factory->Get(to));
    return std::unique_ptr<KineticListing>(listing);
}


struct PendingDelete {
    std::shared_ptr<Delete> op;
    std::shared_ptr<ClientInterface> client;
    std::shared_ptr<Sync> sync;

    PendingDelete(std::shared_ptr<ClientInterface> c, const std::string &k)
            : op(new Delete), client{c}, sync(nullptr) {
        op->Key(k);
    }
    void Start() {
        assert(sync.get() == nullptr);
        sync = client->RPC(op.get());
    }
};

class KineticRemoval : public blob::Removal {
    friend class RemovalBuilder;
  public:
    KineticRemoval(std::shared_ptr<ClientFactory> f,
            std::vector<std::string> tv)
            : parallelism_factor{8}, chunkid(), targets(), factory(f), ops() {
        targets.swap(tv);
    }

    ~KineticRemoval() { }

    Status Prepare() override {
        ListingBuilder builder(factory);
        builder.Name(chunkid);
        for (const auto &to: targets)
            builder.Target(to);
        auto listing = builder.Build();

        auto rc = listing->Prepare();
        if (rc.Ok()) {
            std::string id, key;
            while (listing->Next(id, key)) {
                PendingDelete del(factory->Get(id), key);
                DLOG(INFO) << "rem(" << id << "," << key << ")";
                ops.push_back(del);
            }
        }
        return rc;
    }

    Status Commit() override {
        DLOG(INFO) << __FUNCTION__ << " of " << ops.size() << " ops";
        // Pre-start as many parallel operations as the configured parallelism
        for (unsigned int i = 0; i < parallelism_factor && i < ops.size(); ++i)
            ops[i].Start();

        for (unsigned int i = 0; i < ops.size(); ++i) {
            ops[i].sync->Wait();
            // an operation finished, pre-start another one
            if (i + parallelism_factor < ops.size())
                ops[i + parallelism_factor].Start();
        }
        return Status();
    }

    Status Abort() override { return Status(Cause::Unsupported); }

  private:
    unsigned int parallelism_factor;
    std::string chunkid;
    std::vector<std::string> targets;
    std::shared_ptr<ClientFactory> factory;

    std::vector<PendingDelete> ops;
};

RemovalBuilder::RemovalBuilder(std::shared_ptr<ClientFactory> f)
        : factory(f), targets(), name() {
}

bool RemovalBuilder::Name(const std::string &n) {
    name.assign(n);
    return true;
}

bool RemovalBuilder::Name(const char *n) {
    assert(n != nullptr);
    return Name(std::string(n));
}

bool RemovalBuilder::Target(const std::string &to) {
    targets.insert(to);
    return true;
}

bool RemovalBuilder::Target(const char *to) {
    assert(to != nullptr);
    return Target(std::string(to));
}

std::unique_ptr<blob::Removal> RemovalBuilder::Build() {
    assert(!targets.empty());
    assert(!name.empty());

    std::vector<std::string> tv;
    for (const auto &t : targets)
        tv.push_back(t);
    auto rem = new KineticRemoval(factory, std::move(tv));
    rem->chunkid.assign(name);
    return std::unique_ptr<KineticRemoval>(rem);
}


class KineticUpload : public blob::Upload {
    friend class UploadBuilder;

  public:
    KineticUpload();

    ~KineticUpload();

    Status Prepare() override;

    void SetXattr (const std::string &k, const std::string &v) override;

    Status Commit() override;

    Status Abort() override;

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
    syncs.emplace_back(client->RPC(put));
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

Status KineticUpload::Commit() {

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

    return Status();
}

Status KineticUpload::Abort() {
    return Status();
}

Status KineticUpload::Prepare() {

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
        syncs.push_back(cli->RPC(op.get()));
    }
    for (auto sync: syncs)
        sync->Wait();
    for (auto op: ops) {
        if (!op->Ok())
            return Status(Cause::NetworkError);
    }
    for (auto op: ops) {
        std::vector<std::string> keys;
        op->Steal(keys);
        if (!keys.empty())
            return Status(Cause::Already);
    }

    return Status();
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