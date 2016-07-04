/** Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#ifndef OIO_BLOB__SRC__SOCKET_H
#define OIO_BLOB__SRC__SOCKET_H 1

#include <string>
#include "Addr.h"

extern int default_backlog;
extern bool default_reuse_port;

class Socket {
  protected:
	int fd_;
	Addr peer_;
	Addr local_;
  public:

	~Socket() { }

	Socket(): fd_{-1} { }

	Socket(Socket &&o): fd_{o.fd_}, peer_{o.peer_}, local_{o.local_} {
		o.reset();
	}

	Socket(const Socket &o): fd_{o.fd_}, peer_{o.peer_},
									  local_{o.local_} { }

	int fileno() const { return fd_; }

	void reset();

	void init(int family);

	void close();

	bool connect(const char *u);

	bool bind(const char *u);

	bool listen(int backlog);

	bool setopt(int dom, int opt, int val);

	bool accept(Socket &cli);

	std::string debug_string() const;
};

#endif // OIO_BLOB__SRC__SOCKET_H
