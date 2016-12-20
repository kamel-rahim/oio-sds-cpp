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

#define SELECTOR(str) std::string("/v3.0/") + DirParam.NameSpace()    +\
                      std::string("/reference/") + str                +\
                      std::string("?acct=") + DirParam.Account()      +\
                      std::string("&ref=") +  DirParam.Container()    +\
                      (DirParam.Type().size() ? std::string("&type=") +\
                              DirParam.Type() : "&type=meta2")

oio_err directory::http_call_parse_body(http_param *http, body_type type) {
    http::Code rc = http->HTTP_call();
    oio_err err;
    if (!rc == http::Code::OK) {
        err.get_message(-1, "HTTP_exec exec error");
    } else {
        bool ret = 0;
        switch (type) {
        case body_type::META:
            ret = DirParam.put_meta(http->body_out);
            break;
        case body_type::METAS:
            ret = DirParam.put_metas(http->body_out);
            break;
        case body_type::PROPERTIES:
            ret = DirParam.put_properties(http->body_out);
                break;
        }
        if (!ret)
            err.put_message(http->body_out);
    }
    return err;
}

oio_err directory::http_call(http_param *http) {
    http::Code rc = http->HTTP_call();
    oio_err err;
    if (!rc == http::Code::OK)
        err.get_message(-1, "HTTP_exec exec error");
    else
        err.put_message(http->body_out);
    return err;
}

oio_err directory::Create() {
    http_param http(_socket, "POST", (SELECTOR("create")));
    return http_call(&http);
}

oio_err directory::Unlink() {
    http_param http(_socket, "POST", (SELECTOR("unlink")));
    return http_call(&http);
}

oio_err directory::Destroy() {
    http_param http(_socket, "POST", (SELECTOR("destroy")));
    return http_call(&http);
}

oio_err directory::DelProperties() {
    http_param http(_socket, "POST", SELECTOR("del_properties"));
    return http_call(&http);
}

oio_err directory::Renew() {
    http_param http(_socket, "POST", SELECTOR("Renew"));
    return http_call(&http);
}

oio_err directory::Show() {
    http_param http(_socket, "GET", SELECTOR("show"));
    return http_call_parse_body(&http, body_type::METAS);
}

oio_err directory::GetProperties() {
    http_param http(_socket, "POST", SELECTOR("get_properties"));
    return http_call_parse_body(&http, body_type::PROPERTIES);
}

oio_err directory::Link() {
    http_param http(_socket, "POST", SELECTOR("link"));
    return http_call_parse_body(&http, body_type::META);
}

oio_err directory::SetProperties() {
    std::string body_in;
    DirParam.get_properties(&body_in);
    http_param http(_socket, "POST", SELECTOR("set_properties"), body_in);
    return http_call(&http);
}


