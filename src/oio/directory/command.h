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

#ifndef SRC_OIO_DIRECTORY_COMMAND_H_
#define SRC_OIO_DIRECTORY_COMMAND_H_

#include <set>
#include <map>
#include <string>
#include <algorithm>
#include <iomanip>

#include "openssl/sha.h"
#include "utils/serialize_def.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"

class OioUrl {
 private:
    std::string ns;
    std::string account;
    std::string container;
    std::string type;
    std::string path;
    std::string cid;

 private:
    void compute_container_id() {
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256_CTX sha256;
        SHA256_Init(&sha256);

        static const char zero = 0;

        SHA256_Update(&sha256, account.c_str(), account.size());
        SHA256_Update(&sha256, &zero, 1);
        SHA256_Update(&sha256, container.c_str(), container.size());

        SHA256_Final(hash, &sha256);
        std::stringstream ss;
        for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
            ss << std::uppercase << std::hex << std::setw(2)
               << std::setfill('0') << static_cast<int> (hash[i]);
        }

        cid.assign(ss.str());
    }

 public:
    OioUrl() : ns(), account(), container(), type(), path(), cid() {}

    explicit OioUrl(const OioUrl &o) : ns(o.ns), account(o.account),
                                       container(o.container), type(o.type),
                                       path(o.path), cid(o.cid) {}

    OioUrl(const std::string _ns,
            const std::string _acct,
            const std::string _cname,
            const std::string _type = "",
            const std::string _cpath = "") : ns(_ns), account(_acct),
                                             container(_cname), type(_type),
                                             path(_cpath), cid() {}

    void Set(const OioUrl &o) {
        ns.assign(o.ns);
        account.assign(o.account);
        container.assign(o.container);
        type.assign(o.type);
        path.assign(o.path);
        cid.assign(o.cid);
    }

    OioUrl &operator=(const OioUrl &arg) {
        Set(arg);
        return *this;
    }

    OioUrl &GetFile() { return *this; }

    const std::string &NS() const { return ns; }

    const std::string &Account() const { return account; }

    const std::string &Container() const { return container; }

    const std::string &Type() const { return type; }

    const std::string &Filename() const { return path; }

    const std::string &ContainerId() const { return cid; }

    std::string URL() const {
        std::stringstream ss;
        ss << ns << '/' << account << '/' << container
           << '/' << type << '/' + path;
        return ss.str();
    }
};

class SrvID {
 protected:
    std::string host;
    int port;

 public:
    ~SrvID() {}

    SrvID() {}

    SrvID(const std::string _host, int _port) : host(_host), port(_port) {}

    SrvID(const SrvID &o) : host(o.host), port{o.port} {}

    SrvID(SrvID &&o) : host(), port{o.port} { host.swap(o.host); }

    void Set(const SrvID &o) {
        host.assign(o.host);
        port = o.port;
    }

    bool Parse(const std::string packed) {
        auto colon = packed.rfind(':');
        if (colon == packed.npos)
            return false;
        host.assign(packed, 0, colon);
        auto strport = std::string(packed, colon + 1);
        port = ::atoi(strport.c_str());
        return true;
    }
};

class DirURL {
 protected:
    int seq;
    std::string type;
    std::string args;
    SrvID host;

 public:
    ~DirURL() {}

    DirURL() {}

    DirURL(const std::string _type, std::string _h, int _p, int _seq = 1) :
            seq{_seq}, type(_type), host(_h, _p) {}

    DirURL &operator=(const DirURL &arg) {
        type.assign(arg.type);
        host.Set(arg.host);
        args = arg.args;
        return *this;
    }

    bool operator<(const DirURL &a) const { return type < a.type; }

    bool Parse(std::string v) {
        std::string tmpStr;
        rapidjson::Document document;
        if (document.Parse(v.c_str()).HasParseError()) {
            LOG(ERROR) << "Invalid JSON";
            return false;
        }

        if (!document.HasMember("seq")) {
            LOG(ERROR) << "Missing 'seq' field";
            return false;
        }
        if (!document.HasMember("type")) {
            LOG(ERROR) << "Missing 'type' field";
            return false;
        }
        if (!document.HasMember("host")) {
            LOG(ERROR) << "Missing 'host' field";
            return false;
        }
        if (!document.HasMember("args")) {
            LOG(ERROR) << "Missing 'args' field";
            return false;
        }

        if (!host.Parse(document["host"].GetString()))
            return false;

        seq = document["seq"].GetInt();
        type.assign(document["type"].GetString());
        args.assign(document["args"].GetString());
        return true;
    }
};

class DirPayload {
 private:
    std::set<DirURL> metas;
    std::map<std::string, std::string> properties;

 public:
    DirPayload() {}

    DirPayload &operator=(const DirPayload &arg) {
        metas.clear();
        metas.insert(arg.metas.begin(), arg.metas.end());
        return *this;
    }

    std::string &operator[](std::string key) { return properties[key]; }

    bool put_properties(std::string s) {
        std::string v = s;
        std::string u = "{}\"";
        for (const auto &p : u)  // strip  {}"
            remove_p(v, p);
        std::stringstream ss(v);

        std::string tmpStr;
        // read properties keyword
        read_and_validate(ss, tmpStr, "properties", ':');

        std::string key;
        std::string value;
        properties.clear();
        for (;;) {
            read_any(ss, key, ':');
            read_any(ss, value, ',');
            if (!key.size())
                break;
            properties[key] = value;
            key.clear();
            value.clear();
        }
        return true;
    }

    bool get_properties(std::string *s) {
        std::stringstream ss;
        ss << "{\"properties\":{\"";

        bool bfirst = true;
        for (const auto &e : properties) {
            if (!bfirst)
                ss << ",\"";
            ss << e.first << "\":\"" << e.second << "\"";
            bfirst = false;
        }

        ss << "}}";

        *s = ss.str();
        return true;
    }

 public:
    bool Parse(std::string p) {
        rapidjson::Document document;
        if (document.Parse(p.c_str()).HasParseError()) {
            LOG(ERROR) << "Invalid JSON";
            return false;
        }

        if (!document.HasMember("dir")) {
            LOG(ERROR) << "Missing 'properties' field";
            return false;
        } else {
            const rapidjson::Value &obj = document["dir"];
            if (obj.IsArray()) {
                for (auto &a : obj.GetArray()) {
                    if (a.IsObject()) {
                        rapidjson::StringBuffer buffer;
                        rapidjson::Writer<rapidjson::StringBuffer>
                                writer(buffer);
                        a.Accept(writer);
                        DirURL url;
                        if (url.Parse(buffer.GetString()))
                            metas.insert(url);
                    }
                }
            }
        }

        if (!document.HasMember("srv")) {
            LOG(ERROR) << "Missing 'system' field";
            return false;
        } else {
            const rapidjson::Value &obj = document["srv"];
            if (obj.IsObject()) {
                for (auto &v : obj.GetObject()) {
                    DirURL url;
                    if (url.Parse(v.value.GetString()))
                        metas.insert(url);
                }
            }
        }

        return true;
    }
};

#endif  // SRC_OIO_DIRECTORY_COMMAND_H_
