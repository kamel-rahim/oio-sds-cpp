/** Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#include <queue>
#include <functional>
#include <forward_list>
#include <glog/logging.h>
#include <oio/kinetic/coro/rpc/Get.h>
#include <oio/kinetic/coro/rpc/GetKeyRange.h>
#include "oio/kinetic/coro/client/ClientInterface.h"
#include <oio/kinetic/coro/blob.h>

using oio::kinetic::rpc::Get;
using oio::kinetic::rpc::GetKeyRange;
using oio::kinetic::client::ClientFactory;
using oio::kinetic::client::ClientInterface;
using oio::kinetic::client::Sync;
using oio::kinetic::blob::ListingBuilder;
using oio::kinetic::blob::DownloadBuilder;

namespace blob = ::oio::api::blob;

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
            : chunkid{n}, targets(), factory{f}, running(), waiting(), done(),
              parallel_factor{4} { targets.swap(t); };

    virtual ~KineticDownload() { }

    blob::Download::Status Prepare() override {

        // List the chunks
        ListingBuilder builder(factory);
        builder.Name(chunkid);
        for (const auto &to: targets)
            builder.Target(to);

        auto listing = builder.Build();
        switch (listing->Prepare()) {
            case blob::Listing::Status::OK:
                break;
            case blob::Listing::Status::NotFound:
                return blob::Download::Status::NotFound;
            case blob::Listing::Status::NetworkError:
                return blob::Download::Status::NetworkError;
            case blob::Listing::Status::ProtocolError:
                return blob::Download::Status::ProtocolError;
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

        return blob::Download::Status::OK;
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
                auto &pg = waiting.front();
                pg.sync = pg.client->Start(pg.op.get());
                running.push(pg);
                waiting.pop();
                DLOG(INFO) << "chunk download started";
            }
        }

        if (running.empty())
            return 0;

        auto &pg = running.front();
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
        name(), targets(), factory(f) { }

DownloadBuilder::~DownloadBuilder() { }

void DownloadBuilder::Name(const std::string &n) {
    name.assign(n);
}

void DownloadBuilder::Name(const char *n) {
    assert(n != nullptr);
    name.assign(std::string(n));
}

void DownloadBuilder::Target(const std::string &to) {
    targets.insert(to);
}

void DownloadBuilder::Target(const char *to) {
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
