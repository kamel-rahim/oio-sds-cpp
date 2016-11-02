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
