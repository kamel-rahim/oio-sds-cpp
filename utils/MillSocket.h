/** Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#ifndef OIO_BLOB__SRC__MILLSOCKET_H
#define OIO_BLOB__SRC__MILLSOCKET_H
#include <string>
#include "Socket.h"

struct iovec;

#define MILLSOCKET_EVENT_OUT 1
#define MILLSOCKET_EVENT_IN  2
#define MILLSOCKET_EVENT_ERR 4

struct MillSocket {
    Socket sock_;

    ~MillSocket() {}
    MillSocket (): sock_() {}
    MillSocket(const MillSocket &o): sock_{o.sock_} {}

    // move c'tor
    MillSocket(MillSocket &&o): sock_{std::move(o.sock_)} {}

    inline int fileno() const { return sock_.fileno(); }

    void close ();

    /* Wraps libmill's fdwait().
     * This wrapper copes with libmill's early stage and lack of API stability.
     * In facts, functions (in their prefixed form) are still sometimes renamed,
     * thus breaking calling code. */
    unsigned int poll(unsigned int what, int64_t dl);

    bool connect (const char *u) {
        return sock_.connect(u);
    }

    bool connect (const std::string &u) {
        return connect(u.c_str());
    }

    bool bind (const char *u)  {
        return sock_.bind(u);
    }

    bool listen (int backlog) {
        return sock_.listen(backlog);
    }

    void setopt (int dom, int opt, int val) {
        sock_.setopt(dom, opt, val);
    }

    bool accept (MillSocket &cli) {
        return sock_.accept(cli.sock_);
    }

    std::string debug_string () const;

    // Returns -2 in case of connection closed
    // Returns -1 in case of error
    // Returns the number of bytes written in the delay
    ssize_t read (uint8_t *buf, size_t len, int64_t dl);

    /* Reads exactly max bytes from the current Socket */
    bool read_exactly(uint8_t *buf, size_t len, int64_t dl);

    bool send (struct iovec *iov, unsigned int count, int64_t dl);

    bool send (const uint8_t *buf, size_t len, int64_t dl);

    bool send (const char *str, size_t len, int64_t dl) {
        return this->send(reinterpret_cast<const uint8_t *>(str), len, dl);
    }
};

#endif // OIO_BLOB__SRC__MILLSOCKET_H
