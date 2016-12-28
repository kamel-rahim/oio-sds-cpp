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
#include <map>

#include "utils/serialize_def.h"
#include "utils/command.h"
#include "oio/api/blob.h"
#include "oio/blob/http/blob.h"
#include "oio/directory/command.h"

enum body_type { META, METAS, PROPERTIES };

class DirectoryClient {
 private :
    OioUrl url;
    DirPayload output;
    std::shared_ptr<net::Socket> _socket;

    oio_err http_call_parse_body(http_param *http, body_type type);
    oio_err http_call(http_param *http);

 public:
    explicit DirectoryClient(const OioUrl &u) : url(u), output() {}

    void SetSocket(std::shared_ptr<net::Socket> socket ) {
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

    oio_err Create();

    oio_err Link();
    oio_err Unlink();
    oio_err Destroy();
    oio_err SetProperties();
    oio_err GetProperties();
    oio_err DelProperties();
    oio_err Renew();
    oio_err Show();
};

#endif  //  SRC_OIO_DIRECTORY_DIR_H_
