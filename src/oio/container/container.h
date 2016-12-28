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

#ifndef SRC_OIO_CONTAINER_CONTAINER_H_
#define SRC_OIO_CONTAINER_CONTAINER_H_

#include <string>
#include <memory>
#include <map>
#include <set>

#include "oio/api/blob.h"
#include "oio/blob/http/blob.h"
#include "utils/serialize_def.h"
#include "oio/container/command.h"
#include "utils/command.h"

namespace user_container {

class ContainerClient {
 private:
    OioUrl url;
    ContainerProperties payload;
    std::set<std::string> del_properties;
    std::shared_ptr<net::Socket> _socket;

    oio_err http_call_parse_body(http_param *http);
    oio_err http_call(http_param *http);

 public:
    explicit ContainerClient(OioUrl &file_id) : url(file_id) { }

    void SetSocket(std::shared_ptr<net::Socket> socket) { _socket = socket; }

    void SetData(const ContainerProperties &Param) {
        payload = Param;
    }

    void AddProperty(std::string key, std::string value) {
        payload[key] =  value;
    }

    void RemoveProperty(std::string key) {
        del_properties.insert(key);
    }

    ContainerProperties &GetData() {
        return payload;
    }

    oio_err Create();
    oio_err Show();
    oio_err List();
    oio_err Destroy();
    oio_err Touch();
    oio_err Dedup();
    oio_err GetProperties();
    oio_err SetProperties();
    oio_err DelProperties();
};

}  // namespace user_container

#endif  // SRC_OIO_CONTAINER_CONTAINER_H_
