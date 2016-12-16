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

#ifndef SRC_OIO_CONTENT_CONTENT_H_
#define SRC_OIO_CONTENT_CONTENT_H_

#include <string>
#include <memory>
#include <map>

#include "oio/api/blob.h"
#include "oio/http/blob.h"
#include "oio/api/serialize_def.h"
#include "oio/directory/command.h"
#include "oio/content/command.h"

namespace user_content {

enum calltype { PROPERTIES, PREPARE, SHOW };

class content {
 private :
    _content_param ContentParam;
    std::map<std::string, std::string> extra_headers;
    std::shared_ptr<net::Socket> _socket;

    oio_err HttpCall(std::string selector);
    oio_err HttpCall(std::string selector, std::string data);
    oio_err HttpCall(std::string selector, std::string data, bool autocreate);
    oio_err HttpCall(std::string method, std::string selector,
            std::string data, calltype type, bool autocreate);

 public:
    explicit content(_file_id &file_id) : ContentParam(file_id) { }
    content(std::string account, std::string container, std::string type,
            std::string filename) {
        ContentParam = _content_param(account, container, type, filename);
    }

    void SetSocket(std::shared_ptr<net::Socket> socket)  {_socket = socket;   }
    void ClearData()                     { ContentParam.ClearData();          }
    void SetData(_content_param *Param)  { ContentParam = *Param;             }
    _content_param &GetData()            { return ContentParam;               }
    void RemoveProperty(std::string key) { ContentParam.erase_properties(key);}
    void AddProperty(std::string key, std::string value)  {
        ContentParam[key] = value;
    }

    oio_err Create(int size);
    oio_err Prepare();
    oio_err Show();
    oio_err List();
    oio_err Delete();
    oio_err Copy(std::string url);

    oio_err GetProperties();
    oio_err SetProperties();
    oio_err DelProperties();
};
}  // namespace user_content
#endif  // SRC_OIO_CONTENT_CONTENT_H_
