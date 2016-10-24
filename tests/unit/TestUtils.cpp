/**
 * Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/
 */

#include <gtest/gtest.h>

#include <array>

#include "utils/macros.h"
#include "utils/utils.h"
#include "oio/api/blob.h"

using oio::api::blob::Status;
using oio::api::blob::Cause;
using oio::api::blob::Errno;

TEST(Utils, Checksum) {
    auto checksum = checksum_make_MD5();
    checksum->Update("JFS", 3);
    auto h = checksum->Final();
    ASSERT_EQ("26d80754ef7129ffae60b3fe018ba53a", h);
}

TEST(Utils, bin2hex) {
    std::array<uint8_t, 3> bin;
    bin.fill(0);
    ASSERT_EQ("000000", bin2hex(bin.data(), bin.size()));
    bin.fill(1);
    ASSERT_EQ("010101", bin2hex(bin.data(), bin.size()));
    bin.fill(255);
    ASSERT_EQ("ffffff", bin2hex(bin.data(), bin.size()));
}

Status _gen(int err) { return Errno(err); }

TEST(Api, Status) {
    auto rc = _gen(ENOENT);
    LOG(INFO) << "Status " << rc.Name() << "/" << rc.Message();
    ASSERT_FALSE(rc.Ok());
    ASSERT_TRUE(_gen(0).Ok());
    ASSERT_FALSE(_gen(ENOENT).Ok());
    ASSERT_EQ(_gen(ENOENT).Why(), Cause::NotFound);
    ASSERT_EQ(_gen(ENOTDIR).Why(), Cause::Forbidden);
    ASSERT_EQ(_gen(EPERM).Why(), Cause::Forbidden);
    ASSERT_EQ(_gen(EACCES).Why(), Cause::Forbidden);
}

int main(int argc UNUSED, char **argv) {
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    google::InitGoogleLogging(argv[0]);
    ::testing::InitGoogleTest(&argc, argv);
    FLAGS_logtostderr = true;
    return RUN_ALL_TESTS();
}
