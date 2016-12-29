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

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <rapidjson/document.h>
#include "utils/macros.h"

#include "oio/api/common.h"

using oio::api::Status;
using oio::api::Cause;
using oio::api::Errno;
using oio::api::OioError;

static inline const char *Status2Str(Cause s) {
    switch (s) {
        case Cause::OK:
            return "OK";
        case Cause::Already:
            return "Already";
        case Cause::Forbidden:
            return "Forbidden";
        case Cause::NotFound:
            return "NotFound";
        case Cause::NetworkError:
            return "NetworkError";
        case Cause::ProtocolError:
            return "ProtocolError";
        case Cause::InternalError:
            return "InternalError";
        case Cause::Unsupported:
            return "Unsupported";
        default:
            return "***invalid status***";
    }
}

std::ostream& oio::api::operator<<(std::ostream &out, const Cause c) {
    out << Status2Str(c);
    return out;
}

std::ostream& oio::api::operator<<(std::ostream &out, const Status s) {
    out << s.Why() << "(" << s.Message() << ")";
    return out;
}

const char *Status::Name() const { return Status2Str(rc_); }

std::string Status::Encode() const {
    rapidjson::StringBuffer buf;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buf);
    writer.StartObject();
    writer.Key("status");
    writer.Int(code_);
    writer.Key("message");
    writer.String(msg_.c_str());
    writer.EndObject();
    return std::move(buf.GetString());
}

bool OioError::Decode(const std::string &encoded) {
    rapidjson::Document doc;

    if (doc.Parse(encoded.c_str()).HasParseError())
        return false;
    if (!doc.HasMember("status") || !doc["status"].IsInt())
        return false;
    if (!doc.HasMember("message") || !doc["message"].IsString())
        return false;

    code_ = doc["status"].GetInt();
    msg_.assign(doc["message"].GetString());
    return true;
}

void OioError::mapCode() {
    switch (code_) {
        case 420:
        case 431:
            rc_ = Cause::NotFound;
            return;
        default:
            rc_ = Cause::InternalError;
            return;
    }
}

Cause _map(int err) {
    switch (err) {
        case ENOENT:
            return Cause::NotFound;
        case EEXIST:
            return Cause::Already;
        case ECONNRESET:
        case ECONNREFUSED:
        case ECONNABORTED:
        case EPIPE:
            return Cause::NetworkError;
        case EACCES:
        case EPERM:
        case ENOTDIR:
            return Cause::Forbidden;
        case ENOTSUP:
            return Cause::Unsupported;
        default:
            return Cause::InternalError;
    }
}

Errno::Errno(int err) : Status() {
    if (err != 0)
        rc_ = _map(err);
    msg_.assign(::strerror(err));
}

Errno::Errno() : Errno(errno) {}
