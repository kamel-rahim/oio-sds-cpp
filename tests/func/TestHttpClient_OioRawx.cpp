/**
 * Copyright (c) 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/
 */

/**
 * Tests the internal HTTP client works as expected when accessing an OpenIO
 * Rawx service.
 */

#include <gtest/gtest.h>
#include <gflags/gflags.h>
#include <memory>

#include "utils/macros.h"
#include "utils/net.h"
#include "utils/Http.h"
#include "bin/rawx-server-headers.h"
#include "tests/common/BlobTestSuite.h"
#include "oio/rawx/blob.h"

// The default address used for the target RAWX is an address typically
// generated by the oio-reset.sh tool applied on the "SINGLE" preset.
DEFINE_string(
        URL_RAWX,
        "127.0.0.1:6008",
        "URL of the test rawx");

using oio::rawx::blob::UploadBuilder;
using oio::rawx::blob::DownloadBuilder;
using oio::rawx::blob::RemovalBuilder;

class RawxBlobOpsFactory : public BlobOpsFactory {
    friend class RawxBlobTestSuite;

 protected:
    std::string url;
    std::string srvid, chunkid, cid, content_path, content_id;
    std::string mime_type, storage_policy, chunk_method;
    int64_t content_version;

    std::shared_ptr<net::Socket> socket;

 public:
    ~RawxBlobOpsFactory() {}
    explicit RawxBlobOpsFactory(std::string u): url(u) {}

    void ResetConnection() {
        socket.reset(new net::MillSocket);
        ASSERT_TRUE(socket->connect(url));
    }
    std::unique_ptr<oio::api::blob::Upload> Upload() override {
        UploadBuilder op;
        op.ChunkId(chunkid);
        op.ChunkPosition(0, 0);
        op.RawxId(srvid);
        op.ContainerId(cid);
        op.ContentPath(content_path);
        op.ContentId(content_id);
        op.ContentVersion(content_version);
        op.StoragePolicy(storage_policy);
        op.MimeType(mime_type);
        op.ChunkMethod(chunk_method);
        return op.Build(socket);
    }

    std::unique_ptr<oio::api::blob::Download> Download() override {
        DownloadBuilder op;
        op.ChunkId(chunkid);
        op.RawxId(srvid);
        return op.Build(socket);
    }

    std::unique_ptr<oio::api::blob::Removal> Removal() override {
        RemovalBuilder op;
        op.ChunkId(chunkid);
        op.RawxId(srvid);
        return op.Build(socket);
    }
};

class RawxBlobTestSuite : public BlobTestSuite {
 protected:
    std::string url_;

 protected:
    ~RawxBlobTestSuite() override {}

    RawxBlobTestSuite(): ::BlobTestSuite() {
        url_.assign(FLAGS_URL_RAWX);
    }

    void SetUp() override {
        auto f = new RawxBlobOpsFactory(url_);
        f->ResetConnection();
        f->srvid = generate_string_random(8, random_chars);
        f->chunkid = generate_string_random(64, random_hex);
        f->cid = generate_string_random(64, random_hex);
        f->content_path = generate_string_random(8, random_chars);
        f->content_version = ::time(0);
        f->content_id = generate_string_random(8, random_hex);
        f->storage_policy = "SINGLE";
        f->chunk_method = "plain";
        f->mime_type = "application/octet-stream";
        this->factory_ = f;
    }

    void TearDown() override {
        delete this->factory_;
        this->factory_ = nullptr;
    }
};

DECLARE_BLOBTESTSUITE(RawxBlobTestSuite);

#if 0
DEFINE_string(
        CHUNKID,
        "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
        "ChunkID to be used (X replaced by random hexadecimal char");
DEFINE_string(
        CONTENTID, "XXXXXXXX",
        "ContentID to be used (X replaced by random hexadecimal char");
DEFINE_string(
        CONTENTPATH,
        "path-XXXXXXXX",
        "Content path to be used (X replaced by random hexadecimal char");
DEFINE_string(
        CONTAINERID,
        "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
        "ContainerID to be used (X replaced by random hexadecimal char");

