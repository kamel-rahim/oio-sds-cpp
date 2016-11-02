/**
 * Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/
 */

#include <signal.h>

#include <glog/logging.h>
#include <libmill.h>
#include <liberasurecode/erasurecode.h>

#include <iostream>
#include <algorithm>

#include "utils/utils.h"
#include "oio/api/blob.h"
#include "oio/ec/blob.h"

#include "./MillDaemon.h"
#include "./ec-proxy-headers.h"

using oio::ec::blob::RemovalBuilder;
using oio::ec::blob::DownloadBuilder;
using oio::ec::blob::UploadBuilder;

static volatile bool flag_running{true};

/*
 * List of possible values for the "algo" parameter of "ec" data security:
 */

static std::map<std::string, int> TheEncodingMethods = { {"jerasure_rs_vand", 		EC_BACKEND_JERASURE_RS_VAND},
		                                         	 	 {"jerasure_rs_cauchy", 	EC_BACKEND_JERASURE_RS_CAUCHY},
														 {"flat_xor_hd", 			EC_BACKEND_FLAT_XOR_HD},
														 {"isa_l_rs_vand", 			EC_BACKEND_ISA_L_RS_VAND},
														 {"shss", 					EC_BACKEND_SHSS},
														 {"liberasurecode_rs_vand", EC_BACKEND_LIBERASURECODE_RS_VAND} };

static void _sighandler_stop(int s UNUSED) {
    flag_running = 0;
}

static void transform_nondigit_to_space(std::string &str) {  // NOLINT
    for (auto &c : str) {
        if (!isdigit(c))
            c = ' ';
    }
}

struct numeric_only : std::ctype<char> {
    numeric_only() : std::ctype<char>(get_table()) {}

    static std::ctype_base::mask const *get_table() {
        static std::vector<std::ctype_base::mask>
                rc(std::ctype<char>::table_size, std::ctype_base::space);

        std::fill(&rc['0'], &rc[':'], std::ctype_base::digit);
        return &rc[0];
    }
};


class EcHandler : public BlobHandler {
 private:
    std::string chunk_id;
    std::set<oio::ec::blob::rawxSet> targets;

    std::map<std::string, std::string> xattrs;
    int kVal, mVal, nbChunks, EncodingMethod;
    int64_t offset_pos;
    int64_t chunkSize;
    std::string req_id;
    uint64_t offset, size_expected;

 public:
    EcHandler() : chunk_id(), targets(), xattrs() {}

    ~EcHandler() {}

    std::unique_ptr<oio::api::blob::Upload> GetUpload() override {
        auto builder = UploadBuilder();
        builder.BlockSize(1024 * 1024);

        builder.OffsetPos(offset_pos);
        builder.Encoding_Method (EncodingMethod);
        builder.M_Val(mVal);
        builder.K_Val(kVal);
        builder.Req_id(req_id);
        builder.NbChunks(nbChunks);

        for (const auto &to : targets)
            builder.Target(to);
        auto ul = builder.Build();
        for (const auto &e : xattrs)
            ul->SetXattr(e.first, e.second);
        return ul;
    }

    std::unique_ptr<oio::api::blob::Download> GetDownload() override {
        auto builder = DownloadBuilder();

        builder.ChunkSize(chunkSize);
        builder.M_Val(mVal);
        builder.K_Val(kVal);
        builder.NbChunks(nbChunks);
        builder.Offset(offset);
        builder.Req_id(req_id);
        builder.SizeExpected(size_expected);

        for (const auto to : targets)
            builder.Target(to);
        return builder.Build();
        return NULL;
    }

    std::unique_ptr<oio::api::blob::Removal> GetRemoval() override {
        auto builder = RemovalBuilder();

        for (const auto to : targets)
            builder.Target(to);
        // return builder.Build();
        return NULL;
    }

    SoftError SetUrl(const std::string &u UNUSED) override {
        // Get the name, this is common to al the requests
//        http_parser_url url;
//        if (0 != http_parser_parse_url(u.data(), u.size(), false, &url))
//            return {400, 400, "URL parse error"};
//        if (!_http_url_has(url, UF_PATH))
//            return {400, 400, "URL has no path"};

//        // Get the chunk-id, the last part of the URL path
//        auto path = _http_url_field(url, UF_PATH, u.data(), u.size());
//        auto sep = path.rfind('/');
//        if (sep == std::string::npos || sep == path.size() - 1)
//            return {400, 400, "URL has no/empty basename"};

//        chunk_id.assign(path, sep + 1, std::string::npos);
        return {200, 200, "OK"};
    }

