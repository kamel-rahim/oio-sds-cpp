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

#ifndef  SRC_OIO_DIRECTORY_DIR_H_
#define  SRC_OIO_DIRECTORY_DIR_H_

#include <string>
#include <memory>

#include "utils/Http.h"
#include "oio/api/blob.h"
#include "oio/blob/http/blob.h"
#include "oio/directory/command.h"

enum body_type { META, METAS, PROPERTIES };

namespace oio {
namespace directory {

class DirectoryClient {
 private :
    OioUrl url;
    DirPayload output;
    std::shared_ptr<net::Socket> _socket;

    oio::api::OioError
    http_call_parse_body(::http::Parameters *params, body_type type);

    oio::api::OioError http_call(::http::Parameters *params);

 public:
    explicit DirectoryClient(const OioUrl &u) : url(u), output() {}

    void SetSocket(std::shared_ptr<net::Socket> socket) {
        _socket = socket;
    }

    void SetData(const DirPayload &Param) {
        output = Param;
    }

    void AddProperties(std::string key, std::string value) {
        output[key] = value;
    }

    DirPayload &GetData() {
        return output;
    }

    oio::api::OioError Create();

    oio::api::OioError Link();

    oio::api::OioError Unlink();

    oio::api::OioError Destroy();

    oio::api::OioError SetProperties();

    oio::api::OioError GetProperties();

    oio::api::OioError DelProperties();

    oio::api::OioError Renew();

    oio::api::OioError Show();
};

}  // namespace directory
}  // namespace oio

#endif  //  SRC_OIO_DIRECTORY_DIR_H_
