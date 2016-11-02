/**
 * This file is part of the test tools for the OpenIO client libraries
 * Copyright (C) 2016 OpenIO SAS
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3.0 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.
 */

#include <array>

#include "utils/macros.h"
#include "utils/utils.h"
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
using blob::Cause;

const char *envkey_URL = "OIO_KINETIC_URL";
const char *target = ::getenv(envkey_URL);

static void
test_upload_empty(std::string chunkid, std::shared_ptr<ClientFactory> factory) {
    DLOG(INFO) << __FUNCTION__;
    std::array<uint8_t, 8> buf;
    buf.fill('0');

    auto builder = UploadBuilder(factory);
    builder.BlockSize(1024 * 1024);
    builder.Name(chunkid);
    for (int i = 0; i < 5; ++i)
        builder.Target(target);

    auto up = builder.Build();
    auto rc = up->Prepare();
    assert(rc.Ok());
    up->Commit();
}

static void test_upload_2blocks(std::string chunkid,
        std::shared_ptr<ClientFactory> factory) {
    DLOG(INFO) << __FUNCTION__;
    std::array<uint8_t, 8192> buf;
    buf.fill('0');

    auto builder = UploadBuilder(factory);
    builder.BlockSize(1024 * 1024);
    builder.Name(chunkid);
    for (int i = 0; i < 5; ++i)
        builder.Target(target);

    auto up = builder.Build();
    auto rc = up->Prepare();
    assert(rc.Ok());
    up->Write(buf.data(), buf.size());
    up->Write(buf.data(), buf.size());
    up->Commit();
}

static void
test_listing(std::string chunkid, std::shared_ptr<ClientFactory> factory) {
    DLOG(INFO) << __FUNCTION__;
    auto builder = ListingBuilder(factory);
    builder.Name(chunkid);
    builder.Target(target);
    builder.Target(target);
    builder.Target(target);
    auto list = builder.Build();
    auto rc = list->Prepare();
    assert(rc.Ok());
    std::string id, key;
    while (list->Next(&id, &key)) {
        DLOG(INFO) << "id:" << id << " key:" << key;
    }
}

static void
test_download(std::string chunkid, std::shared_ptr<ClientFactory> factory) {
    DLOG(INFO) << __FUNCTION__;
    auto builder = DownloadBuilder(factory);
    builder.Target(target);
    builder.Name(chunkid);
    auto dl = builder.Build();
    auto rc = dl->Prepare();
    assert(rc.Ok());
    DLOG(INFO) << "DL ready, chunk found, eof " << dl->IsEof();

    while (!dl->IsEof()) {
        std::vector<uint8_t> buf;
        dl->Read(&buf);
    }
}

static void
test_removal(std::string chunkid, std::shared_ptr<ClientFactory> factory) {
    DLOG(INFO) << __FUNCTION__;
    auto builder = RemovalBuilder(factory);
    builder.Target(target);
    builder.Name(chunkid);
    auto rem = builder.Build();
    auto rc = rem->Prepare();
    assert(rc.Ok());
    rem->Commit();
}

static void test_cycle(std::shared_ptr<ClientFactory> factory) {
    std::string chunkid;
    append_string_random(&chunkid, 32, "0123456789ABCDEF");

    test_upload_empty(chunkid, factory);
    test_listing(chunkid, factory);
    test_download(chunkid, factory);
    test_removal(chunkid, factory);

    test_upload_2blocks(chunkid, factory);
    test_listing(chunkid, factory);
    test_download(chunkid, factory);
    test_removal(chunkid, factory);
}

int main(int argc UNUSED, char **argv) {
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
