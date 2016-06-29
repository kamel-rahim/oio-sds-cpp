/** Copyright (c) 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#ifndef OIO_KINETIC_MILLDAEMON_H
#define OIO_KINETIC_MILLDAEMON_H

#include <vector>

#include <libmill.h>
#include <rapidjson/filereadstream.h>
#include <rapidjson/document.h>
#include <glog/logging.h>

#include "utils/MillSocket.h"

class BlobClient;

class BlobService;

class BlobDaemon;

class BlobClient {
  public:
	BlobClient(const MillSocket &c) : client(c) {
		DLOG(INFO) << __FUNCTION__;
	}

	~BlobClient() {
		DLOG(INFO) << __FUNCTION__;
	}

	void Run() noexcept {
		std::array<uint8_t, 1024> buf;
		for (; ;) {
			ssize_t r = client.read(buf.data(), buf.size(), mill_now() + 1000);
			if (r > 0)
				DLOG(INFO) << r << " bytes received";
			if (r < 0) {
				if (r == -2)
					DLOG(INFO) << "EOF input";
				break;
			}
		}
	}

  private:
	BlobClient() = delete;

	BlobClient(const BlobClient &o) = delete;

	BlobClient(BlobClient &o) = delete;

	BlobClient(BlobClient &&o) = delete;

  private:
	MillSocket client;
};

class BlobService {
	friend class BlobDaemon;

  public:

	BlobService() : front() {
		done = chmake(uint32_t, 1);
	}

	~BlobService() {
		front.close();
	}

	void Start(volatile bool &flag_running) noexcept {
		mill_go(Run(flag_running));
	}

	void Join() noexcept {
		uint32_t s = chr(done, uint32_t);
		(void) s;
	}

  private:
	NOINLINE void Run(volatile bool &flag_running) noexcept {
		bool input_ready = true;
		while (flag_running) {
			MillSocket client;
			if (input_ready && front.accept(client)) {
				mill_go(RunClient(client));
			} else {
				auto events = front.poll(MILLSOCKET_EVENT_IN,
										 mill_now() + 1000);
				if (events & MILLSOCKET_EVENT_ERR) {
					DLOG(INFO) << "front.poll() error";
					flag_running = false;
				} else {
					input_ready = events & MILLSOCKET_EVENT_IN;
				}
			}
		}
		chs(done, uint32_t, 0);
	}

	NOINLINE void RunClient(MillSocket s0) noexcept {
		BlobClient client(s0);
		client.Run();
	}

  private:
	MillSocket front;
	chan done;
};

class BlobDaemon {
  public:
	BlobDaemon() noexcept: services() {
	}

	~BlobDaemon() noexcept {
	}

	bool LoadFile(const std::string &path) noexcept {
		std::stringstream ss;
		std::ifstream ifs;
		ifs.open(path, std::ios::binary);
		ss << ifs.rdbuf();
		ifs.close();

		rapidjson::Document document;
		if (document.Parse<0>(ss.str().c_str()).HasParseError()) {
			LOG(ERROR) << "Invalid JSON in " << path;
			return false;
		}
		if (!LoadJSON(document))
			return false;
		DLOG(INFO) << "Successfully loaded " << path;
		return true;
	}

	void Start(volatile bool &flag_running) noexcept {
		DLOG(INFO) << __FUNCTION__;
		for (auto &srv: services)
			srv.Start(flag_running);
	}

	void Join() {
		DLOG(INFO) << __FUNCTION__;
		for (auto &srv: services)
			srv.Join();
	}

  private:
	bool LoadJSON(rapidjson::Document &doc) noexcept {
		const char *key_srv = "service";
		if (!doc.HasMember(key_srv)) {
			LOG(INFO) << "Missing [" << key_srv << "] field";
			return true;
		}
		if (!doc[key_srv].IsObject()) {
			LOG(ERROR) << "Unexpected [" << key_srv << "] field: not an object";
			return false;
		}
		if (!doc[key_srv].HasMember("bind")) {
			LOG(ERROR) << "Missing [" << key_srv << "] field";
			return false;
		}
		if (!doc[key_srv]["bind"].IsString()) {
			LOG(ERROR) << "Unexpected [" << key_srv << ".bind] field: not a string";
			return false;
		}
		const auto url = doc[key_srv]["bind"].GetString();
		services.emplace_back();
		auto &srv = services.back();
		if (!srv.front.bind(url)) {
			LOG(ERROR) << "Bind(" << url << ") error";
			return false;
		}
		if (!srv.front.listen(8192)) {
			LOG(ERROR) << "Listen(" << url << ") error";
			return false;
		}
		LOG(INFO) << "Bind(" << url << ") done";
		return true;
	}

  private:
	std::vector<BlobService> services;
};

#endif //OIO_KINETIC_MILLDAEMON_H
