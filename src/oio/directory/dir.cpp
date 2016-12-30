/**
 * This file is part of the OpenIO client libraries
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

#include <openssl/sha.h>

#include <string>
#include <functional>
#include <sstream>
#include <memory>
#include <vector>
#include <iomanip>
#include <cstring>

#include "utils/http.hpp"
#include "oio/api/blob.hpp"
#include "oio/directory/dir.hpp"

#define SELECTOR(str) std::string("/v3.0/") + url.NS()       +\
                     std::string("/reference/") + str        +\
                     std::string("?acct=") + url.Account()   +\
                     std::string("&ref=") +  url.Container() +\
                     (!url.Type().empty() \
                            ? (std::string("&type=") + url.Type()) \
                            : "&type=meta2")

using ::http::Parameters;
using ::http::Code;
using oio::api::OioError;
using oio::directory::DirectoryClient;
using oio::directory::OioUrl;

OioError
DirectoryClient::http_call_parse_body(Parameters *params, body_type type) {
    Code rc = params->DoCall();
    if (!rc == Code::OK)
        return OioError(-1, "HTTP_exec exec error");

    OioError err;
    bool ret = 0;
    switch (type) {
        case body_type::META:
        case body_type::METAS:
            ret = output.Parse(params->body_out);
            break;
        case body_type::PROPERTIES:
            ret = output.Parse(params->body_out);
            break;
    }
    if (!ret)
        err.Decode(params->body_out);
    return err;
}

OioError DirectoryClient::http_call(Parameters *params) {
    auto rc = params->DoCall();
    if (rc != Code::OK)
        return OioError(-1, "HTTP_exec exec error");
    OioError err;
    err.Decode(params->body_out);
    return err;
}

OioError DirectoryClient::Create() {
    Parameters params(_socket, "POST", (SELECTOR("create")));
    return http_call(&params);
}

OioError DirectoryClient::Unlink() {
    Parameters params(_socket, "POST", (SELECTOR("unlink")));
    return http_call(&params);
}

OioError DirectoryClient::Destroy() {
    Parameters params(_socket, "POST", (SELECTOR("destroy")));
    return http_call(&params);
}

OioError DirectoryClient::DelProperties() {
    Parameters params(_socket, "POST", SELECTOR("del_properties"));
    return http_call(&params);
}

OioError DirectoryClient::Renew() {
    Parameters params(_socket, "POST", SELECTOR("Renew"));
    return http_call(&params);
}

OioError DirectoryClient::Show() {
    Parameters params(_socket, "GET", SELECTOR("show"));
    return http_call_parse_body(&params, body_type::METAS);
}

OioError DirectoryClient::GetProperties() {
    Parameters params(_socket, "POST", SELECTOR("EncodeProperties"));
    return http_call_parse_body(&params, body_type::PROPERTIES);
}

OioError DirectoryClient::Link() {
    Parameters params(_socket, "POST", SELECTOR("link"));
    return http_call_parse_body(&params, body_type::META);
}

OioError DirectoryClient::SetProperties() {
    std::string body_in;
    output.EncodeProperties(&body_in);
    Parameters params(_socket, "POST", SELECTOR("set_properties"), body_in);
    return http_call(&params);
}

void OioUrl::compute_container_id() {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);

    static const char zero = 0;

    SHA256_Update(&sha256, account.c_str(), account.size());
    SHA256_Update(&sha256, &zero, 1);
    SHA256_Update(&sha256, container.c_str(), container.size());

    SHA256_Final(hash, &sha256);
    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::uppercase << std::hex << std::setw(2)
           << std::setfill('0') << static_cast<int> (hash[i]);
    }

    cid.assign(ss.str());
}
