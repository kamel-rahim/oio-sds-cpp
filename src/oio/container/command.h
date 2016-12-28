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

#ifndef SRC_OIO_CONTAINER_COMMAND_H_
#define SRC_OIO_CONTAINER_COMMAND_H_

#include <string.h>
#include <algorithm>
#include <map>
#include <string>
#include <set>

#include "utils/serialize_def.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "oio/directory/command.h"


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

#endif  // SRC_OIO_CONTAINER_COMMAND_H_
