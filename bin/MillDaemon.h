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

#include <utils/MillSocket.h>
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

    SoftError() noexcept: http{0}, soft{0}, why{nullptr} { }

    SoftError(int http, int soft, const char *why) noexcept:
            http(http), soft(soft), why(why) {
    }

    void Reset() noexcept { http = 0; soft = 0, why = nullptr; }

    void Pack(std::string &dst) noexcept;
};

class BlobRepository {
  public:
    virtual ~BlobRepository() {}

    virtual BlobRepository* Clone () = 0;

    virtual bool Configure (const std::string &cfg) = 0;

    virtual std::unique_ptr<oio::api::blob::Upload> GetUpload(
            const BlobClient &client) noexcept = 0;

    virtual std::unique_ptr<oio::api::blob::Download> GetDownload(
            const BlobClient &client) noexcept = 0;

    virtual std::unique_ptr<oio::api::blob::Removal> GetRemoval(
            const BlobClient &client) noexcept = 0;
};

struct BlobClient {

    FORBID_ALL_CTOR(BlobClient);

    ~BlobClient() noexcept;

    BlobClient(const MillSocket &c, std::shared_ptr<BlobRepository> r) noexcept;

    void Run(volatile bool &flag_running) noexcept;

    void Reset() noexcept;

    void SaveError(SoftError err) noexcept;

    void ReplyError(SoftError err) noexcept;

    void ReplySuccess() noexcept;

    void ReplyStream() noexcept;

    void ReplyEndOfStream() noexcept;

    void Reply100() noexcept;


    MillSocket client;
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
};

class BlobService {
    friend class BlobDaemon;

  public:
    FORBID_ALL_CTOR(BlobService);

    BlobService(std::shared_ptr<BlobRepository> r) noexcept;

    ~BlobService() noexcept;

    void Start(volatile bool &flag_running) noexcept;

    void Join() noexcept;

    bool Configure(const std::string &cfg) noexcept;

  private:
    NOINLINE void Run(volatile bool &flag_running) noexcept;

    NOINLINE void RunClient(volatile bool &flag_running,
            MillSocket s0) noexcept;

  private:
    MillSocket front;
    std::shared_ptr<BlobRepository> repository;
    chan done;
};

class BlobDaemon {
  public:
    FORBID_ALL_CTOR(BlobDaemon);

    BlobDaemon(std::shared_ptr<BlobRepository> rp) noexcept;

    ~BlobDaemon() noexcept;

    bool LoadJsonFile(const std::string &path) noexcept;

    bool LoadJson(const std::string &cfg) noexcept;

    void Start(volatile bool &flag_running) noexcept;

    void Join() noexcept;

  private:
    std::vector<std::shared_ptr<BlobService>> services;
    std::shared_ptr<BlobRepository> repository_prototype;
};

#endif //OIO_KINETIC_MILLDAEMON_H
