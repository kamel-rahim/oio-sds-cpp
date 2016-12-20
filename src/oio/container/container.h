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

#include "oio/api/blob.h"
#include "oio/http/blob.h"
#include "oio/api/serialize_def.h"
#include "oio/container/command.h"
#include "utils/command.h"

namespace user_container {

class container {
 private :
    _container_param ContainerParam;
    std::shared_ptr<net::Socket> _socket;

    oio_err http_call_parse_body(http_param *http);
    oio_err http_call(http_param *http);

 public:
    explicit container(_file_id &file_id) : ContainerParam(file_id) { }
    container(std::string account, std::string container, std::string type) {
        ContainerParam = _container_param(account , container, type);
    }

    void SetSocket(std::shared_ptr<net::Socket> socket) {
        _socket = socket;
    }

    void SetData(const _container_param &Param) {
        ContainerParam = Param;
    }

    void AddProperty(std::string key, std::string value) {
        ContainerParam[key] =  value;
    }

    void RemoveProperty(std::string key) {
        ContainerParam.erase_properties(key);
    }

    _container_param &GetData() {
        return ContainerParam;
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
