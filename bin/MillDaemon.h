/** Copyright (c) 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#ifndef BIN_MILLDAEMON_H_
#define BIN_MILLDAEMON_H_

#include <sys/uio.h>

#include <http-parser/http_parser.h>
#include <rapidjson/document.h>
#include <libmill.h>

#include <vector>
#include <map>
#include <limits>
#include <memory>
#include <string>

#include "utils/macros.h"
#include "utils/net.h"
#include "oio/api/blob.h"
#include "./common-header-parser.h"
#include "./common-server-headers.h"

class BlobRepository;

class BlobClient;

class BlobService;

class BlobDaemon;

DECLARE_bool(verbose_daemon);

/* -------------------------------------------------------------------------- */

class BlobHandler {
 public:
    virtual ~BlobHandler() {}

    virtual SoftError SetUrl(const std::string &u) = 0;

    virtual SoftError SetHeader(const std::string &k, const std::string &v) = 0;

    virtual std::unique_ptr<oio::api::blob::Upload> GetUpload() = 0;

    virtual std::unique_ptr<oio::api::blob::Download> GetDownload() = 0;

    virtual std::unique_ptr<oio::api::blob::Removal> GetRemoval() = 0;
};

class BlobRepository {
 public:
    virtual ~BlobRepository() {}

    virtual BlobRepository *Clone() = 0;

    virtual bool Configure(const std::string &cfg) = 0;

    virtual BlobHandler *Handler() = 0;
};

struct BlobClient {
    FORBID_ALL_CTOR(BlobClient);

    ~BlobClient();

    BlobClient(std::unique_ptr<net::Socket> c,
               std::shared_ptr<BlobRepository> r);

    void Run(volatile bool *flag_running);

    void Reset();

    void SaveError(SoftError err);

    void ReplyError(SoftError err);

    void ReplySuccess();

    void ReplySuccess(int code, const std::string &payload);

    void ReplyStream();

    void ReplyEndOfStream();

    void Reply100();

    /**
     * Writes the status line and the headers.
     */
    void ReplyPreamble(int code, const char *msg, int64_t length);

    std::unique_ptr<net::Socket> client;
    std::unique_ptr<BlobHandler> handler;

    struct http_parser parser;
    struct http_parser_settings settings;

    // Related to the current request
    std::map<std::string, std::string> reply_headers;

    SoftError defered_error;
    std::unique_ptr<oio::api::blob::Upload> upload;
    std::unique_ptr<oio::api::blob::Download> download;
    std::unique_ptr<oio::api::blob::Removal> removal;

    // used during the parsing of the request
    std::string last_field_name;
    bool expect_100;
    bool want_closure;

    int64_t bytes_in;
    std::shared_ptr<Checksum> checksum_in;
};

class BlobService {
    friend class BlobDaemon;

 public:
    FORBID_ALL_CTOR(BlobService);

    explicit BlobService(std::shared_ptr<BlobRepository> r);

    ~BlobService();

    void Start(volatile bool *running);

    void Join();

    bool Configure(const std::string &cfg);

 private:
    NOINLINE void Run(volatile bool *flag_running);

    void StartClient(volatile bool *flag_running,
            std::unique_ptr<net::Socket> s0);

    NOINLINE void RunClient(volatile bool *flag_running,
            std::unique_ptr<net::Socket> s0);

 private:
    net::MillSocket front;
    std::shared_ptr<BlobRepository> repository;
    chan done;
};

class BlobDaemon {
 public:
    FORBID_ALL_CTOR(BlobDaemon);

    explicit BlobDaemon(std::shared_ptr<BlobRepository> rp);

    ~BlobDaemon();

    bool LoadJsonFile(const std::string &path);

    bool LoadJson(const std::string &cfg);

    void Start(volatile bool *flag_running);

    void Join();

 private:
    std::vector<std::shared_ptr<BlobService>> services;
    std::shared_ptr<BlobRepository> repository_prototype;
};

#endif  // BIN_MILLDAEMON_H_
