/** Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#include <vector>
#include <memory>
#include <cassert>

#include <gflags/gflags.h>
#include <glog/logging.h>
#include <gtest/gtest.h>

#include <utils/utils.h>
#include <oio/kinetic/coro/rpc/Put.h>
#include <oio/kinetic/coro/rpc/Get.h>
#include <oio/kinetic/coro/rpc/GetNext.h>
#include <oio/kinetic/coro/rpc/GetKeyRange.h>
#include <oio/kinetic/coro/rpc/GetLog.h>
#include "oio/kinetic/coro/client/CoroutineClient.h"
#include "oio/kinetic/coro/client/CoroutineClientFactory.h"

using oio::kinetic::client::ClientInterface;
using oio::kinetic::client::CoroutineClientFactory;
using oio::kinetic::client::Sync;
using oio::kinetic::rpc::Exchange;
using oio::kinetic::rpc::Put;
using oio::kinetic::rpc::Get;
using oio::kinetic::rpc::GetKeyRange;
using oio::kinetic::rpc::GetNext;
using oio::kinetic::rpc::GetLog;

DEFINE_string(url_device, "", "URL of the Kinetic device");

class ClientWrapper;

class FactoryWrapper {
  public:
    virtual ~FactoryWrapper() {}
    virtual ClientWrapper Get() = 0;
    virtual void Release(std::shared_ptr<ClientInterface>) = 0;
};

class ClientWrapper {
  private:
    FactoryWrapper *factory;
    std::shared_ptr<ClientInterface> client;
  public:
    ClientWrapper(FactoryWrapper *f, std::shared_ptr<ClientInterface> c)
            : factory{f}, client(c) {}
    ~ClientWrapper() { factory->Release(client); }
    std::shared_ptr<ClientInterface> operator* () { return client; }
};

class FactoryWrapperReuse : public FactoryWrapper {
  private:
    std::unique_ptr<CoroutineClientFactory> factory;
  public:
    FactoryWrapperReuse(): factory(nullptr) {
        factory.reset(new CoroutineClientFactory);
    }
    ClientWrapper Get() override {
        return ClientWrapper(this, factory->Get(FLAGS_url_device));
    }
    void Release(std::shared_ptr<ClientInterface> client UNUSED) override {}
};

class FactoryWrapperReset : public FactoryWrapper {
  private:
    std::unique_ptr<CoroutineClientFactory> factory;
    void reset () { factory.reset(new CoroutineClientFactory); }
  public:
    FactoryWrapperReset(): factory(nullptr) { reset(); }
    ClientWrapper Get() override {
        return ClientWrapper(this, factory->Get(FLAGS_url_device));
    }
    void Release(std::shared_ptr<ClientInterface> client UNUSED) override {
        reset();
    }
};

std::unique_ptr<FactoryWrapper> factory(nullptr);

TEST(Kinetic,Put) {
    auto client = factory->Get();
	Put put;
	put.Key("k");
	put.Value("v");
	put.PostVersion("0");
    (*client)->RPC(&put)->Wait();
	assert(put.Ok());
}

TEST(Kinetic,UploadSequential) {
    auto client = factory->Get();
    for (int i = 0; i < 5; ++i) {
		Put put;
		put.Key("k");
		put.Value("v");
		put.PostVersion("0");
        (*client)->RPC(&put)->Wait();
		assert(put.Ok());
	}
}

TEST(Kinetic,UploadInline) {
    auto client = factory->Get();

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

	auto op0 = (*client)->RPC(&put0);
	auto op1 = (*client)->RPC(&put1);
	auto op2 = (*client)->RPC(&put2);

	op0->Wait();
	op1->Wait();
	op2->Wait();

	assert(put0.Ok());
	assert(put1.Ok());
	assert(put2.Ok());
}

TEST(Kinetic,UploadArray) {
    auto client = factory->Get();

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
        auto pe = (*client)->RPC(p.get());
        ops.push_back(std::move(pe));
    }
    for (auto &op: ops)
        op->Wait();
    for (auto &p: puts)
        assert(p->Ok());
}

TEST(Kinetic,GetSingle) {
    auto client = factory->Get();
	Get get;
	get.Key("k");
	auto op = (*client)->RPC(&get);
	op->Wait();
	assert(get.Ok());
	std::vector<uint8_t> v;
	get.Steal(v);
}

TEST(Kinetic,GetKeyRange) {
    auto client = factory->Get();
	GetKeyRange op;
	op.Start("a");
	op.End("z");
	auto sync = (*client)->RPC(&op);
	sync->Wait();
	assert(op.Ok());
}

TEST(Kinetic,GetNext) {
    auto client = factory->Get();
	GetNext op;
	op.Key("a");
	auto sync = (*client)->RPC(&op);
	sync->Wait();
	assert(op.Ok());
}

TEST(Kinetic,GetLog) {
    auto client = factory->Get();

	GetLog op;
	auto sync = (*client)->RPC(&op);
	sync->Wait();
	assert(op.Ok());

	DLOG(INFO)
			<< " cpu=" << op.getCpu()
			<< " io=" << op.getIo()
			<< " space=" << op.getSpace()
			<< " temp=" << op.getTemp();
}

int main(int argc, char **argv) {
	gflags::ParseCommandLineFlags(&argc, &argv, true);
	google::InitGoogleLogging(argv[0]);
	::testing::InitGoogleTest(&argc, argv);
	FLAGS_logtostderr = true;

    if (FLAGS_url_device == "") {
        LOG(ERROR) << "Missing \"-url_device\" flags";
        return -1;
    }

    factory.reset(new FactoryWrapperReuse);
	int rc = RUN_ALL_TESTS();
    if (rc != 0) return rc;
    factory.reset(new FactoryWrapperReset);
    return RUN_ALL_TESTS();
}