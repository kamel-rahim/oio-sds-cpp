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


#include <string>
#include <functional>
#include <sstream>
#include <memory>
#include <vector>
#include <iomanip>
#include <cstring>

#include "utils/Http.h"
#include "oio/api/blob.h"
#include "oio/directory/dir.h"

#define SELECTOR(str) std::string("/v3.0/") + url.NS()       +\
                     std::string("/reference/") + str        +\
                     std::string("?acct=") + url.Account()   +\
                     std::string("&ref=") +  url.Container() +\
                     (!url.Type().empty() \
                            ? (std::string("&type=") + url.Type()) \
                            : "&type=meta2")

using oio::api::OioError;

OioError
DirectoryClient::http_call_parse_body(http_param *http, body_type type) {
    http::Code rc = http->HTTP_call();
    OioError err;
    if (!rc == http::Code::OK) {
        err.Set(-1, "HTTP_exec exec error");
    } else {
        bool ret = 0;
        switch (type) {
            case body_type::META:
                ret = output.Parse(http->body_out);
                break;
            case body_type::METAS:
                ret = output.Parse(http->body_out);
                break;
            case body_type::PROPERTIES:
                ret = output.put_properties(http->body_out);
                break;
        }
        if (!ret)
            err.Decode(http->body_out);
    }
    return err;
}

OioError DirectoryClient::http_call(http_param *http) {
    http::Code rc = http->HTTP_call();
    OioError err;
    if (!rc == http::Code::OK)
        err.Set(-1, "HTTP_exec exec error");
    else
        err.Decode(http->body_out);
    return err;
}

OioError DirectoryClient::Create() {
    http_param http(_socket, "POST", (SELECTOR("create")));
    return http_call(&http);
}

OioError DirectoryClient::Unlink() {
    http_param http(_socket, "POST", (SELECTOR("unlink")));
    return http_call(&http);
}

OioError DirectoryClient::Destroy() {
    http_param http(_socket, "POST", (SELECTOR("destroy")));
    return http_call(&http);
}

OioError DirectoryClient::DelProperties() {
    http_param http(_socket, "POST", SELECTOR("del_properties"));
    return http_call(&http);
}

OioError DirectoryClient::Renew() {
    http_param http(_socket, "POST", SELECTOR("Renew"));
    return http_call(&http);
}

OioError DirectoryClient::Show() {
    http_param http(_socket, "GET", SELECTOR("show"));
    return http_call_parse_body(&http, body_type::METAS);
}

OioError DirectoryClient::GetProperties() {
    http_param http(_socket, "POST", SELECTOR("EncodeProperties"));
    return http_call_parse_body(&http, body_type::PROPERTIES);
}

OioError DirectoryClient::Link() {
    http_param http(_socket, "POST", SELECTOR("link"));
    return http_call_parse_body(&http, body_type::META);
}

OioError DirectoryClient::SetProperties() {
    std::string body_in;
    output.get_properties(&body_in);
    http_param http(_socket, "POST", SELECTOR("set_properties"), body_in);
    return http_call(&http);
}
