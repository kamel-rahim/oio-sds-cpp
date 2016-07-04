/** Copyright (c) 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */
#include <glog/logging.h>
#include <oio/local/blob.h>

using oio::local::blob::UploadBuilder;

static std::string path("/tmp/blob");

static void test_download(void) {

}

static void test_upload(void) {
    UploadBuilder builder;
    builder.Path(path);
    auto ul = builder.Build();
    auto rc = ul->Prepare();
    if (rc == oio::api::blob::Upload::Status::OK) {
        ul->Write("0");
        ul->Write("1");
        ul->Write("2");
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
    return 0;
}