    SoftError SetHeader(const std::string &k, const std::string &v) override {
        EcHeader header;
        header.Parse(k);
        if (header.Matched()) {
            switch (header.Get()) {
                case EcHeader::Value::ChunkDest: {
                    oio::ec::blob::rawxSet rawx;

                    std::string str(k);
                    transform_nondigit_to_space(str);

                    std::stringstream ss(str);
                    ss >> rawx.chunk_number;

                    std::size_t pos = v.find(":");
                    std::size_t pos2 = v.find(":",
                                              pos + 1);  // we need second one

                    str = v.substr(pos + 3, pos2 - pos + 2);
                    rawx.host = str;

                    str = v.substr(pos2 + 1, 4);
                    std::stringstream ss2(str);
                    ss2 >> rawx.chunk_port;

                    str = str = v.substr(pos2 + 6);
                    rawx.filename = str;
                    rawx.chunk_str = v;

                    targets.insert(rawx);
                }
                    break;
                 case EcHeader::Value::ChunkMethod: {
                	EncodingMethod = EC_BACKEND_NULL ;
                    for (const auto &e : TheEncodingMethods) {
                        std::size_t pos = v.find(e.first);
                        if (pos!=std::string::npos) {
                        	EncodingMethod = e.second ;
                        	break ;
                        }
                    }
                    if (EncodingMethod == EC_BACKEND_NULL)
                        LOG(ERROR) << "LIBERASURECODE: Could not detect encoding method";

                    std::string all_numbers(v);
                    transform_nondigit_to_space(all_numbers);
                    std::stringstream ss(all_numbers);
                    ss >> kVal;
                    ss >> mVal;
                }
                    break;
                case EcHeader::Value::Chunks: {
                    std::string all_numbers(v);
                    std::stringstream ss(all_numbers);
                    ss >> nbChunks;
                }
                    break;
                case EcHeader::Value::ChunkPos: {
                    std::string all_numbers(v);
                    std::stringstream ss(all_numbers);
                    ss >> offset_pos;
                }
                    break;
                case EcHeader::Value::ChunkSize: {
                    std::string all_numbers(v);
                    std::stringstream ss(all_numbers);
                    ss >> chunkSize;
                }
                    break;
                case EcHeader::Value::ReqId: {
                    std::string all_numbers(v);
                    std::stringstream ss(all_numbers);
                    ss >> chunkSize;
                }
                    break;
                case EcHeader::Value::Range: {
                    std::string all_numbers(v);
                    transform_nondigit_to_space(all_numbers);
                    std::stringstream ss(all_numbers);
                    ss >> offset;
                    ss >> size_expected;
                    size_expected = size_expected + 1 - offset;
                }
                    break;
                default: {
                    // remove leading string OIO_HEADER_EC_PREFIX
                    std::string str_cpy = k;
                    std::string str_leading(OIO_HEADER_EC_PREFIX);
                    int Length = str_cpy.length();
                    std::size_t pos = str_cpy.find(OIO_HEADER_EC_PREFIX);
                    if (pos == 0) {
                        Length = Length - str_leading.length();
                        str_cpy.erase(str_cpy.begin(), str_cpy.end() - Length);
                    }
                    xattrs[str_cpy] = v;
                }
            }
        }
        return {200, 200, "OK"};
    }
};

class EcRepository : public BlobRepository {
 public:
    EcRepository() {}

    ~EcRepository() override {}

    BlobRepository *Clone() override { return new EcRepository; }

    bool Configure(const std::string &cfg UNUSED) override { return true; }

    BlobHandler *Handler() override { return new EcHandler(); }
};

int main(int argc, char **argv) {
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    google::InitGoogleLogging(argv[0]);
    FLAGS_logtostderr = true;

    if (argc < 2) {
        LOG(ERROR) << "Usage: " << argv[0] << " FILE [FILE...]";
        return 1;
    }

    signal(SIGINT, _sighandler_stop);
    signal(SIGTERM, _sighandler_stop);
    signal(SIGUSR1, SIG_IGN);
    signal(SIGUSR2, SIG_IGN);
    signal(SIGHUP, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
    stdin = freopen("/dev/null", "r", stdin);
    stdout = freopen("/dev/null", "a", stdout);
    mill_goprepare(1024, 16384, sizeof(void *));

    std::shared_ptr<BlobRepository> repo(new EcRepository);
    BlobDaemon daemon(repo);
    for (int i = 1; i < argc; ++i) {
        if (!daemon.LoadJsonFile(argv[i]))
            return 1;
    }
    daemon.Start(&flag_running);
    daemon.Join();
    return 0;
}
