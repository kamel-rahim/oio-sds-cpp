/** Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#include <cassert>
#include <queue>
#include <glog/logging.h>
#include <oio/kinetic/coro/rpc/Delete.h>
#include <oio/kinetic/coro/blob.h>

using oio::kinetic::client::Sync;
using oio::kinetic::client::ClientInterface;
using oio::kinetic::client::ClientFactory;
using oio::kinetic::blob::RemovalBuilder;
using oio::kinetic::blob::ListingBuilder;
using oio::kinetic::rpc::Delete;
namespace blob = ::oio::api::blob;

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
        sync = client->Start(op.get());
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

    blob::Removal::Status Prepare() override {
        ListingBuilder builder(factory);
        builder.Name(chunkid);
        for (const auto &to: targets)
            builder.Target(to);
        auto listing = builder.Build();

        auto rc = listing->Prepare();
        switch (rc) {
            case blob::Listing::Status::OK:
                break;
            case blob::Listing::Status::NotFound:
                return blob::Removal::Status::NotFound;
            case blob::Listing::Status::NetworkError:
                return blob::Removal::Status::NetworkError;
            case blob::Listing::Status::ProtocolError:
                return blob::Removal::Status::ProtocolError;
        }

        std::string id, key;
        while (listing->Next(id, key)) {
            PendingDelete del(factory->Get(id), key);
            DLOG(INFO) << "rem("<< id << ","<< key <<")";
            ops.push_back(del);
        }

        return blob::Removal::Status::OK;
    }

    bool Commit() override {
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
        return true;
    }

    bool Abort() override { return false; }

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