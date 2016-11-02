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

#include <src/kinetic.pb.h>

#include <algorithm>

#include "utils/macros.h"
#include "utils/utils.h"
#include "oio/kinetic/coro/rpc/GetLog.h"

namespace proto = ::com::seagate::kinetic::proto;

using oio::kinetic::rpc::GetLog;

GetLog::GetLog() : Exchange(), cpu{0}, temp{0}, space{0}, io{0} {
    auto h = cmd.mutable_header();
    h->set_messagetype(proto::Command_MessageType_GETLOG);
    auto types = cmd.mutable_body()->mutable_getlog()->mutable_types();
    types->Add(proto::Command_GetLog_Type::Command_GetLog_Type_CAPACITIES);
    types->Add(proto::Command_GetLog_Type::Command_GetLog_Type_TEMPERATURES);
    types->Add(proto::Command_GetLog_Type::Command_GetLog_Type_UTILIZATIONS);
}

GetLog::~GetLog() {}

void GetLog::ManageReply(oio::kinetic::rpc::Request *rep) {
    checkStatus(rep);
    const auto &gl = rep->cmd.body().getlog();

    if (gl.has_capacity()) {
        const double usage = gl.capacity().portionfull();
        space = 100.0 * (1.0 - usage);
    } else {
        LOG(ERROR) << "no capacity returned";
    }

    // TODO(jfs) replace the check on size() by empty()
    // as soon as empty() is available on all target distros
    if (0 < gl.temperatures().size()) {
        double t0 = 100.0;
        for (const auto &temperature : gl.temperatures()) {
            const double min = temperature.minimum();
            const double max = temperature.maximum() - min;
            const double cur = temperature.current() - min;
            const double t = 100.0 * (1.0 - (cur / max));
            t0 = std::min(t0, t);
        }
        temp = t0;
    } else {
        LOG(ERROR) << "no temperature returned";
    }

    // TODO(jfs) replace the check on size() by empty()
    // as soon as empty() is available on all target distros
    if (0 < gl.utilizations().size()) {
        for (const auto &u : gl.utilizations()) {
            const double val = 100.0 * (1.0 - u.value());
            if (0 == u.name().compare(0, 3, "CPU"))
                cpu = val;
            if (0 == u.name().compare(0, 2, "HD"))
                io = val;
        }
    } else {
        LOG(ERROR) << "no cpu/disk utilization returned";
    }
}

double GetLog::getCpu() const { return cpu; }

double GetLog::getTemp() const { return temp; }

double GetLog::getSpace() const { return space; }

double GetLog::getIo() const { return io; }
