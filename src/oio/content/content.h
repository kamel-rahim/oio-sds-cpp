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
#include <set>

#include "oio/api/blob.h"
#include "utils/serialize_def.h"
#include "oio/blob/http/blob.h"
#include "oio/directory/command.h"
#include "oio/content/command.h"

namespace oio {
namespace content {

enum body_type { PROPERTIES, PREPARE, SHOW };

class Content {
 private :
    OioUrl url;
    ContentInfo param;
    std::set<std::string> del_properties;
    std::shared_ptr<net::Socket> _socket;

 private:
    oio::api::OioError
    http_call_parse_body(::http::Parameters *params, body_type type);

    oio::api::OioError http_call(::http::Parameters *http);

 public:
    explicit Content(const OioUrl &u) : url(u) {}

    void SetSocket(std::shared_ptr<net::Socket> socket) { _socket = socket; }

    void ClearData() { param.ClearData(); }

    void SetData(ContentInfo *Param) { param = *Param; }

    ContentInfo &GetData() { return param; }

    OioUrl &GetUrl() { return url; }

    void RemoveProperty(std::string key) { del_properties.insert(key); }

    void AddProperty(std::string key, std::string value) {
        param[key] = value;
    }

    oio::api::OioError Create(int size);

    oio::api::OioError Prepare(bool autocreate);

    oio::api::OioError Show();

    oio::api::OioError List();

    oio::api::OioError Delete();

    oio::api::OioError Copy(std::string url);

    oio::api::OioError GetProperties();

    oio::api::OioError SetProperties();

    oio::api::OioError DelProperties();
};

}  // namespace content
}  // namespace oio

#endif  // SRC_OIO_CONTENT_CONTENT_H_
