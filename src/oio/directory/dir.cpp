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
#include "oio/directory/dir.h"

#define SEL(o,u,str) std::string("/v3.0/") + u.NS()    +\
                     std::string("/reference/") + str         +\
                     std::string("?acct=") + u.Account()      +\
                     std::string("&ref=") +  u.Container()    +\
                     (!u.Type().empty() ? (std::string("&type=") + u.Type()) \
                                        : "&type=meta2")

#define SELECTOR(S) SEL(output,url,S)

oio_err DirectoryClient::http_call_parse_body(http_param *http, body_type type) {
    http::Code rc = http->HTTP_call();
    oio_err err;
    if (!rc == http::Code::OK) {
        err.get_message(-1, "HTTP_exec exec error");
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
            err.put_message(http->body_out);
    }
    return err;
}

oio_err DirectoryClient::http_call(http_param *http) {
    http::Code rc = http->HTTP_call();
    oio_err err;
    if (!rc == http::Code::OK)
        err.get_message(-1, "HTTP_exec exec error");
    else
        err.put_message(http->body_out);
    return err;
}

oio_err DirectoryClient::Create() {
    http_param http(_socket, "POST", (SELECTOR("create")));
    return http_call(&http);
}

oio_err DirectoryClient::Unlink() {
    http_param http(_socket, "POST", (SELECTOR("unlink")));
    return http_call(&http);
}

oio_err DirectoryClient::Destroy() {
    http_param http(_socket, "POST", (SELECTOR("destroy")));
    return http_call(&http);
}

oio_err DirectoryClient::DelProperties() {
    http_param http(_socket, "POST", SELECTOR("del_properties"));
    return http_call(&http);
}

oio_err DirectoryClient::Renew() {
    http_param http(_socket, "POST", SELECTOR("Renew"));
    return http_call(&http);
}

oio_err DirectoryClient::Show() {
    http_param http(_socket, "GET", SELECTOR("show"));
    return http_call_parse_body(&http, body_type::METAS);
}

oio_err DirectoryClient::GetProperties() {
    http_param http(_socket, "POST", SELECTOR("EncodeProperties"));
    return http_call_parse_body(&http, body_type::PROPERTIES);
}

oio_err DirectoryClient::Link() {
    http_param http(_socket, "POST", SELECTOR("link"));
    return http_call_parse_body(&http, body_type::META);
}

oio_err DirectoryClient::SetProperties() {
    std::string body_in;
    output.get_properties(&body_in);
    http_param http(_socket, "POST", SELECTOR("set_properties"), body_in);
    return http_call(&http);
}


