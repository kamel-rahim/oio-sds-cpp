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

#include <glog/logging.h>
#include <gflags/gflags.h>
#include <gtest/gtest.h>

#include <string>

#include "utils/macros.h"
#include "utils/utils.h"
#include "oio/mem/blob.h"
#include "tests/common/BlobTestSuite.h"

using oio::api::Status;
using oio::api::Cause;
using oio::mem::blob::Cache;
using oio::mem::blob::UploadBuilder;
using oio::mem::blob::RemovalBuilder;
using oio::mem::blob::DownloadBuilder;

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
