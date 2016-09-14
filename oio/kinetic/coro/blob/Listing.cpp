/** Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#include <string>
#include <glog/logging.h>
#include <oio/api/blob.h>
#include <oio/kinetic/coro/client/ClientInterface.h>
#include <oio/kinetic/coro/rpc/GetKeyRange.h>
#include <oio/kinetic/coro/blob.h>

using oio::kinetic::blob::ListingBuilder;
using oio::kinetic::client::ClientInterface;
using oio::kinetic::client::ClientFactory;
using oio::kinetic::client::Sync;
using oio::kinetic::rpc::GetKeyRange;

namespace blob = ::oio::api::blob;

class KineticListing : public blob::Listing {
    friend class ListingBuilder;

  public:
    KineticListing(): clients(), name(), items(), next_item{0} { }

    ~KineticListing() { }

    // Concurrently get the lists on each node
    // TODO limit the parallelism to N (configurable) items
    blob::Listing::Status Prepare() override {
        items.clear();
        std::vector<std::shared_ptr<GetKeyRange>> ops;
        std::vector<std::shared_ptr<Sync>> syncs;

        for (auto cli: clients) {
            auto gkr = new GetKeyRange;
            gkr->Start(name + "-#");
            gkr->End(name + "-X");
            gkr->IncludeStart(true);
            gkr->IncludeEnd(false);
            ops.emplace_back(gkr);
            syncs.emplace_back(cli->Start(gkr));
        }
        for (auto sync: syncs)
            sync->Wait();

        for (unsigned int i = 0; i < ops.size(); ++i) {
            std::vector<std::string> keys;
            ops[i]->Steal(keys);
            for (auto &k: keys) {
                // We try here to avoid copies
                std::string s;
                s.swap(k);
                items.emplace_back(i, std::move(s));
                assert(s.size() == 0);
                assert(k.size() == 0);
            }
        }

        if (items.empty())
            return blob::Listing::Status::NotFound;
        return blob::Listing::Status::OK;
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

void ListingBuilder::Name(const std::string &n) {
    name.assign(n);
}

void ListingBuilder::Name(const char *n) {
    assert(n != nullptr);
    return Name(std::string(n));
}

void ListingBuilder::Target(const std::string &to) {
    targets.insert(to);
}

void ListingBuilder::Target(const char *to) {
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
