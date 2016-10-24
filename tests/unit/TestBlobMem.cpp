/**
 * Copyright (c) 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/
 */

#include <glog/logging.h>
#include <gflags/gflags.h>
#include <gtest/gtest.h>

#include <string>

#include "utils/macros.h"
#include "utils/utils.h"
#include "oio/mem/blob.h"
#include "tests/common/BlobTestSuite.h"

using oio::mem::blob::Cache;
using oio::mem::blob::UploadBuilder;
using oio::mem::blob::RemovalBuilder;
using oio::mem::blob::DownloadBuilder;
using oio::api::blob::Status;
using oio::api::blob::Cause;

class MemBlobOpsFactory : public BlobOpsFactory {
 private:
    std::shared_ptr<Cache> cache_;
    std::string blobname;

 public:
    ~MemBlobOpsFactory() override {}

    explicit MemBlobOpsFactory(const std::string &n): cache_{new Cache},
                                                      blobname(n) {}

    std::unique_ptr<oio::api::blob::Upload> Upload() override {
        UploadBuilder op(cache_);
        op.Name(blobname);
        return op.Build();
    }
    std::unique_ptr<oio::api::blob::Download> Download() override {
        DownloadBuilder op(cache_);
        op.Name(blobname);
        return op.Build();
    }
    std::unique_ptr<oio::api::blob::Removal> Removal() override {
        RemovalBuilder op(cache_);
        op.Name(blobname);
        return op.Build();
    }
};

class MemBlobTestSuite : public BlobTestSuite {
 protected:
    ~MemBlobTestSuite() {}

    MemBlobTestSuite(): ::BlobTestSuite() {}

    void SetUp() override {
        std::string blobname("blob-");
        append_string_random(&blobname, 8, random_chars);
        this->factory_ = new MemBlobOpsFactory(blobname);
    }

    void TearDown() override {
        delete this->factory_;
        this->factory_ = nullptr;
    }
};

DECLARE_BLOBTESTSUITE(MemBlobTestSuite);

int main(int argc, char **argv) {
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    google::InitGoogleLogging(argv[0]);
    ::testing::InitGoogleTest(&argc, argv);
    FLAGS_logtostderr = true;
    return RUN_ALL_TESTS();
}
