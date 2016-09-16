/** Copyright (c) 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */
#include <glog/logging.h>
#include <oio/local/blob.h>
#include <utils/utils.h>

using oio::local::blob::UploadBuilder;
using oio::local::blob::DownloadBuilder;
using oio::local::blob::RemovalBuilder;

static std::string path("/tmp/blob");

static void test_removal(void) {
    RemovalBuilder builder;
    builder.Path(path);
    auto rem = builder.Build();
    auto rc = rem->Prepare();
    if (rc == oio::api::blob::Removal::Status::OK) {
        if (!rem->Commit())
            abort();
    } else {
        rem->Abort();
    }
}

static void test_download(void) {
    DownloadBuilder builder;
    builder.Path(path);
    auto dl = builder.Build();
    auto rc = dl->Prepare();
    if (rc == oio::api::blob::Download::Status::OK) {
        while (!dl->IsEof()) {
            std::vector<uint8_t> buf;
            int32_t r = dl->Read(buf);
            (void) r;
        }
    }
}

static void test_upload(void) {
    UploadBuilder builder;
    builder.Path(path);
    auto ul = builder.Build();
    auto rc = ul->Prepare();
    if (rc == oio::api::blob::Upload::Status::OK) {
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
    (void) argc;
    google::InitGoogleLogging(argv[0]);
    FLAGS_logtostderr = true;
    test_upload();
    test_download();
    test_removal();
    return 0;
}