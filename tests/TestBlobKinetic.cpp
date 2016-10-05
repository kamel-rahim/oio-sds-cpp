/** Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#include <array>

#include <utils/macros.h>
#include <utils/utils.h>
#include "oio/kinetic/coro/client/ClientInterface.h"
#include "oio/kinetic/coro/client/CoroutineClientFactory.h"
#include "oio/api/blob.h"
#include "oio/kinetic/coro/blob.h"

using oio::kinetic::client::ClientFactory;
using oio::kinetic::client::CoroutineClientFactory;
using oio::kinetic::blob::UploadBuilder;
using oio::kinetic::blob::RemovalBuilder;
using oio::kinetic::blob::ListingBuilder;
using oio::kinetic::blob::DownloadBuilder;

namespace blob = ::oio::api::blob;
using blob::Upload;
using blob::Download;
using blob::Removal;
using blob::Status;

const char * envkey_URL = "OIO_KINETIC_URL";
const char * target = ::getenv(envkey_URL);

static void test_upload_empty (std::string chunkid, std::shared_ptr<ClientFactory> factory) {
    DLOG(INFO) << __FUNCTION__;
    std::array<uint8_t,8> buf;
    buf.fill('0');

    auto builder = UploadBuilder(factory);
    builder.BlockSize(1024*1024);
    builder.Name(chunkid);
    for (int i=0; i<5 ;++i)
        builder.Target(target);

    auto up = builder.Build();
    auto rc = up->Prepare();
    assert(rc == Status::OK);
    up->Commit();
}

static void test_upload_2blocks (std::string chunkid, std::shared_ptr<ClientFactory> factory) {
    DLOG(INFO) << __FUNCTION__;
    std::array<uint8_t,8192> buf;
    buf.fill('0');

    auto builder = UploadBuilder(factory);
    builder.BlockSize(1024*1024);
    builder.Name(chunkid);
    for (int i=0; i<5 ;++i)
        builder.Target(target);

    auto up = builder.Build();
    auto rc = up->Prepare();
    assert(rc == Status::OK);
    up->Write(buf.data(), buf.size());
    up->Write(buf.data(), buf.size());
    up->Commit();
}

static void test_listing (std::string chunkid, std::shared_ptr<ClientFactory> factory) {
    DLOG(INFO) << __FUNCTION__;
    auto builder = ListingBuilder(factory);
    builder.Name(chunkid);
    builder.Target(target);
    builder.Target(target);
    builder.Target(target);
    auto list = builder.Build();
    auto rc = list->Prepare();
    assert(rc == Status::OK);
    std::string id, key;
    while (list->Next(id, key)) {
        DLOG(INFO) << "id:" << id << " key:" << key;
    }
}

static void test_download (std::string chunkid, std::shared_ptr<ClientFactory> factory) {
    DLOG(INFO) << __FUNCTION__;
    auto builder = DownloadBuilder(factory);
    builder.Target(target);
    builder.Name(chunkid);
    auto dl = builder.Build();
    auto rc = dl->Prepare();
    assert(rc == Status::OK);
    DLOG(INFO) << "DL ready, chunk found, eof " << dl->IsEof();

    while (!dl->IsEof()) {
        std::vector<uint8_t> buf;
        dl->Read(buf);
    }
}

static void test_removal (std::string chunkid, std::shared_ptr<ClientFactory> factory) {
    DLOG(INFO) << __FUNCTION__;
    auto builder = RemovalBuilder(factory);
    builder.Target(target);
    builder.Name(chunkid);
    auto rem = builder.Build();
    auto rc = rem->Prepare();
    assert (rc == Status::OK);
    rem->Commit();
}

static void test_cycle (std::shared_ptr<ClientFactory> factory) {
    std::string chunkid;
    append_string_random(chunkid, 32, "0123456789ABCDEF");

    test_upload_empty(chunkid, factory);
    test_listing(chunkid, factory);
    test_download(chunkid, factory);
    test_removal(chunkid, factory);

    test_upload_2blocks(chunkid, factory);
    test_listing(chunkid, factory);
    test_download(chunkid, factory);
    test_removal(chunkid, factory);
}

int main (int argc UNUSED, char **argv) {
    google::InitGoogleLogging(argv[0]);
    FLAGS_logtostderr = true;

    if (target == nullptr) {
        LOG(ERROR) << "Missing " << envkey_URL << " variable in environment";
        return -1;
    }

    std::shared_ptr<ClientFactory> factory(new CoroutineClientFactory);
    test_cycle(factory);
    test_cycle(factory);
    return 0;
}