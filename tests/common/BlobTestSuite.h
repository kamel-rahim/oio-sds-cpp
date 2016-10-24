/**
 * Copyright (c) 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/
 */

#ifndef TESTS_COMMON_BLOBTESTSUITE_H_
#define TESTS_COMMON_BLOBTESTSUITE_H_

#include <glog/logging.h>
#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "oio/api/blob.h"

const char *random_chars =
        "0123456789"
                "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                "abcdefghijklmnopqrstuvwxyz";
/**
 * Generates operations on blobs.
 * Each operation will be used only once in a test case.
 *
 * Each test should describe the sequence of operations it will call from the
 * factory, so that each Factory implementation can generate the operation with
 * the appropriate internal state.
 */
class BlobOpsFactory {
 public:
    virtual ~BlobOpsFactory() {}
    virtual std::unique_ptr<oio::api::blob::Upload> Upload() = 0;
    virtual std::unique_ptr<oio::api::blob::Download> Download() = 0;
    virtual std::unique_ptr<oio::api::blob::Removal> Removal() = 0;
};

DEFINE_uint64(test_file_size,
              8 * 1024 * 1024,
              "Size of the sample file");

DEFINE_uint64(test_batch_size,
              1024 * 1024,
              "Size of the batches to write in the sample file");

#define OP_LOOP_ON(op, Step, head, tail) do { \
        auto rc = op->Step(); \
        ASSERT_EQ(rc.Why(), oio::api::blob::Cause::head); \
        for (int i=0; i < 2; ++i) { \
            rc = op->Step(); \
            ASSERT_EQ(rc.Why(), oio::api::blob::Cause::tail); \
        } \
    } while (0)

#define LOOP_ON(Step, head, tail) OP_LOOP_ON(op, Step, head, tail)

class BlobTestSuite : public ::testing::Test {
 protected:
    BlobOpsFactory *factory_;

    ~BlobTestSuite() override {}

    BlobTestSuite(): ::testing::Test(), factory_{nullptr} {}

    /* helpers, just for more compact code in the test cases */

    std::unique_ptr<oio::api::blob::Upload> Upload() {
        return this->factory_->Upload();
    }
    std::unique_ptr<oio::api::blob::Download> Download() {
        return this->factory_->Download();
    }
    std::unique_ptr<oio::api::blob::Removal> Removal() {
        return this->factory_->Removal();
    }

    /**
     * Test a complete functional cycle made of uploads, downloads, removals.
     * All the operations are called on the same content.
     */
    void test_cycle() {
        uint64_t max{FLAGS_test_file_size};

        do {
            auto ul = Upload();
            auto rc = ul->Prepare();
            ASSERT_TRUE(rc.Ok());

            std::string blah;
            append_string_random(&blah, FLAGS_test_batch_size,
                                 random_chars);
            for (uint64_t written = 0; written < max;) {
                if (written + blah.size() > max)
                    blah.resize(blah.size() - written);
                ul->Write(blah);
                written += blah.size();
            }

            rc = ul->Commit();
            ASSERT_TRUE(rc.Ok());
        } while (0);

        // Now download the content, check the size
        do {
            uint64_t sum{0};
            auto dl0 = Download();
            auto rc = dl0->Prepare();
            ASSERT_TRUE(rc.Ok());
            while (!dl0->IsEof()) {
                std::vector<uint8_t> buf;
                int32_t nbread = dl0->Read(&buf);
                ASSERT_GE(nbread, 0);
                ASSERT_EQ(static_cast<uint64_t>(nbread), buf.size());
                sum += nbread;
            }
            ASSERT_EQ(sum, max);
        } while (0);

        // A Removal's Prepare must succeed and abort must let in in place
        do {
            auto op = Removal();
            auto rc = op->Prepare();
            ASSERT_TRUE(rc.Ok());
            rc = op->Abort();
            ASSERT_TRUE(rc.Ok());
        } while (0);

        // A Removal must succeed
        do {
            auto op = Removal();
            auto rc = op->Prepare();
            ASSERT_TRUE(rc.Ok());
            rc = op->Commit();
            ASSERT_TRUE(rc.Ok());
        } while (0);

        // And a subsequent removal fail because of the blob not found
        do {
            auto op = Removal();
            auto rc = op->Prepare();
            ASSERT_FALSE(rc.Ok());
            ASSERT_EQ(rc.Why(), oio::api::blob::Cause::NotFound);
        } while (0);
    }

    /**
     * Test Abort() without any Prepare()
     * All the calls to Abort() must fail, and the internal state of the upload
     * must not change. A subsequent Prepare() must succeed.
     */
    void test_upload_init_abort() {
        auto op = Upload();
        LOOP_ON(Abort, InternalError, InternalError);
        LOOP_ON(Prepare, OK, InternalError);
        LOOP_ON(Abort, OK, InternalError);
    }

    /**
     * Test Abort() after Prepare() on an upload.
     */
    void test_upload_prepare_abort() {
        auto op = Upload();
        LOOP_ON(Prepare, OK, InternalError);
        LOOP_ON(Abort, OK, InternalError);
    }

    /**
     * Test a Prepare() then a Commit() on a removal and a content that
     * doesn't exist
     */
    void test_remove_not_found_and_commit() {
        auto op = Removal();
        LOOP_ON(Prepare, NotFound, NotFound);
        LOOP_ON(Commit, InternalError, InternalError);
    }

    /**
     * Test a Prepare() then a Abort() on a removal and a content that doesn't
     * exist.
     */
    void test_remove_not_found_and_abort() {
        auto op = Removal();
        LOOP_ON(Prepare, NotFound, NotFound);
        LOOP_ON(Abort, InternalError, InternalError);
    }

    /**
     * Test Abort() (without Prepare()) on a removal.
     */
    void test_remove_abort() {
        auto op = Removal();
        LOOP_ON(Abort, InternalError, InternalError);
    }

    /**
     * Test Commit() (without Prepare()) on a removal.
     */
    void test_remove_commit() {
        auto op = Removal();
        LOOP_ON(Commit, InternalError, InternalError);
    }

    /**
     * Test Prepare() on a download and a content that doesn't exist.
     */
    void test_download_not_found() {
        auto op = Download();
        LOOP_ON(Prepare, NotFound, NotFound);
    }
};


/**
 * Declares all the test cases around the FinalClass that must extend the
 * BlobTestSuite interface.
 */
#define DECLARE_BLOBTESTSUITE(FinalClass) \
TEST_F(FinalClass, UploadInitAbort) { \
    test_upload_init_abort(); \
} \
TEST_F(FinalClass, UploadPrepareAbort) { \
    test_upload_prepare_abort(); \
} \
TEST_F(FinalClass, RemoveCommit) { \
    test_remove_abort(); \
} \
TEST_F(FinalClass, RemoveAbort) { \
    test_remove_commit(); \
} \
TEST_F(FinalClass, RemovePrepareNotFoundCommit) { \
    test_remove_not_found_and_abort(); \
} \
TEST_F(FinalClass, RemovePreapreNotFoundAbort) { \
    test_remove_not_found_and_commit(); \
} \
TEST_F(FinalClass, DownloadNotFound) { \
    test_download_not_found(); \
} \
TEST_F(FinalClass, Cycle) { \
    test_cycle(); \
}


#endif  // TESTS_COMMON_BLOBTESTSUITE_H_
