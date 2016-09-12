/** Copyright (c) 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */
#include <cerrno>
#include <thread>
#include <mutex>
#include <queue>

#include <sys/socket.h>
#include <arpa/inet.h>

#include <glog/logging.h>
#include <rapidjson/document.h>
#include <rapidjson/filereadstream.h>
#include <rapidjson/writer.h>
#include <libmill.h>

#include <utils/utils.h>
#include <utils/net.h>
#include <utils/Http.h>

#include "oio/kinetic/coro/rpc/GetLog.h"
#include "oio/kinetic/coro/client/CoroutineClient.h"
#include "oio/kinetic/coro/client/CoroutineClientFactory.h"

#ifndef PERIOD_REG
#define PERIOD_REG          4000
#endif

#ifndef KINETIC_MCAST_PORT
#define KINETIC_MCAST_PORT  8123
#endif

#ifndef KINETIC_MCAST_IP
#define KINETIC_MCAST_IP    "239.1.2.3"
#endif

#ifndef OIO_KINE_DEFAULT_SRVTYPE
#define OIO_KINE_DEFAULT_SRVTYPE "kine"
#endif

#define PORT  msg["port"]
#define WWN   msg["world_wide_name"]
#define ITF   msg["network_interfaces"]
#define ITF1  ITF[0]
#define IPV4  ITF1["ipv4_addr"]

using oio::kinetic::client::ClientInterface;
using oio::kinetic::client::CoroutineClientFactory;
using oio::kinetic::client::Sync;
using oio::kinetic::rpc::Exchange;
using oio::kinetic::rpc::GetLog;

struct oio_srv_stats {
	oio_srv_stats(double c, double i, double s, double t) : cpu{c}, io{i},
															space{s}, temp{t} {}
	oio_srv_stats(): cpu{0}, io{0}, space{0}, temp{0} {}
	double cpu, io, space, temp;
};

std::map<std::string, std::string> REGISTRATIONS;
std::map<std::string, oio_srv_stats> STATS;

std::string nsname;
std::string url_proxy;
std::string srvtype{OIO_KINE_DEFAULT_SRVTYPE};

CoroutineClientFactory factory;

static volatile bool flag_running{true};

static int make_kinetic_socket(void) {
	int fd = ::socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0);
	if (fd < 0) {
		LOG(FATAL) << "Socket creation failed: (" << errno << ") " <<
				   ::strerror(errno);
		return -1;
	}

	struct sockaddr_in sa;
	memset(&sa, 0, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = inet_addr(KINETIC_MCAST_IP);
	sa.sin_port = htons(KINETIC_MCAST_PORT);
	socklen_t salen = sizeof(sa);
	if (0 > ::bind(fd, (struct sockaddr *) &sa, salen)) {
		LOG(FATAL) << "Socket bind failed: (" << errno << ") "
				   << ::strerror(errno);
		::close(fd);
		return -1;
	}

	struct ip_mreq mr;
	memset(&mr, 0, sizeof(mr));
	mr.imr_multiaddr.s_addr = inet_addr(KINETIC_MCAST_IP);
	mr.imr_interface.s_addr = htonl(INADDR_ANY);
	if (0 > setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mr, sizeof(mr))) {
		LOG(FATAL) << "Socket multicast failed: (" << errno << ") " <<
				   ::strerror(errno);
		::close(fd);
		return -1;
	}

	return fd;
}

/**
 * Returns a map of <key,value> tags for the service identified by 'url'
 * @param id the network endpoint of the service
 * @param url the network endpoint of the service
 * @return the tags for the service
 */
static NOINLINE bool
qualify(const std::string id, const std::string url) {
	GetLog op;
	auto client = factory.Get(url);
	auto sync = client->Start(&op);
	sync->Wait();
	if (!op.Ok()) {
		LOG(INFO) << "Failed to stat " << url;
		return false;
	} else {
		STATS[id] = oio_srv_stats(op.getCpu(), op.getIo(), op.getSpace(),
								  op.getTemp());
		DLOG(INFO) << "Stat " << id << "/" << url << " -> ok";
		return true;
	}
}

