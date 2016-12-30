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
#include "oio/api/blob.hpp"
#include "oio/container/container.hpp"

using Parameters = ::http::Parameters;
using Code = ::http::Code;
using OioError = ::oio::api::OioError;
using ::oio::container::Client;

#define SELECTOR(str) std::string("/v3.0/") + url.NS()        +\
                      std::string("/container/") + str        +\
                      std::string("?acct=") + url.Account()   +\
                      std::string("&ref=") +  url.Container() +\
                      (url.Type().size() \
                            ? std::string("&type=") + url.Type() : "")

OioError Client::http_call_parse_body(Parameters *params) {
    Code rc = params->DoCall();
    if (!rc == Code::OK)
        return OioError(-1, "HTTP_exec exec error");

    OioError err;
    if (!payload.DecodeProperties(params->body_out))
        err.Decode(params->body_out);
    return err;
}

OioError Client::http_call(Parameters *params) {
    Code rc = params->DoCall();
    OioError err;
    if (!rc == Code::OK)
        err.Set(-1, "HTTP_exec exec error");
    else
        err.Decode(params->body_out);
    return err;
}

OioError Client::GetProperties() {
    Parameters http(_socket, "POST", SELECTOR("EncodeProperties"));
    return http_call_parse_body(&http);
}

OioError Client::Create() {
    Parameters http(_socket, "POST", SELECTOR("create"));
    payload.EncodeProperties(&http.body_in);
    http.header_in["x-oio-action-mode"] = "autocreate";
    return http_call(&http);
}

OioError Client::SetProperties() {
    Parameters params(_socket, "POST", SELECTOR("set_properties"));
    payload.EncodeProperties(&params.body_in);
    return http_call(&params);
}

static void get_properties_key(std::string *p, std::set<std::string> *props) {
    rapidjson::StringBuffer buf;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buf);

    writer.StartArray();
    for (const auto &e : *props)
        writer.String(e.c_str());
    p->assign(buf.GetString());
}

OioError Client::DelProperties() {
    Parameters params(_socket, "POST", SELECTOR("del_properties"));
    get_properties_key(&params.body_in, &del_properties);
    del_properties.clear();
    return http_call(&params);
}

OioError Client::Show() {
    Parameters params(_socket, "GET", SELECTOR("show"));
    params.header_out_filter = "x-oio-container-meta-sys-";
    params.header_out = &payload.System();
    return http_call(&params);
}

OioError Client::List() {
    Parameters params(_socket, "GET", SELECTOR("list"));
    params.header_out_filter = "x-oio-container-meta-sys-";
    params.header_out = &payload.System();
    return http_call(&params);
}

OioError Client::Destroy() {
    Parameters params(_socket, "POST", SELECTOR("destroy"));
    return http_call(&params);
}

OioError Client::Touch() {
    Parameters params(_socket, "POST", SELECTOR("touch"));
    return http_call(&params);
}

OioError Client::Dedup() {
    Parameters params(_socket, "POST", SELECTOR("dedup"));
    return http_call(&params);
}
