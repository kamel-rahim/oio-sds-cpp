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

#ifndef SRC_OIO_CONTENT_COMMAND_H_
#define SRC_OIO_CONTENT_COMMAND_H_

#include <string.h>
#include <algorithm>
#include <set>
#include <map>
#include <string>

#include "utils/serialize_def.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "oio/directory/command.h"
#include "oio/blob/rawx/command.h"

struct ContentSet : public RawxUrl {
    int chunk_number;
    int pos;
    int score;
    int64_t size;
    std::string hash;

    ContentSet() : chunk_number(0), pos(0), score(0), size(0), hash("") {}

    bool operator<(const ContentSet &a) const {
        return chunk_number < a.chunk_number;
    }

    bool operator==(const ContentSet &a) const {
        return chunk_number == a.chunk_number;
    }

    void SetContent(const ContentSet &arg) {
        RawxUrl::SetUrl(arg);
        chunk_number = arg.chunk_number;
        pos = arg.pos;
        size = arg.size;
        score = arg.score;
        hash = arg.hash;
    }

    Range GetRange() { return Range(0, size); }
};


class ContentParam : public FileId {
 private:
    std::map<std::string, std::string> properties;
    std::map<std::string, std::string> system;
    std::set<ContentSet> targets;
    int default_ask_size;

 public:
    ContentParam() : default_ask_size(1) {}

    explicit ContentParam(FileId &file_id) : FileId(file_id),
                                                 default_ask_size(1) {}

    ContentParam(std::string _name_space, std::string _account,
            std::string _container, std::string _type = "",
            std::string _filename = "")
            : FileId(_name_space, _account, _container, _type,
                       _filename), default_ask_size(1) {}

    ContentParam &operator=(const ContentParam &arg) {
        FileId::operator=(arg);
        properties = arg.properties;
        return *this;
    }

    std::string &operator[](std::string key) { return properties[key]; }

    void erase_properties(std::string key) { properties.erase(key); }

    void UpdateTarget(ContentSet *set) {
        targets.erase(*set);
        targets.insert(*set);
    }

    int GetTargetsSize() { return targets.size(); }

    ContentSet GetTarget(int index) {
        for (auto &to : targets) {
            if (to.chunk_number == index)
                return to;
        }
        return ContentSet();
    }

    std::map<std::string, std::string> &System() { return system; }

    void ClearData() {
        targets.clear();
        system.clear();
        properties.clear();
    }

    bool get_size(std::string *p) {
        // Pack the size
        rapidjson::StringBuffer buf;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buf);
        writer.StartObject();
        writer.Key("size");
        writer.Int(default_ask_size);
        writer.EndObject();
        *p = std::string(buf.GetString(), buf.GetSize());
        return true;
    }

    bool put_contents(std::string p) {
        rapidjson::Document document;
        if (document.Parse(p.c_str()).HasParseError()) {
            LOG(ERROR) << "Invalid JSON";
            return false;
        }

        if (document.IsArray()) {
            for (auto &a : document.GetArray()) {
                if (a.IsObject()) {
                    rapidjson::StringBuffer buffer;
                    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
                    a.Accept(writer);
                    put_content(buffer.GetString());
                }
            }
        }
        return true;
    }

    bool put_content(std::string v) {
        ContentSet set;
        std::string tmpStr;
        rapidjson::Document document;
        if (document.Parse(v.c_str()).HasParseError()) {
            LOG(ERROR) << "Invalid JSON";
            return false;
        }

        if (!document.HasMember("url")) {
            LOG(ERROR) << "Missing 'url' field";
            return false;
        } else {
            set.SetUrl(RawxUrl(document["url"].GetString()));
        }

        if (!document.HasMember("hash")) {
            LOG(ERROR) << "Missing 'hash' field";
            return false;
        } else {
            set.hash = document["hash"].GetString();
        }

        if (!document.HasMember("size")) {
            LOG(ERROR) << "Missing 'size' field";
            return false;
        } else {
            set.size = document["size"].GetInt();
        }

        if (!document.HasMember("score")) {
            LOG(ERROR) << "Missing 'score' field";
            return false;
        } else {
            set.score = document["score"].GetInt();
        }

        if (!document.HasMember("pos")) {
            LOG(ERROR) << "Missing 'pos' field";
            return false;
        } else {
            tmpStr = document["pos"].GetString();
            std::size_t pos = tmpStr.find(".");

            if (pos != std::string::npos) {  // get pos + chunk_number
                tmpStr[pos] = ' ';
                std::stringstream ss(tmpStr);
                ss >> set.pos;
                ss >> set.chunk_number;
            } else {
                std::stringstream ss(tmpStr);
                ss >> set.pos;
                if (set.pos) {
                    set.chunk_number = set.pos;
                } else {
                    std::set<ContentSet>::iterator it = targets.begin();
                    if (targets.size() && it->pos)
                        set.chunk_number = set.pos;
                    else
                        set.chunk_number = targets.size();
                }
            }
        }

        targets.insert(set);

        return true;
    }

    bool get_content(std::string *p, ContentSet *set) {
        rapidjson::StringBuffer buf;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buf);
        writer.StartObject();

        writer.Key("hash");
        writer.String(set->hash.c_str());

        writer.Key("pos");
        writer.String(std::to_string(set->pos).c_str());

        writer.Key("size");
        writer.Int64(set->size);

        writer.Key("url");
        writer.String(set->GetString().c_str());

        writer.EndObject();
        *p = std::string(buf.GetString(), buf.GetSize());
        return true;
    }


    bool get_contents(std::string *p) {
        *p = "[";
        bool bfirst = true;
        for (ContentSet to : targets) {
            if (bfirst)
                bfirst = false;
            else
                *p += ",";
            std::string str;
            get_content(&str, &to);
            *p += str;
        }
        *p += "]";
        return true;
    }

    bool put_properties(std::string p) {
        properties.clear();
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
                    properties[v.name.GetString()] = v.value.GetString();
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

    bool get_properties_key(std::string *p,
            std::set<std::string> *del_properties) {
        *p = "[";

        bool bfirst = false;
        for (const auto &e : *del_properties) {
            if (bfirst)
                *p += ",";
            *p += "\"" + e + "\"";
            bfirst = true;
        }

        *p += "]";
        return true;
    }
};

#endif  // SRC_OIO_CONTENT_COMMAND_H_
