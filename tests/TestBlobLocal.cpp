/**
 * Copyright (c) 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/
 */

#include <gtest/gtest.h>

#include <utils/macros.h>
#include <utils/utils.h>
#include <oio/local/blob.h>

using oio::local::blob::UploadBuilder;
using oio::local::blob::DownloadBuilder;
using oio::local::blob::RemovalBuilder;
using oio::api::blob::Status;

DEFINE_string(string_path, "/tmp/blob", "Path of the random file");

TEST(Local,Removal) {
    RemovalBuilder builder;
    builder.Path(FLAGS_string_path);
    auto rem = builder.Build();
    auto rc = rem->Prepare();
    if (rc == Status::OK) {
        if (!rem->Commit())
            abort();
    } else {
        rem->Abort();
    }
}

TEST(Local,Download) {
    DownloadBuilder builder;
    builder.Path(FLAGS_string_path);
    auto dl = builder.Build();
    auto rc = dl->Prepare();
    if (rc == Status::OK) {
        while (!dl->IsEof()) {
            std::vector<uint8_t> buf;
            int32_t r = dl->Read(buf);
            (void) r;
        }
    }
}

TEST(Local,Upload) {
    UploadBuilder builder;
    builder.Path(FLAGS_string_path);
    auto ul = builder.Build();
    auto rc = ul->Prepare();
    if (rc == Status::OK) {
        std::string blah;
        append_string_random(blah, 1024*1024, "0123456789ABCDEF");
        for (int i=0; i<8 ;++i)
            ul->Write(blah);
        ul->Commit();
    } else {
        ul->Abort();
    }
}

int main(int argc, char **argv) {
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    google::InitGoogleLogging(argv[0]);
    ::testing::InitGoogleTest(&argc, argv);
    FLAGS_logtostderr = true;
    return RUN_ALL_TESTS();
}