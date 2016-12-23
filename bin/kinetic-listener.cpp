/**
 * This file is part of the CLI tools around the OpenIO client libraries
 * Copyright (C) 2016 OpenIO SAS
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <sys/socket.h>
#include <arpa/inet.h>

#include <rapidjson/document.h>
#include <rapidjson/filereadstream.h>
#include <rapidjson/writer.h>
#include <libmill.h>

#include <queue>

#include "utils/utils.h"
#include "utils/net.h"
#include "utils/Http.h"
#include "oio/blob/kinetic/coro/rpc/GetLog.h"
#include "oio/blob/kinetic/coro/client/CoroutineClient.h"
#include "oio/blob/kinetic/coro/client/CoroutineClientFactory.h"

#ifndef PERIOD_REG
#define PERIOD_REG          5000
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
using oio::kinetic::client::CoroutineClient;
using oio::kinetic::client::Sync;
using oio::kinetic::rpc::Exchange;
using oio::kinetic::rpc::GetLog;

static std::map<std::string, std::string> REGISTRATIONS;

static char nsname[512];
static char url_proxy[512];
static char srvtype[32] = OIO_KINE_DEFAULT_SRVTYPE;

static CoroutineClientFactory factory;

static volatile bool flag_running{true};

struct stat_s {
    double cpu, io, space, temp;
};

static void
proxy_register(const std::string &id, const std::string &url, const stat_s st) {
    // then forward them to the proxy
    std::shared_ptr<net::Socket> client(new net::MillSocket);
    client->connect(url_proxy);

    rapidjson::StringBuffer buf;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buf);

    LOG(INFO) << "Registering " << id << " url=" << url;

    // Pack the REGISTRATIONS
    writer.StartObject();
    writer.Key("ns");
    writer.String(nsname);
    writer.Key("type");
    writer.String(srvtype);
    writer.Key("id");
    writer.String(id.c_str());
    writer.Key("addr");
    writer.String(url.c_str());
    writer.Key("tags");
    writer.StartObject();

    writer.Key("tag.up");
    writer.Bool(true);
    writer.Key("stat.cpu");
    writer.Double(st.cpu);
    writer.Key("stat.io");
    writer.Double(st.io);
    writer.Key("stat.space");
    writer.Double(st.space);
    writer.Key("stat.temp");
    writer.Double(st.temp);

    writer.EndObject();
    writer.Key("score");
    writer.Int(-2);
    writer.EndObject();

    std::string out;
    http::Call req(client);
    req.Method("POST")
            .Selector("/v3.0/" + std::string(nsname) + "/conscience/register")
            .Field("Host", "proxy")
            .Field("Connection", "keep-alive")
            .Field("Content-Type", "application/json");

    auto rc = req.Run(buf.GetString(), &out);
    LOG_IF(WARNING, rc != http::Code::OK) << "Registration failed";

    client->close();
}

/**
 * Collect the stats aboud 'id' and push the registration to the proxy
 * @param id the UUID of the service
 * @param url the network endpoint of the service
 */
static void monitoring_round(const std::string id, const std::string url) {
    GetLog op;
    do {
        auto client = factory.Get(url);
        auto sync = client->RPC(&op);
        sync->Wait();
        if (!op.Ok()) {
            LOG(INFO) << "Stat [" << id << "] FAILED";
            return;
        }
    } while (0);
    proxy_register(id, url, {op.getCpu(), op.getIo(),
                             op.getSpace(), op.getTemp()});
    DLOG(INFO) << "Stat [" << id << "] done";
}

static coroutine void monitoring_loop(const std::string &id0) {
    // Immediately make a copy of the input ID, so that the original string
    // may be freed but it exists a copy in the ccurrent stack frame.
    std::string id(id0);
    DLOG(INFO) << "Monitoring [" << id << "]";
    yield();

    int64_t next_check = mill_now();
    while (flag_running) {
        monitoring_round(id, REGISTRATIONS[id]);
        next_check += PERIOD_REG;
        if (mill_now() < next_check)
            msleep(next_check);
    }
}

static void manage_message(const rapidjson::Document &msg) {
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

    std::stringstream ss;
    ss << IPV4.GetString() << ":" << PORT.GetInt();
    const auto uuid = WWN.GetString();
    const auto url = ss.str();

    // We have a full registration, save or update it
    auto it = REGISTRATIONS.find(uuid);
    if (it == REGISTRATIONS.end()) {
        REGISTRATIONS[uuid] = url;
        mill_go(monitoring_loop(uuid));
    } else if (it->second != ss.str()) {
        REGISTRATIONS[uuid] = url;
    }
}

static coroutine void consumer(int fd, chan done) {
    struct sockaddr_in sa;
    std::array<uint8_t, 4096> buffer;
    while (flag_running) {
        memset(&sa, 0, sizeof(sa));
        socklen_t salen = sizeof(sa);
        auto rc = ::recvfrom(fd, buffer.data(), buffer.size() - 1, 0,
                             (struct sockaddr *) &sa, &salen);
        if (rc < 0) {
            if (errno == EINTR) {
                continue;
            } else if (errno == EAGAIN) {
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
            LOG(ERROR) << "Invalid Frame";
        } else {
            manage_message(msg);
        }
    }
    chs(done, bool, true);
}

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

int main(int argc UNUSED, char **argv) {
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    google::InitGoogleLogging(argv[0]);

    if (argc != 3) {
        LOG(ERROR) << "Usage: " << argv[0] << " NS URL";
        return 1;
    }

    ::strncpy(nsname, argv[1], sizeof(nsname));
    ::strncpy(url_proxy, argv[2], sizeof(url_proxy));
    DLOG(INFO) << "Configuration: NS=" << nsname << " PROXY=" << url_proxy;

    int fd = make_kinetic_socket();
    if (fd < 0)
        return 1;
    DLOG(INFO) << "mcast socket fd=" << fd;

    chan done = chmake(bool, 2);
    mill_go(consumer(fd, done));
    chr(done, bool);
    chclose(done);

    ::fdclean(fd);
    ::close(fd);
    fd = -1;
    return 0;
}
