/** Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#ifndef KINE_KINETIC_MILL_SOCKET_H
#define KINE_KINETIC_MILL_SOCKET_H
#include <string>
#include "Socket.h"

struct iovec;

struct MillSocket {
    Socket sock_;

    ~MillSocket() noexcept {}
    MillSocket () noexcept: sock_() {}
    MillSocket(MillSocket &o) noexcept: sock_{o.sock_} {}

    // move c'tor
    MillSocket(MillSocket &&o) noexcept: sock_{std::move(o.sock_)} {}

    inline int fileno() const noexcept { return sock_.fileno(); }

    void close () noexcept;

    bool connect (const char *u) noexcept {
        return sock_.connect(u);
    }

    bool connect (const std::string &u) noexcept {
        return connect(u.c_str());
    }

    bool bind (const char *u) noexcept  {
        return sock_.bind(u);
    }

    bool listen (int backlog) noexcept {
        return sock_.listen(backlog);
    }

    void setopt (int dom, int opt, int val) noexcept {
        sock_.setopt(dom, opt, val);
    }

    bool accept (MillSocket &cli) noexcept {
        return sock_.accept(cli.sock_);
    }

    std::string debug_string () const noexcept;

    // Returns -2 in case of connection closed
    // Returns -1 in case of error
    // Returns the number of bytes written in the delay
    ssize_t read (uint8_t *buf, size_t len, int64_t dl) noexcept;

    /* Reads exactly max bytes from the current Socket */
    bool read_exactly(uint8_t *buf, size_t len, int64_t dl) noexcept;

    bool send (struct iovec *iov, unsigned int count, int64_t dl) noexcept;

    bool send (uint8_t *buf, size_t len, int64_t dl) noexcept;
};

#endif
