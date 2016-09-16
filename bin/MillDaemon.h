/** Copyright (c) 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#ifndef OIO_KINETIC_MILLDAEMON_H
#define OIO_KINETIC_MILLDAEMON_H

#include <vector>
#include <map>
#include <limits>
#include <memory>

#include <sys/uio.h>

#include <rapidjson/document.h>
#include <libmill.h>
#include <http-parser/http_parser.h>

#include <utils/net.h>
#include <oio/api/blob.h>
#include "headers.h"

class BlobRepository;

class BlobClient;

class BlobService;

class BlobDaemon;

/* -------------------------------------------------------------------------- */

struct SoftError {
    int http, soft;
    const char *why;

    SoftError(): http{0}, soft{0}, why{nullptr} { }

    SoftError(int http, int soft, const char *why):
            http(http), soft(soft), why(why) {
    }

    void Reset() { http = 0; soft = 0, why = nullptr; }

    void Pack(std::string &dst);
};

class BlobRepository {
  public:
    virtual ~BlobRepository() {}

    virtual BlobRepository* Clone () = 0;

    virtual bool Configure (const std::string &cfg) = 0;

    virtual std::unique_ptr<oio::api::blob::Upload> GetUpload(
            const BlobClient &client) = 0;

    virtual std::unique_ptr<oio::api::blob::Download> GetDownload(
            const BlobClient &client) = 0;

    virtual std::unique_ptr<oio::api::blob::Removal> GetRemoval(
            const BlobClient &client) = 0;
};

struct BlobClient {

    FORBID_ALL_CTOR(BlobClient);

    ~BlobClient();

    BlobClient(std::unique_ptr<net::Socket> c, std::shared_ptr<BlobRepository> r);

    void Run(volatile bool &flag_running);

    void Reset();

    void SaveError(SoftError err);

    void ReplyError(SoftError err);

    void ReplySuccess();

    void ReplyStream();

    void ReplyEndOfStream();

    void Reply100();


    std::unique_ptr<net::Socket> client;
    std::shared_ptr<BlobRepository> repository;

    struct http_parser parser;
    struct http_parser_settings settings;

    // Related to the current request
    std::string chunk_id;
    std::vector<std::string> targets;

    SoftError defered_error;
    std::unique_ptr<oio::api::blob::Upload> upload;
    std::unique_ptr<oio::api::blob::Download> download;
    std::unique_ptr<oio::api::blob::Removal> removal;

    enum http_header_e last_field;
    std::string last_field_name;
    std::map<std::string, std::string> xattrs;
    bool expect_100;
    bool want_closure;
};

class BlobService {
    friend class BlobDaemon;

  public:
    FORBID_ALL_CTOR(BlobService);

    BlobService(std::shared_ptr<BlobRepository> r);

    ~BlobService();

    void Start(volatile bool &flag_running);

    void Join();

    bool Configure(const std::string &cfg);

  private:
    NOINLINE void Run(volatile bool &flag_running);

    NOINLINE void RunClient(volatile bool &flag_running,
            std::unique_ptr<net::Socket> s0);

  private:
    net::MillSocket front;
    std::shared_ptr<BlobRepository> repository;
    chan done;
};

class BlobDaemon {
  public:
    FORBID_ALL_CTOR(BlobDaemon);

    BlobDaemon(std::shared_ptr<BlobRepository> rp);

    ~BlobDaemon();

    bool LoadJsonFile(const std::string &path);

    bool LoadJson(const std::string &cfg);

    void Start(volatile bool &flag_running);

    void Join();

  private:
    std::vector<std::shared_ptr<BlobService>> services;
    std::shared_ptr<BlobRepository> repository_prototype;
};

#endif //OIO_KINETIC_MILLDAEMON_H
