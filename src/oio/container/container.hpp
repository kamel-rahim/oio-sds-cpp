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

#ifndef SRC_OIO_CONTAINER_CONTAINER_HPP_
#define SRC_OIO_CONTAINER_CONTAINER_HPP_

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <string>
#include <memory>
#include <map>
#include <set>

#include "oio/api/blob.hpp"
#include "oio/directory/dir.hpp"
#include "oio/blob/http/blob.hpp"

namespace oio {
namespace container {

class ContainerProperties {
 public:
    using PropSet = std::map<std::string, std::string>;

 private:
    PropSet user;
    PropSet system;

 public:
    ContainerProperties() {}

    ContainerProperties &operator=(const ContainerProperties &arg) {
        user = arg.user;
        system = arg.system;
        return *this;
    }

    std::string &operator[](std::string key) { return user[key]; }

    void erase_properties(std::string key) { user.erase(key); }

    PropSet &System() { return system; }

    bool DecodeProperties(std::string p) {
        user.clear();
        system.clear();

        rapidjson::Document document;
        if (document.Parse(p.c_str()).HasParseError()) {
            LOG(ERROR) << "Invalid JSON";
            return false;
        }
        if (!document.HasMember("properties")) {
            LOG(ERROR) << "Missing 'properties' field";
            return false;
        } else {
            const rapidjson::Value &obj = document["properties"];
            if (obj.IsObject()) {
                for (auto &v : obj.GetObject())
                    user[v.name.GetString()] = v.value.GetString();
            }
        }

        if (!document.HasMember("system")) {
            LOG(ERROR) << "Missing 'system' field";
            return false;
        } else {
            const rapidjson::Value &obj = document["system"];
            if (obj.IsObject()) {
                for (auto &v : obj.GetObject())
                    system[v.name.GetString()] = v.value.GetString();
            }
        }
        return true;
    }

    void EncodeProperties(std::string *p) {
        rapidjson::StringBuffer buf;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buf);
        writer.StartObject();

        // Pack the properties
        writer.Key("properties");
        writer.StartObject();
        for (const auto &e : user) {
            // TODO(jfs): skip the prefix?
            writer.Key(e.first.c_str());
            writer.String(e.second.c_str());
        }
        writer.EndObject();

        // Idem for the system properties
        writer.Key("system");
        writer.StartObject();
        for (const auto &e : system) {
            // TODO(jfs): skip the prefix?
            writer.Key(e.first.c_str());
            writer.String(e.second.c_str());
        }
        writer.EndObject();

        p->assign(buf.GetString());
    }
};

class Client {
 private:
    using OioUrl = ::oio::directory::OioUrl;

    OioUrl url;
    ContainerProperties payload;
    std::set<std::string> del_properties;
    std::shared_ptr<net::Socket> _socket;

    oio::api::OioError http_call_parse_body(::http::Parameters *params);

    oio::api::OioError http_call(::http::Parameters *params);

 public:
    explicit Client(OioUrl &file_id) : url(file_id) {}

    void SetSocket(std::shared_ptr<net::Socket> socket) { _socket = socket; }

    void SetData(const ContainerProperties &Param) {
        payload = Param;
    }

    void AddProperty(std::string key, std::string value) {
        payload[key] = value;
    }

    void RemoveProperty(std::string key) {
        del_properties.insert(key);
    }

    ContainerProperties &GetData() {
        return payload;
    }

    oio::api::OioError Create();

    oio::api::OioError Show();

    oio::api::OioError List();

    oio::api::OioError Destroy();

    oio::api::OioError Touch();

    oio::api::OioError Dedup();

    oio::api::OioError GetProperties();

    oio::api::OioError SetProperties();

    oio::api::OioError DelProperties();
};

}  // namespace container
}  // namespace oio

#endif  // SRC_OIO_CONTAINER_CONTAINER_HPP_
