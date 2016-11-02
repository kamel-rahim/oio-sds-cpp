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

#include "oio/api/blob.h"
#include "utils/macros.h"

using oio::api::blob::Download;
using oio::api::blob::Errno;
using oio::api::blob::Status;
using oio::api::blob::Cause;
using oio::api::blob::TransactionStep;

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

std::ostream& oio::api::blob::operator<<(std::ostream &out, const Cause c) {
    out << Status2Str(c);
    return out;
}

std::ostream& oio::api::blob::operator<<(std::ostream &out, const Status s) {
    out << s.Why() << "(" << s.Message() << ")";
    return out;
}

std::ostream& oio::api::blob::operator<<(std::ostream &out,
                                         const TransactionStep s) {
    switch (s) {
        case TransactionStep::Init:
            out << "Init/" << static_cast<int>(s);
            return out;
        case TransactionStep::Prepared:
            out << "Prepared/" << static_cast<int>(s);
            return out;
        case TransactionStep::Done:
            out << "Done/" << static_cast<int>(s);
            return out;
        default:
            out << "Invalid/" << static_cast<int>(s);
            return out;
    }
}

Status Download::SetRange(uint32_t offset UNUSED, uint32_t size UNUSED) {
    return Status(Cause::Unsupported);
}

const char *Status::Name() const { return Status2Str(rc_); }

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
