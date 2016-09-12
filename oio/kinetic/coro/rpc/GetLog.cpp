/** Copyright (c) 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#include <utils/utils.h>
#include <kinetic.pb.h>
#include "GetLog.h"

namespace proto = ::com::seagate::kinetic::proto;

using oio::kinetic::rpc::GetLog;

GetLog::GetLog(): Exchange() {
    auto h = req_->cmd.mutable_header();
    h->set_messagetype(proto::Command_MessageType_GETLOG);
	auto types = req_->cmd.mutable_body()->mutable_getlog()->mutable_types();
	types->Add(proto::Command_GetLog_Type::Command_GetLog_Type_CAPACITIES);
	types->Add(proto::Command_GetLog_Type::Command_GetLog_Type_TEMPERATURES);
	types->Add(proto::Command_GetLog_Type::Command_GetLog_Type_UTILIZATIONS);
}

GetLog::~GetLog() {
}

void GetLog::ManageReply(oio::kinetic::rpc::Request &rep) {
	checkStatus(rep);
	const auto &gl = req_->cmd.body().getlog();

	if (gl.has_capacity())
		space = 1.0 - gl.capacity().portionfull();

	if (!gl.temperatures().empty()) {
		for (const auto &t : gl.temperatures()) {
			const double min = t.minimum();
			const double max = t.maximum() - min;
			const double cur = t.current() - min;
			temp = 1.0 - (cur / max);
		}
	}

	if (!gl.utilizations().empty()) {
		for (const auto &u: gl.utilizations()) {
			const double val = 1.0 - u.value();
			if (u.name().compare(0, 3, "CPU"))
				cpu = val;
			if (u.name().compare(0, 2, "HD"))
				io = val;
		}
	}
}

double GetLog::getCpu() const {
	return cpu;
}

double GetLog::getTemp() const {
	return temp;
}

double GetLog::getSpace() const {
	return space;
}

double GetLog::getIo() const {
	return io;
}
