/** Copyright (c) 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#include <glog/logging.h>
#include <utils/utils.h>
#include <kinetic.pb.h>
#include "GetLog.h"


namespace proto = ::com::seagate::kinetic::proto;

using oio::kinetic::rpc::GetLog;

GetLog::GetLog(): Exchange(), cpu{0}, temp{0}, space{0}, io{0} {
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
	const auto &gl = rep.cmd.body().getlog();

	if (gl.has_capacity()) {
		const double usage = gl.capacity().portionfull();
		space = 100.0 * (1.0 - usage);
	} else {
		LOG(ERROR) << "no capacity returned";
	}

	if (!gl.temperatures().empty()) {
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

	if (!gl.utilizations().empty()) {
		for (const auto &u: gl.utilizations()) {
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
