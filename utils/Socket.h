/** Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#ifndef OIO_KINETIC_UTILS__SOCKET_H
#define OIO_KINETIC_UTILS__SOCKET_H 1
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

	~Socket() noexcept {}
    Socket () noexcept: fd_{-1} {}
    Socket(Socket &&o) noexcept: fd_{o.fd_}, peer_{o.peer_}, local_{o.local_} { o.reset(); }
    Socket(Socket &o) noexcept: fd_{o.fd_}, peer_{o.peer_}, local_{o.local_} {}

    int fileno () const noexcept { return fd_; }

    void reset () noexcept;
    void init (int family) noexcept;
    void close () noexcept;
    bool connect (const char *u) noexcept;
    bool bind (const char *u) noexcept;
    bool listen (int backlog) noexcept;
    bool setopt (int dom, int opt, int val) noexcept;

    bool accept(Socket &cli) noexcept;
	std::string debug_string () const noexcept;
};

#endif
