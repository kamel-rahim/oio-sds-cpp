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

#include <fcntl.h>

#include <gtest/gtest.h>

#include "utils/macros.h"
#include "utils/utils.h"
#include "oio/local/blob.h"
#include "tests/common/BlobTestSuite.h"

using oio::local::blob::UploadBuilder;
using oio::local::blob::DownloadBuilder;
using oio::local::blob::RemovalBuilder;
using oio::api::blob::Cause;

DEFINE_string(test_file_path,
              "/tmp/blob-",
              "Path of the random file");

#define ASSERT_ABSENT(path) do { \
    UploadBuilder builder; \
    builder.Path(path); \
    ASSERT_FALSE(isPresent(path)); \
    ASSERT_FALSE(isPresent(PathPending())); \
} while (0)

static bool isPresent(const std::string &path) {
    struct stat64 st;
    return 0 == ::stat64(path.c_str(), &st);
}

static int getMode(const std::string &path) {
    struct stat64 st;
    st.st_mode = 0;
    ::stat64(path.c_str(), &st);
    st.st_mode = st.st_mode & ~(S_IFMT);
    return st.st_mode;
}

class LocalBlobOpsFactory : public BlobOpsFactory {
 protected:
    std::string path;
 public:
    ~LocalBlobOpsFactory() {}
    explicit LocalBlobOpsFactory(const std::string &p): path(p) {}

    std::unique_ptr<oio::api::blob::Upload> Upload(mode_t f, mode_t d) {
        UploadBuilder op;
        op.Path(path);
        op.FileMode(f);
        op.DirMode(d);
        return op.Build();
    }

    std::unique_ptr<oio::api::blob::Upload> Upload() override {
        return Upload(0644, 0755);
    }

    std::unique_ptr<oio::api::blob::Download> Download() override {
        DownloadBuilder op;
        op.Path(path);
        return op.Build();
    }

    std::unique_ptr<oio::api::blob::Removal> Removal() override {
        RemovalBuilder op;
        op.Path(path);
        return op.Build();
    }
};

class LocalBlobTestSuite : public BlobTestSuite {
 protected:
    std::string test_file_path;
 protected:
    ~LocalBlobTestSuite() override {}

    LocalBlobTestSuite(): ::BlobTestSuite() {}

    void SetUp() override {
        test_file_path.assign(FLAGS_test_file_path);
        append_string_random(&test_file_path, 16, "0123456789ABCDEF");
        this->factory_ = new LocalBlobOpsFactory(test_file_path);
    }

    void TearDown() override {
        delete this->factory_;
        this->factory_ = nullptr;

        // check all the files (pending, final) are absent
        ASSERT_ABSENT(test_file_path);
    }

    std::unique_ptr<oio::api::blob::Upload> Upload(mode_t f, mode_t d) {
        UploadBuilder op;
        op.Path(test_file_path);
        op.FileMode(f);
        op.DirMode(d);
        return op.Build();
    }

    std::string PathPending() const {
        return test_file_path + ".pending";
    }
};

DECLARE_BLOBTESTSUITE(LocalBlobTestSuite);

// Test Prepare() when the temp file already exists
TEST_F(LocalBlobTestSuite, UploadAlreadyPending) {
    auto ul = Upload(0610, 0755);

    int fd = ::open(PathPending().c_str(), O_CREAT | O_EXCL | O_WRONLY, 0600);
    ASSERT_GE(fd, 0);
    ASSERT_EQ(getMode(PathPending()), 0600);
    ASSERT_FALSE(isPresent(test_file_path));

    auto rc = ul->Prepare();
    ASSERT_FALSE(rc.Ok());
    ASSERT_EQ(rc.Why(), Cause::Already);
    ASSERT_EQ(getMode(PathPending()), 0600);
    ASSERT_FALSE(isPresent(test_file_path));

    rc = ul->Abort();
    ASSERT_FALSE(rc.Ok());
    ASSERT_EQ(getMode(PathPending()), 0600);
    ASSERT_FALSE(isPresent(test_file_path));

    // let a clean env after the test case
    ::close(fd);
    ::unlink(PathPending().c_str());
}

// Test Prepare() when the final file already exists
TEST_F(LocalBlobTestSuite, UploadAlreadyFinal) {
    auto ul = Upload(0610, 0755);

    int fd = ::open(test_file_path.c_str(), O_CREAT | O_EXCL | O_WRONLY,
                    0600);
    ASSERT_GE(fd, 0);
    ASSERT_FALSE(isPresent(PathPending()));
    ASSERT_EQ(getMode(test_file_path), 0600);

    auto rc = ul->Prepare();
    ASSERT_FALSE(rc.Ok());
    ASSERT_EQ(rc.Why(), Cause::Already);
    ASSERT_FALSE(isPresent(PathPending()));
    ASSERT_EQ(getMode(test_file_path), 0600);

    rc = ul->Abort();
    ASSERT_FALSE(rc.Ok());
    OP_LOOP_ON(ul, Abort, InternalError, InternalError);
    ASSERT_FALSE(isPresent(PathPending()));
    ASSERT_EQ(getMode(test_file_path), 0600);

    // let a clean env after the test case
    ::close(fd);
    ::unlink(test_file_path.c_str());
}

int main(int argc, char **argv) {
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    google::InitGoogleLogging(argv[0]);
    ::testing::InitGoogleTest(&argc, argv);
    FLAGS_logtostderr = true;
    return RUN_ALL_TESTS();
}
