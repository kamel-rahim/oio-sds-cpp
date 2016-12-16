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


#define SELECTOR(str) std::string("/v3.0/") + ContentParam.NameSpace()    +\
                      std::string("/content/") + str                      +\
                      std::string("?acct=") + ContentParam.Account()      +\
                      std::string("&ref=") +  ContentParam.Container()    +\
                      (ContentParam.Type().size() ? std::string("&type=") +\
                       ContentParam.Type() :  "")                         +\
                      std::string("&path=") + ContentParam.Filename()


static http::Code HTTP_exec(std::shared_ptr<net::Socket> socket,
                            std::string method, std::string selector,
                            bool autocreate, const std::string &in,
                            std::string *out) {
    http::Call call;
    call.Socket(socket)
            .Method(method)
            .Selector(selector)
            .Field("Connection", "keep-alive");
    if (autocreate)
        call.Field("x-oio-action-mode", "autocreate");

    LOG(INFO) << method << " " << selector;
    http::Code rc = call.Run(in, out);
    LOG(INFO) << " rc=" << rc << " reply=" << *out;
    return rc;
}

static http::Code HTTP_exec(std::shared_ptr<net::Socket> socket,
                            std::string method, std::string selector,
                            bool autocreate, const std::string &in,
                            std::string *out,
                            std::map<std::string, std::string> *system) {
    http::Call call;
    call.Socket(socket)
            .Method(method)
            .Selector(selector)
            .Field("Connection", "keep-alive");
    if (autocreate)
        call.Field("x-oio-action-mode", "autocreate");

    LOG(INFO) << method << " " << selector;
    http::Code rc = call.Run(in, out);
    LOG(INFO) << " rc=" << rc << " reply=" << *out;
    if ( rc == http::Code::OK )
        rc = call.GetReplyHeaders(system, "x-oio-content-meta-");
    return rc;
}

static http::Code HTTP_exec(std::shared_ptr<net::Socket> socket,
                            std::string method, std::string selector,
                            const std::string &in, std::string *out,
                            const std::map<std::string,
                            std::string> &otherFields) {
    http::Call call;
    call.Socket(socket)
            .Method(method)
            .Selector(selector)
            .Field("Connection", "keep-alive");
    if (otherFields.size()) {
        for (auto& a : otherFields) {
            call.Field(a.first, a.second);
        }
    }

    LOG(INFO) << method << " " << selector;
    http::Code rc = call.Run(in, out);
    LOG(INFO) << " rc=" << rc << " reply=" << *out;
    return rc;
}

oio_err content::HttpCall(std::string selector) {
    std::string in;
    std::string out;
    oio_err err;
    http::Code rc = HTTP_exec(_socket, "POST", selector, false, in, &out);
    if (!rc == http::Code::OK)
        err.get_message(-1, "HTTP_exec exec error");
    else
        err.put_message(out);
    return err;
}

oio_err content::HttpCall(std::string selector, std::string data) {
    std::string out;
    oio_err err;
    http::Code rc = HTTP_exec(_socket, "POST", selector, data,
                              &out, extra_headers);
    if (!rc == http::Code::OK)
        err.get_message(-1, "HTTP_exec exec error");
    else
        err.put_message(out);
    return err;
}

oio_err content::HttpCall(std::string selector, std::string data,
                          bool autocreate ) {
    std::string out;
    oio_err err;
    http::Code rc = HTTP_exec(_socket, "POST", selector, autocreate,
                              data, &out);
    if (!rc == http::Code::OK)
        err.get_message(-1, "HTTP_exec exec error");
    else
        err.put_message(out);
    return err;
}

oio_err content::HttpCall(std::string method, std::string selector,
                          std::string data, calltype type, bool autocreate) {
    std::string out;
    oio_err err;
    http::Code rc = HTTP_exec(_socket, method, selector, autocreate, data,
                               &out, &ContentParam.System());
    if (!rc == http::Code::OK) {
        err.get_message(-1, "HTTP_exec exec error");
    } else {
        bool ret;
        switch (type) {
        case calltype::PREPARE:
        case calltype::SHOW:
            ret = ContentParam.put_contents(out);
            break;
        case calltype::PROPERTIES:
            ret = ContentParam.put_properties(out);
            break;
        }
        if (!ret)
            err.put_message(out);
    }
    return err;
}


oio_err content::Create(int size) {
    std::string Props;
    ContentParam.get_contents(&Props);

    extra_headers.clear();

    ContentParam.SetSize(size);

    extra_headers["x-oio-content-meta-length"] =
                  std::to_string(ContentParam.GetSize());
    extra_headers["x-oio-content-meta-hash"]   =
                  "00000000000000000000000000000000";
    extra_headers["x-oio-content-type"]        = "octet/stream";

    return HttpCall(SELECTOR ("create"), Props);
}

oio_err content::Copy(std::string url) {
    std::string in;
    extra_headers.clear();
    extra_headers["destination"] = url;

    return HttpCall(SELECTOR ("copy"), in);
}

oio_err content::Prepare() {
    std::string Props;
    ContentParam.get_size(&Props);
    return HttpCall("POST", SELECTOR("prepare"), Props,
                     calltype::PREPARE, true);
}

oio_err content::SetProperties() {
    std::string Props;
    ContentParam.get_properties(&Props);
    return HttpCall(SELECTOR("set_properties"), Props, false);
}

oio_err content::DelProperties() {
    std::string Props;
    ContentParam.get_properties_key(&Props);
    return HttpCall(SELECTOR("del_properties"), Props, false);
}

oio_err content::Show() {
    std::string In;
    return HttpCall("GET", SELECTOR("show"), In,
                    calltype::SHOW, false);
}

oio_err content::GetProperties() {
    std::string In;
    return HttpCall("POST", SELECTOR("get_properties"), In,
                    calltype::PROPERTIES, false);
}

oio_err content::Delete() { return HttpCall( SELECTOR("delete")); }


