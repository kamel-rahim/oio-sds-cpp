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


#include <cassert>
#include <string>
#include <functional>
#include <sstream>
#include <memory>
#include <vector>
#include <iomanip>
#include <cstring>

#include "utils/macros.h"
#include "utils/net.h"
#include "utils/Http.h"
#include "oio/api/blob.h"
#include "oio/content/content.h"

using user_content::content;

#define SELECTOR(str) std::string("/v3.0/") + contentParam.NameSpace()    +\
                      std::string("/content/") + str                      +\
                      std::string("?acct=") + contentParam.Account()      +\
                      std::string("&ref=") +  contentParam.Container()    +\
                      (contentParam.Type().size() ? std::string("&type=") +\
                       contentParam.Type() :  "")                         +\
                      std::string("&path=") + contentParam.Filename()

oio_err content::http_call_parse_body(http_param *http, body_type type) {
    http::Code rc = http->HTTP_call();
    oio_err err;
    if (!rc == http::Code::OK) {
        err.get_message(-1, "HTTP_exec exec error");
    } else {
        bool ret = 0;
        switch (type) {
        case body_type::PREPARE:
        case body_type::SHOW:
            ret = contentParam.put_contents(http->body_out);
            break;
        case body_type::PROPERTIES:
            ret = contentParam.put_properties(http->body_out);
            break;
        }
        if (!ret)
            err.put_message(http->body_out);
    }
    return err;
}

oio_err content::http_call(http_param *http) {
    http::Code rc = http->HTTP_call();
    oio_err err;
    if (!rc == http::Code::OK)
        err.get_message(-1, "HTTP_exec exec error");
    else
        err.put_message(http->body_out);
    return err;
}

oio_err content::Create(int size) {
    std::string body_in;
    contentParam.get_contents(&body_in);
    http_param http(_socket, "POST", (SELECTOR("create")), body_in);

    http.header_in["x-oio-content-meta-length"] = std::to_string(size);
    http.header_in["x-oio-content-meta-hash"]   =
            "00000000000000000000000000000000";
    http.header_in["x-oio-content-type"]        = "octet/stream";
    return http_call(&http);
}

oio_err content::Copy(std::string url) {
    http_param http(_socket, "POST", (SELECTOR("copy")));
    http.header_in["destination"] = url;
    return http_call(&http);
}


oio_err content::SetProperties() {
    std::string body_in;
    contentParam.get_properties(&body_in);
    http_param http(_socket, "POST", SELECTOR("set_properties"), body_in);
    return http_call(&http);
}

oio_err content::DelProperties() {
    std::string body_in;
    contentParam.get_properties_key(&body_in, &del_properties);
    http_param http(_socket, "POST", SELECTOR("del_properties"), body_in);
    del_properties.clear();
    return http_call(&http);
}

oio_err content::Delete() {
    http_param http(_socket, "POST", (SELECTOR("delete")));
    return http_call(&http);
}

oio_err content::Show() {
    http_param http(_socket, "GET", SELECTOR("show"));
    http.header_out_filter = "x-oio-content-meta-";
    http.header_out = &contentParam.System();
    return http_call_parse_body(&http, body_type::SHOW);
}

oio_err content::GetProperties() {
    http_param http(_socket, "POST", SELECTOR("get_properties"));
    http.header_out_filter = "x-oio-content-meta-";
    http.header_out = &contentParam.System();
    return http_call_parse_body(&http, body_type::PROPERTIES);
}

oio_err content::Prepare(bool autocreate) {
    std::string body_in;
    contentParam.get_size(&body_in);
    http_param http(_socket, "POST", SELECTOR("prepare"), body_in);
    if (autocreate)
        http.header_in["x-oio-action-mode"] = "autocreate";
    http.header_out_filter = "x-oio-content-meta-";
    http.header_out = &contentParam.System();
    return http_call_parse_body(&http, body_type::PREPARE);
}
