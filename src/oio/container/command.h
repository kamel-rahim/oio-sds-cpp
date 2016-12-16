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

#include "oio/api/serialize_def.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "oio/directory/command.h"


class _container_param : public _file_id {
 private:
    std::map<std::string, std::string> properties;
    std::map<std::string, std::string> system;

 public:
    _container_param() { }
    explicit _container_param(_file_id &file_id) : _file_id(file_id) { }
    _container_param(std::string _account, std::string _container,
                     std::string _type = "")
                             : _file_id(_account, _container, _type) { }

    _container_param& operator=(const _container_param& arg) {
       _file_id::operator =(arg);
       properties   = arg.properties;
       system       = arg.system;
       return *this;
    }

    std::string& operator[](std::string key)      { return properties[key]; }
    void erase_properties(std::string key)        { properties.erase(key);  }
    std::map<std::string, std::string> &System () { return system;          }

    bool put_properties(std::string p) {
        properties.clear();
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
            const rapidjson::Value& obj = document["properties"];
            if (obj.IsObject()) {
                for (auto& v : obj.GetObject())
                    properties[v.name.GetString()] = v.value.GetString();
            }
        }

        if (!document.HasMember("system")) {
            LOG(ERROR) << "Missing 'system' field";
            return false;
        } else {
            const rapidjson::Value& obj = document["system"];
            if (obj.IsObject()) {
                for (auto& v : obj.GetObject())
                    system[v.name.GetString()] = v.value.GetString();
            }
        }
        return true;
    }

    bool get_properties(std::string *p) {
        // Pack the properties
        rapidjson::StringBuffer buf;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buf);
        writer.StartObject();
        for (const auto &e : properties) {
            writer.Key(e.first.c_str());
            writer.String(e.second.c_str());
        }
        writer.EndObject();

        *p = "{\"properties\":" + std::string(buf.GetString(),
              buf.GetSize()) + "}";
        return true;
    }

    bool get_properties_key(std::string *p) {
        *p = "[";

        bool bfirst = false;
        for (const auto &e : properties) {
            if (bfirst)
                *p += ",";
            *p += "\"" + e.first + "\"";
            bfirst = true;
        }

        *p += "]";
        return true;
    }
};

#endif  // SRC_OIO_CONTAINER_COMMAND_H_