static void put(std::shared_ptr<net::Socket> socket) {
    std::string out;
    http::Call call;
    call.Socket(socket)
            .Method("PUT")
            .Field("Host", FLAGS_URL_RAWX)
            .Field(OIO_RAWX_HEADER_PREFIX "container-id", FLAGS_CONTAINERID)
            .Field(OIO_RAWX_HEADER_PREFIX "content-path", FLAGS_CONTENTPATH)
            .Field(OIO_RAWX_HEADER_PREFIX "content-id", FLAGS_CONTENTID)
            .Field(OIO_RAWX_HEADER_PREFIX "content-size", "0")
            .Field(OIO_RAWX_HEADER_PREFIX "content-chunksnb", "1")
            .Field(OIO_RAWX_HEADER_PREFIX "content-version", "0")
            .Field(OIO_RAWX_HEADER_PREFIX "content-storage-policy", "SINGLE")
            .Field(OIO_RAWX_HEADER_PREFIX "content-chunk-method", "plain")
            .Field(OIO_RAWX_HEADER_PREFIX "content-mime-type",
                   "application/octet-stream")
            .Field(OIO_RAWX_HEADER_PREFIX "chunk-id", FLAGS_CHUNKID)
            .Field(OIO_RAWX_HEADER_PREFIX "chunk-size", "0")
            .Field(OIO_RAWX_HEADER_PREFIX "chunk-pos", "0")
            .Selector("/rawx/" + FLAGS_CHUNKID);
    auto rc = call.Run("", &out);
    LOG(INFO) << "PUT rc=" << rc << " reply=" << out;
}

static void get(std::shared_ptr<net::Socket> socket) {
    std::string out;
    http::Call call;
    call.Socket(socket)
            .Method("GET")
            .Field("Host", FLAGS_URL_RAWX)
            .Selector("/rawx/" + FLAGS_CHUNKID);
    auto rc = call.Run("", &out);
    LOG(INFO) << "GET rc=" << rc << " reply=" << out;
}

static void del(std::shared_ptr<net::Socket> socket) {
    std::string out;
    http::Call call;
    call.Socket(socket)
            .Method("DELETE")
            .Field("Host", FLAGS_URL_RAWX)
            .Selector("/rawx/" + FLAGS_CHUNKID);
    auto rc = call.Run("", &out);
    LOG(INFO) << "GET rc=" << rc << " reply=" << out;
}

static void cycle(net::Socket *sptr, const char *url) {
    std::shared_ptr<net::Socket> socket(sptr);
    for (int i = 0; i < 3; ++i) {
        socket->connect(url);
        socket->setnodelay();
        socket->setquickack();
        put(socket);
        socket->close();

        socket->connect(url);
        socket->setnodelay();
        socket->setquickack();
        get(socket);
        socket->close();

        socket->connect(url);
        socket->setnodelay();
        socket->setquickack();
        del(socket);
        socket->close();
    }
}

TEST(Http, MillSocket_Cycle) {
    cycle(new net::MillSocket, FLAGS_URL_RAWX.c_str());
}

TEST(Http, RegularSocket_Cycle) {
    cycle(new net::RegularSocket, FLAGS_URL_RAWX.c_str());
}

static void _randomize(std::string *s) {
    int next = 0;
    auto hexa = generate_string_random(s->size(), "0123456789ABCDEF");
    for (auto &c : *s) {
        if (c == 'X')
            c = hexa[next++];
    }
}
#endif

int main(int argc, char **argv) {
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    google::InitGoogleLogging(argv[0]);
    ::testing::InitGoogleTest(&argc, argv);
    FLAGS_logtostderr = true;

    // TODO(jfs) Run a rawx in a coroutine

#if 0
    _randomize(&FLAGS_CHUNKID);
    _randomize(&FLAGS_CONTAINERID);
    _randomize(&FLAGS_CONTENTID);
    _randomize(&FLAGS_CONTENTPATH);
#endif
    return RUN_ALL_TESTS();
}
