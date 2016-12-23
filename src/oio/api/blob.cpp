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
using oio::api::blob::TransactionStep;
using oio::api::Cause;
using oio::api::Status;

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
