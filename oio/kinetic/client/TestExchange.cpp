/** Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#include <vector>
#include <memory>
#include <cassert>

#include <glog/logging.h>

#include <utils/utils.h>
#include <oio/kinetic/rpc/Put.h>
#include <oio/kinetic/rpc/Get.h>
#include <oio/kinetic/rpc/GetNext.h>
#include <oio/kinetic/rpc/GetKeyRange.h>
#include "CoroutineClient.h"
#include "CoroutineClientFactory.h"

using oio::kinetic::client::ClientInterface;
using oio::kinetic::client::CoroutineClientFactory;
using oio::kinetic::client::Sync;
using oio::kinetic::rpc::Exchange;
using oio::kinetic::rpc::Put;
using oio::kinetic::rpc::Get;
using oio::kinetic::rpc::GetKeyRange;
using oio::kinetic::rpc::GetNext;

static void test_single_upload(
        std::shared_ptr<ClientInterface> client) noexcept {
    Put put;
    put.Key("k");
    put.Value("v");
    put.PostVersion("0");
    client->Start(&put)->Wait();
    assert(put.Ok());
}

static void test_sequential_uploads(
        std::shared_ptr<ClientInterface> client) noexcept {
    for (int i = 0; i < 5; ++i) {
        Put put;
        put.Key("k");
        put.Value("v");
        put.PostVersion("0");
        client->Start(&put)->Wait();
        assert(put.Ok());
    }
}

static void test_multi_upload(
        std::shared_ptr<ClientInterface> client) noexcept {
    Put put0;
    put0.Key("k");
    put0.Value("v");
    put0.Key("0");
    Put put1;
    put1.Key("k");
    put1.Value("v");
    put1.Key("0");
    Put put2;
    put2.Key("k");
    put2.Value("v");
    put2.Key("0");

    auto op0 = client->Start(&put0);
    auto op1 = client->Start(&put1);
    auto op2 = client->Start(&put2);

    op0->Wait();
    op1->Wait();
    op2->Wait();

    assert(put0.Ok());
    assert(put1.Ok());
    assert(put2.Ok());
}

static void test_array_upload(
        std::shared_ptr<ClientInterface> client) noexcept {
    std::vector<std::shared_ptr<Exchange>> puts;
    for (int i = 0; i < 5; ++i) {
        auto ex = new Put();
        ex->Key("k");
        ex->Value("v");
        ex->PostVersion("0");
        puts.emplace_back(ex);
    }

    std::vector<std::shared_ptr<Sync>> ops;
    for (auto &p: puts) {
        auto pe = client->Start(p.get());
        ops.push_back(std::move(pe));
    }
    for (auto &op: ops)
        op->Wait();
    for (auto &p: puts)
        assert(p->Ok());
}

static void test_single_get(std::shared_ptr<ClientInterface> client) noexcept {
    Get get;
    get.Key("k");
    auto op = client->Start(&get);
    op->Wait();
    assert(get.Ok());

    std::vector<uint8_t> v;
    get.Steal(v);
}

static void test_single_keyrange(
        std::shared_ptr<ClientInterface> client) noexcept {
    GetKeyRange op;
    op.Start("a");
    op.End("z");
    auto sync = client->Start(&op);
    sync->Wait();
    assert(op.Ok());
}

static void test_single_getnext(
        std::shared_ptr<ClientInterface> client) noexcept {
    GetNext op;
    op.Key("a");
    auto sync = client->Start(&op);
    sync->Wait();
    assert(op.Ok());
}

int main(int argc UNUSED, char **argv) {
    google::InitGoogleLogging(argv[0]);
    FLAGS_logtostderr = true;

    const char *envkey_URL = "OIO_KINETIC_URL";
    const char *device = ::getenv(envkey_URL);
    if (device == nullptr) {
        LOG(ERROR) << "Missing " << envkey_URL << " variable in environment";
        return -1;
    }
    CoroutineClientFactory factory;

    do { // one batch reusing the client
        auto client = factory.Get(device);
        test_single_upload(client);
        test_sequential_uploads(client);
        test_multi_upload(client);
        test_array_upload(client);
        test_single_get(client);
        test_single_keyrange(client);
        test_single_getnext(client);
    } while (0);

    do { // one batch with a new client each time
        test_single_upload(factory.Get(device));
        test_sequential_uploads(factory.Get(device));
        test_multi_upload(factory.Get(device));
        test_array_upload(factory.Get(device));
        test_single_get(factory.Get(device));
        test_single_keyrange(factory.Get(device));
        test_single_getnext(factory.Get(device));
    } while (0);

    return 0;
}