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

#ifndef SRC_UTILS_COMMAND_H_
#define SRC_UTILS_COMMAND_H_

#include <rapidjson/writer.h>

#include <map>
#include <string>

#include "serialize_def.h"
#include "utils/Http.h"

class http_param {
 public :
    std::shared_ptr<net::Socket> socket;
    std::string method;
    std::string selector;
    std::string body_in;
    std::string body_out;
    std::map<std::string, std::string> header_in;
    std::map<std::string, std::string> *header_out;
    std::string header_out_filter;

    http_param(std::shared_ptr<net::Socket> _socket,
               std::string _method, std::string _selector) :
               socket(_socket), method(_method), selector(_selector),
               header_out(NULL) {
    }

    http_param(std::shared_ptr<net::Socket> _socket,
               std::string _method, std::string _selector,
               std::string _body_in) : socket(_socket), method(_method),
               selector(_selector), body_in(_body_in),
               header_out(NULL) {
}

    http::Code HTTP_call() {
        http::Call call;
        call.Socket(socket)
                .Method(method)
                .Selector(selector)
                .Field("Connection", "keep-alive");
        if (header_in.size()) {
            for (auto& a : header_in) {
                call.Field(a.first, a.second);
            }
        }

        LOG(INFO) << method << " " << selector;
        http::Code rc = call.Run(body_in, &body_out);
        LOG(INFO) << " rc=" << rc << " reply=" << body_out;
        if (rc == http::Code::OK && header_out)
            rc = call.GetReplyHeaders(header_out, header_out_filter);
        return rc;
    }
};

class oio_err {
 public:
    int status;
    std::string message;

 public:
    oio_err() { status = 0 ; }

    void put_message(std::string s) {
        rapidjson::StringBuffer buf;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buf);
        writer.StartObject();
        writer.Key("status");
        writer.Int(status);
        writer.Key("message");
        writer.String(message.c_str());
        writer.EndObject();
        s.assign(buf.GetString());
    }

    void get_message(int st, std::string msg) {
        status = st;
        message = msg;
    }
};

#endif  //  SRC_UTILS_COMMAND_H_
