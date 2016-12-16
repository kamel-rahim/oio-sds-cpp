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

#include "oio/container/container.h"

using user_container::container;


#define SELECTOR(str) std::string("/v3.0/") + ContainerParam.NameSpace()    +\
                      std::string("/container/") + str                      +\
                      std::string("?acct=") + ContainerParam.Account()      +\
                      std::string("&ref=") +  ContainerParam.Container()    +\
                      (ContainerParam.Type().size() ? std::string("&type=") +\
                              ContainerParam.Type() :  "")

static http::Code HTTP_exec(std::shared_ptr<net::Socket> socket,
                            std::string method, std::string selector,
                            bool autocreate,
                            const std::string &in, std::string *out) {
    http::Call call;
    call.Socket(socket)
            .Method(method)
            .Selector(selector)
            .Field("Connection", "keep-alive");
    if (autocreate)
        call.Field("x-oio-action-mode", "autocreate");

    LOG(INFO) << selector;

    http::Code rc = call.Run(in, out);

    LOG(INFO) << method << " rc=" << rc << " reply=" << *out;
    return rc;
}

static http::Code HTTP_exec(std::shared_ptr<net::Socket> socket,
                            std::string method, std::string selector,
                            bool autocreate,
                            const std::string &in, std::string *out,
                            std::map<std::string, std::string> *system) {
    http::Call call;
    call.Socket(socket)
            .Method(method)
            .Selector(selector)
            .Field("Connection", "keep-alive");
    if (autocreate)
        call.Field("x-oio-action-mode", "autocreate");

    LOG(INFO) << selector;

    http::Code rc = call.Run(in, out);

    if ( rc == http::Code::OK )
        rc = call.GetReplyHeaders(system, "x-oio-container-meta-sys-");

    LOG(INFO) << method << " rc=" << rc << " reply=" << *out;

    return rc;
}


oio_err container::HttpCall(std::string selector) {
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

oio_err container::HttpCall(std::string selector, std::string data,
                            bool autocreate) {
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

oio_err container::HttpCall(std::string method, std::string selector) {
    std::string in;
    std::string out;
    oio_err err;
    http::Code rc = HTTP_exec(_socket, method,  selector, false, in, &out,
                              &ContainerParam.System());
    if (!rc == http::Code::OK) {
        err.get_message(-1, "HTTP_exec exec error");
    } else {
        if (!ContainerParam.put_properties(out))
            err.put_message(out);
    }
    return err;
}

oio_err container::GetProperties() {
    return HttpCall ("POST", SELECTOR("get_properties"));
}

oio_err container::Create() {
    std::string Props;
    ContainerParam.get_properties(&Props);
    return HttpCall(SELECTOR ("create"), Props, true);
}

oio_err container::SetProperties() {
    std::string Props;
    ContainerParam.get_properties(&Props);
    return HttpCall(SELECTOR ("set_properties"), Props, true);
}

oio_err container::DelProperties() {
    std::string Props;
    ContainerParam.get_properties_key(&Props);
    return HttpCall (SELECTOR("del_properties"), Props, true);
}

oio_err container::Show()    { return HttpCall ("GET", SELECTOR("show"));  }
oio_err container::List()    { return HttpCall ("GET", SELECTOR("list"));  }
oio_err container::Destroy() { return HttpCall (SELECTOR("destroy"));      }
oio_err container::Touch()   { return HttpCall (SELECTOR("touch"));        }
oio_err container::Dedup()   { return HttpCall (SELECTOR("dedup"));        }

