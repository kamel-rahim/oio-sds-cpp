/**
 * Copyright (c) 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/
 */

#include <fcntl.h>

#include <gtest/gtest.h>

#include <utils/macros.h>
#include <utils/utils.h>
#include <oio/local/blob.h>

using oio::local::blob::UploadBuilder;
using oio::local::blob::DownloadBuilder;
using oio::local::blob::RemovalBuilder;
using oio::api::blob::Cause;

DEFINE_string(test_file_path, "/tmp/blob", "Path of the random file");
DEFINE_uint64(test_file_size, 8*1024*1024, "Size of the sample file");
DEFINE_uint64(test_batch_size, 1024*1024, "Size of the batches to write in the sample file");

#define ASSERT_ABSENT(Builder) do { \
    ASSERT_FALSE(isPresent(FLAGS_test_file_path)); \
    ASSERT_FALSE(isPresent(Builder.PathPending())); \
} while (0)

static bool isPresent(const std::string &path) {
    struct stat64 st;
    return 0 == ::stat64(path.c_str(), &st);
}

static mode_t getMode(const std::string &path) {
    struct stat64 st;
    st.st_mode = 0;
    ::stat64(path.c_str(), &st);
    st.st_mode = st.st_mode & ~(S_IFMT);
    return st.st_mode;
}

static void test_remove (void) {
    RemovalBuilder builder;
    builder.Path(FLAGS_test_file_path);
    auto rem = builder.Build();
    auto rc = rem->Prepare();
    ASSERT_TRUE(rc.Ok());
    rc = rem->Commit();
    ASSERT_TRUE(rc.Ok());
}

static void test_download (void) {
    DownloadBuilder builder;
    builder.Path(FLAGS_test_file_path);
    auto dl = builder.Build();
    auto rc = dl->Prepare();
    ASSERT_TRUE(rc.Ok());

    while (!dl->IsEof()) {
        std::vector<uint8_t> buf;
        int32_t r = dl->Read(buf);
        (void) r;
    }
}

static void test_upload (void) {
    UploadBuilder builder;
    builder.Path(FLAGS_test_file_path);
    auto ul = builder.Build();
    auto rc = ul->Prepare();
    ASSERT_TRUE(rc.Ok());


    std::string blah;
    uint64_t max{FLAGS_test_file_size};
    append_string_random(blah, FLAGS_test_batch_size, "0123456789ABCDEF");
    for (uint64_t written = 0; written < max;) {
        if (written + blah.size() > max)
            blah.resize(blah.size() - written);
        ul->Write(blah);
        written += blah.size();
    }

    rc = ul->Commit();
    ASSERT_TRUE(rc.Ok());
}

TEST(Local,Cycle) {
    test_upload();
    test_download();
    test_remove();
}

// Test Abort() before Prepare()
TEST(Local,UploadInit) {
    UploadBuilder builder;
    builder.Path(FLAGS_test_file_path);
    auto ul = builder.Build();
    ASSERT_ABSENT(builder);
    auto rc = ul->Abort();
    ASSERT_TRUE(rc.Ok());
    ASSERT_ABSENT(builder);
}

// Test Abort() after Prepare()
TEST(Local,UploadAbort) {
    const mode_t mode{0615}; // dummy mode different from the default
    UploadBuilder builder;
    builder.Path(FLAGS_test_file_path);
    builder.FileMode(mode);
    auto ul = builder.Build();

    auto rc = ul->Prepare();
    ASSERT_TRUE(rc.Ok());
    ASSERT_EQ(getMode(builder.PathPending()), mode);
    ASSERT_FALSE(isPresent(FLAGS_test_file_path));

    rc = ul->Abort();
    ASSERT_TRUE(rc.Ok());
    ASSERT_ABSENT(builder);
}

// Test Prepare() when the temp file already exists
TEST(Local,UploadAlreadyPending) {
    UploadBuilder builder;
    builder.Path(FLAGS_test_file_path);
    builder.FileMode(0610);
    auto ul = builder.Build();

    int fd = ::open(builder.PathPending().c_str(), O_CREAT|O_EXCL|O_WRONLY, 0600);
    ASSERT_GE(fd,0);
    ASSERT_EQ(getMode(builder.PathPending()), 0600);
    ASSERT_FALSE(isPresent(FLAGS_test_file_path));

    auto rc = ul->Prepare();
    ASSERT_FALSE(rc.Ok());
    ASSERT_EQ(rc.Why(), Cause::Already);
    ASSERT_EQ(getMode(builder.PathPending()), 0600);
    ASSERT_FALSE(isPresent(FLAGS_test_file_path));

    rc = ul->Abort();
    ASSERT_TRUE(rc.Ok());
    ASSERT_EQ(getMode(builder.PathPending()), 0600);
    ASSERT_FALSE(isPresent(FLAGS_test_file_path));

    // let a clean env after the test case
    ::close(fd);
    ::unlink(builder.PathPending().c_str());
    ASSERT_ABSENT(builder);
}

// Test Prepare() when the final file already exists
TEST(Local,UploadAlreadyFinal) {
    UploadBuilder builder;
    builder.Path(FLAGS_test_file_path);
    builder.FileMode(0610);
    auto ul = builder.Build();

    int fd = ::open(FLAGS_test_file_path.c_str(), O_CREAT|O_EXCL|O_WRONLY, 0600);
    ASSERT_GE(fd,0);
    ASSERT_FALSE(isPresent(builder.PathPending()));
    ASSERT_EQ(getMode(FLAGS_test_file_path), 0600);

    auto rc = ul->Prepare();
    ASSERT_FALSE(rc.Ok());
    ASSERT_EQ(rc.Why(), Cause::Already);
    ASSERT_FALSE(isPresent(builder.PathPending()));
    ASSERT_EQ(getMode(FLAGS_test_file_path), 0600);

    rc = ul->Abort();
    ASSERT_TRUE(rc.Ok());
    ASSERT_FALSE(isPresent(builder.PathPending()));
    ASSERT_EQ(getMode(FLAGS_test_file_path), 0600);

    // let a clean env after the test case
    ::close(fd);
    ::unlink(FLAGS_test_file_path.c_str());
    ASSERT_ABSENT(builder);
}

int main(int argc, char **argv) {
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    google::InitGoogleLogging(argv[0]);
    ::testing::InitGoogleTest(&argc, argv);
    FLAGS_logtostderr = true;
    append_string_random(FLAGS_test_file_path, 16, "0123456789ABCDEF");
    return RUN_ALL_TESTS();
}