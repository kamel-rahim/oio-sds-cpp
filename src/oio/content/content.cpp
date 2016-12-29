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
#include "oio/content/content.h"

using oio::api::OioError;
using user_content::Content;

#define SELECTOR(str) std::string("/v3.0/") + url.NS()           +\
                      std::string("/content/") + str             +\
                      std::string("?acct=") + url.Account()      +\
                      std::string("&ref=") +  url.Container()    +\
                      (url.Type().size() ? std::string("&type=") + url.Type() : "")                         +\
                      std::string("&path=") + url.Filename()

OioError Content::http_call_parse_body(http_param *http, body_type type) {
    http::Code rc = http->HTTP_call();
    OioError err;
    if (!rc == http::Code::OK) {
        err.Set(-1, "HTTP_exec exec error");
    } else {
        bool ret = 0;
        switch (type) {
            case body_type::PREPARE:
            case body_type::SHOW:
                ret = param.put_contents(http->body_out);
                break;
            case body_type::PROPERTIES:
                ret = param.put_properties(http->body_out);
                break;
        }
        if (!ret)
            err.Decode(http->body_out);
    }
    return err;
}

OioError Content::http_call(http_param *http) {
    http::Code rc = http->HTTP_call();
    OioError err;
    if (!rc == http::Code::OK)
        err.Set(-1, "HTTP_exec exec error");
    else
        http->body_out.assign(err.Encode());
    return err;
}

OioError Content::Create(int size) {
    std::string body_in;
    param.get_contents(&body_in);
    http_param http(_socket, "POST", (SELECTOR("create")), body_in);

    http.header_in["x-oio-content-meta-length"] = std::to_string(size);
    http.header_in["x-oio-content-meta-hash"] =
            "00000000000000000000000000000000";
    http.header_in["x-oio-content-type"] = "octet/stream";
    return http_call(&http);
}

OioError Content::Copy(std::string u) {
    http_param http(_socket, "POST", SELECTOR("copy"));
    http.header_in["destination"] = u;
    return http_call(&http);
}


OioError Content::SetProperties() {
    std::string body_in;
    param.get_properties(&body_in);
    http_param http(_socket, "POST", SELECTOR("set_properties"), body_in);
    return http_call(&http);
}

OioError Content::DelProperties() {
    std::string body_in;
    param.get_properties_key(&body_in, &del_properties);
    http_param http(_socket, "POST", SELECTOR("del_properties"), body_in);
    del_properties.clear();
    return http_call(&http);
}

OioError Content::Delete() {
    http_param http(_socket, "POST", (SELECTOR("delete")));
    return http_call(&http);
}

OioError Content::Show() {
    http_param http(_socket, "GET", SELECTOR("show"));
    http.header_out_filter = "x-oio-content-meta-";
    http.header_out = &param.System();
    return http_call_parse_body(&http, body_type::SHOW);
}

OioError Content::GetProperties() {
    http_param http(_socket, "POST", SELECTOR("EncodeProperties"));
    http.header_out_filter = "x-oio-content-meta-";
    http.header_out = &param.System();
    return http_call_parse_body(&http, body_type::PROPERTIES);
}

OioError Content::Prepare(bool autocreate) {
    std::string body_in;
    param.get_size(&body_in);
    http_param http(_socket, "POST", SELECTOR("prepare"), body_in);
    if (autocreate)
        http.header_in["x-oio-action-mode"] = "autocreate";
    http.header_out_filter = "x-oio-content-meta-";
    http.header_out = &param.System();
    return http_call_parse_body(&http, body_type::PREPARE);
}
