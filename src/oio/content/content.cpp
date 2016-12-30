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

#include "utils/http.hpp"
#include "oio/content/content.hpp"

using OioError = oio::api::OioError;
using oio::content::Content;
using ::http::Parameters;
using ::http::Code;

#define SELECTOR(str) std::string("/v3.0/") + url.NS()           +\
                      std::string("/content/") + str             +\
                      std::string("?acct=") + url.Account()      +\
                      std::string("&ref=") +  url.Container()    +\
                      (url.Type().size() ? std::string("&type=") + url.Type() \
                                         : "")                         +\
                      std::string("&path=") + url.Filename()

OioError Content::http_call_parse_body(Parameters *params, body_type type) {
    Code rc = params->DoCall();
    OioError err;
    if (!rc == Code::OK) {
        err.Set(-1, "HTTP_exec exec error");
    } else {
        bool ret = 0;
        switch (type) {
            case body_type::PREPARE:
            case body_type::SHOW:
                ret = param.put_contents(params->body_out);
                break;
            case body_type::PROPERTIES:
                ret = param.put_properties(params->body_out);
                break;
        }
        if (!ret)
            err.Decode(params->body_out);
    }
    return err;
}

OioError Content::http_call(Parameters *params) {
    Code rc = params->DoCall();
    OioError err;
    if (!rc == Code::OK)
        err.Set(-1, "HTTP_exec exec error");
    else
        params->body_out.assign(err.Encode());
    return err;
}

OioError Content::Create(int size) {
    std::string body_in;
    param.get_contents(&body_in);
    Parameters params(_socket, "POST", (SELECTOR("create")), body_in);

    params.header_in["x-oio-content-meta-length"] = std::to_string(size);
    params.header_in["x-oio-content-meta-hash"] =
            "00000000000000000000000000000000";
    params.header_in["x-oio-content-type"] = "octet/stream";
    return http_call(&params);
}

OioError Content::Copy(std::string u) {
    Parameters params(_socket, "POST", SELECTOR("copy"));
    params.header_in["destination"] = u;
    return http_call(&params);
}


OioError Content::SetProperties() {
    std::string body_in;
    param.get_properties(&body_in);
    Parameters params(_socket, "POST", SELECTOR("set_properties"), body_in);
    return http_call(&params);
}

OioError Content::DelProperties() {
    std::string body_in;
    param.get_properties_key(&body_in, &del_properties);
    Parameters params(_socket, "POST", SELECTOR("del_properties"), body_in);
    del_properties.clear();
    return http_call(&params);
}

OioError Content::Delete() {
    Parameters params(_socket, "POST", (SELECTOR("delete")));
    return http_call(&params);
}

OioError Content::Show() {
    Parameters params(_socket, "GET", SELECTOR("show"));
    params.header_out_filter = "x-oio-content-meta-";
    params.header_out = &param.System();
    return http_call_parse_body(&params, body_type::SHOW);
}

OioError Content::GetProperties() {
    Parameters params(_socket, "POST", SELECTOR("EncodeProperties"));
    params.header_out_filter = "x-oio-content-meta-";
    params.header_out = &param.System();
    return http_call_parse_body(&params, body_type::PROPERTIES);
}

OioError Content::Prepare(bool autocreate) {
    std::string body_in;
    param.get_size(&body_in);
    Parameters params(_socket, "POST", SELECTOR("prepare"), body_in);
    if (autocreate)
        params.header_in["x-oio-action-mode"] = "autocreate";
    params.header_out_filter = "x-oio-content-meta-";
    params.header_out = &param.System();
    return http_call_parse_body(&params, body_type::PREPARE);
}