static NOINLINE void push() {

	if (REGISTRATIONS.empty()) {
		DLOG(INFO) << "No registration this turn!";
		return;
	}

	DLOG(INFO) << "Sending " << REGISTRATIONS.size() << " registrations";

	std::map<std::string, std::string> registrations;
	registrations.swap(REGISTRATIONS);

	// then forward them to the proxy
	std::shared_ptr<net::Socket> client(new net::MillSocket);
	client->connect(url_proxy);

	for (const auto reg: registrations) {
		rapidjson::StringBuffer buf;
		rapidjson::Writer<rapidjson::StringBuffer> writer(buf);

		auto qualified = qualify(reg.first, reg.second);

		// Pack the REGISTRATIONS
		writer.StartObject();
		writer.Key("ns");
		writer.String(nsname.c_str());
		writer.Key("type");
		writer.String(srvtype.c_str());
		writer.Key("id");
		writer.String(reg.first.c_str());
		writer.Key("addr");
		writer.String(reg.second.c_str());
		writer.Key("tags");
		writer.StartObject();
		writer.Key("tag.up");
		if (!qualified) {
			writer.Bool(false);
		} else {
			writer.Bool(true);
			const auto st = STATS[reg.first];
			writer.Key("stat.cpu");
			writer.Double(st.cpu);
			writer.Key("stat.io");
			writer.Double(st.io);
			writer.Key("stat.space");
			writer.Double(st.space);
			writer.Key("stat.temp");
			writer.Double(st.temp);
		}
		writer.EndObject();
		writer.Key("score");
		writer.Int(-2);
		writer.EndObject();

		std::string out;
		http::Call req(client);
		req.Method("POST")
				.Selector("/v3.0/" + nsname + "/conscience/register")
				.Field("Host", "proxy")
				.Field("Connection", "keep-alive")
				.Field("Content-Type", "application/json");

		auto rc = req.Run(buf.GetString(), out);
		LOG_IF(WARNING, rc != http::Code::OK) << "Registration failed";
	}
	registrations.clear();

	client->close();
}

static NOINLINE void push_and_set_deadline(int64_t &next_dl) {
	push();
	assert(REGISTRATIONS.empty());
	next_dl = mill_now() + PERIOD_REG;
}

static coroutine void registrar(chan done, chan wake) {
	int64_t next_dl = mill_now();
	while (flag_running) {
		mill_choose {
				mill_in(wake, int, s):
					if (REGISTRATIONS.size() >= 256)
						push_and_set_deadline(next_dl);
					(void) s;
				mill_deadline(next_dl):
					push_and_set_deadline(next_dl);
			mill_end
		}
	}
	chs(done, bool, true);
}

static void manage_message(chan wake, rapidjson::Document &msg) {

	// Schema validation
	if (!msg.HasMember("network_interfaces") || !ITF.IsArray() ||
		ITF.Size() <= 0)
		return;
	if (!ITF1.HasMember("ipv4_addr") || !IPV4.IsString())
		return;
	if (!msg.HasMember("port") || !PORT.IsInt())
		return;
	if (!msg.HasMember("world_wide_name") || !WWN.IsString())
		return;

	// We have a full registration, save it
	std::stringstream ss;
	ss << IPV4.GetString() << ":" << PORT.GetInt();
	REGISTRATIONS[WWN.GetString()] = ss.str();

	// Ping the sender sender coroutine
	chs(wake, int, 1);
}

static coroutine void consumer(int fd, chan done, chan wake) {
	struct sockaddr_in sa;
	std::array<uint8_t, 4096> buffer;
	while (flag_running) {
		memset(&sa, 0, sizeof(sa));
		socklen_t salen = sizeof(sa);
		auto rc = ::recvfrom(fd, buffer.data(), buffer.size() - 1, 0,
							 (struct sockaddr *) &sa, &salen);
		if (rc < 0) {
			if (errno == EINTR)
				continue;
			else if (errno == EAGAIN) {
				int evt = fdwait(fd, FDW_IN, mill_now() + 5000);
				if (evt & FDW_ERR)
					break;
				continue;
			} else {
				break;
			}
		}
		if (rc == 0)
			break;

		buffer[rc] = 0;
		rapidjson::Document msg;
		if (msg.Parse<0>((const char *) buffer.data()).HasParseError()) {
			LOG_EVERY_N(ERROR, 512) << "Invalid Frame";
		} else {
			manage_message(wake, msg);
		}
	}
	chs(done, bool, true);
}

int main(int argc UNUSED, char **argv) {
	google::InitGoogleLogging(argv[0]);
	FLAGS_logtostderr = true;

	if (argc != 3) {
		LOG(ERROR) << "Usage: " << argv[0] << " NS URL";
		return 1;
	}

	nsname.assign(argv[1]);
	url_proxy.assign(argv[2]);
	DLOG(INFO) << "Configuration: NS=" << nsname << " PROXY=" << url_proxy;

	int fd = make_kinetic_socket();
	if (fd < 0)
		return 1;
	DLOG(INFO) << "mcast socket fd=" << fd;

	chan done = chmake(bool, 2);
	chan wake = chmake(int, 0);
	mill_go(consumer(fd, done, wake));
	mill_go(registrar(done, wake));
	chr(done, bool);
	chr(done, bool);
	chclose(wake);
	chclose(done);

	::fdclean(fd);
	::close(fd);
	fd = -1;
	return 0;
}
