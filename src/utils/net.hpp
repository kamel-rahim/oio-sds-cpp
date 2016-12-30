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

#ifndef SRC_UTILS_NET_HPP_
#define SRC_UTILS_NET_HPP_

#include <cstdint>
#include <string>
#include <memory>

#include "./macros.h"

#define MILLSOCKET_EVENT 1U
#define MILLSOCKET_ERROR 2U

// Forward declarations in the main C++ namespace, to match the real definition
// in system headers.
struct iovec;
struct sockaddr;

namespace net {

extern bool default_reuse_port;

/**
 * Large enough for a IPv6 and IPv4 addresses.
 * DO NOT access fields directly!
 */
struct NetAddr {
    int32_t family_;
    uint16_t port;
    uint8_t addr[16];
    uint8_t padding[2];
};

/**
 * Gathers all the fields necessary as a parameter for bind() and connect()
 */
struct Addr {
    NetAddr ss_;
    uint32_t len_;

    Addr();
    Addr(const Addr &o);
    Addr(Addr &&o);

    bool parse(const char *url);

    bool parse(const std::string u);

    void reset();

    /**
     * Get the CIDR form of the address.
     * @return
     */
    std::string Url() const;

    int family() const;
    std::string family_name() const;
    int port() const;
    struct sockaddr* address();
    const struct sockaddr* address() const;


    Addr& operator=(const Addr &o);
};


/**
 * An interface for two-way communication channels.
 */
class Channel {
 public:
    virtual ~Channel() {}

    virtual std::string Debug() const = 0;

    virtual ssize_t read(uint8_t *buf, size_t len, int64_t dl) = 0;

    /* Reads exactly max bytes from the current Socket */
    virtual bool read_exactly(uint8_t *buf, size_t len, int64_t dl) = 0;

    virtual bool send(struct iovec *iov, unsigned int count, int64_t dl) = 0;

    virtual bool send(const uint8_t *buf, size_t len, int64_t dl) = 0;

    virtual bool send(const char *str, size_t len, int64_t dl) {
        return this->send(reinterpret_cast<const uint8_t *>(str), len, dl);
    }

 protected:
    /**
     * No-Op by default, allows yielding another execution context when no wait
     * was necessary, to avoid starvations.
     */
    virtual void switch_context() {}
};

/**
 * Wraps a file descriptor, with some of the common operations.
 */
class Socket : public Channel {
 protected:
    int fd_;
    Addr peer_;
    Addr local_;

 public:
    /**
     * Destructor. DOESN'T CLOSE THE SOCKET
     */
    virtual ~Socket() {}

    /**
     * Default Socket constructor
     * @return
     */
    Socket() : fd_{-1} {}

    /**
     * Wait for the output to be ready
     * @param dl the deadline in monotonic time, in microseconds
     * @return 0 if the deadline was reached without event, or an OR'ed
     * combination of MILLSOCKET_EVENT and MILLSOCKET_ERROR if, respectively, an
     * event or an error occured.
     */
    virtual unsigned int PollOut(int64_t dl) = 0;

    /**
     * Wait for the input to be ready
     * @param dl the deadline in monotonic time, in microseconds
     * @return 0 if the deadline was reached without event, or an OR'ed
     * combination of MILLSOCKET_EVENT and MILLSOCKET_ERROR if, respectively, an
     * event or an error occured.
     */
    virtual unsigned int PollIn(int64_t dl) = 0;

    virtual Socket* accept() = 0;

    /**
     * Accepts a connection on the current server socket.
     * @param peer cannot be null
     * @param local cannot be null
     * @return the file descriptor newly accepted
     */
    int accept_fd(Addr *peer, Addr *local);

    inline int fileno() const { return fd_; }

    void reset();

    void init(int family);

    virtual void close();

    bool connect(const std::string s);

    bool connect(const char *u);

    /**
     * Re-establish the connection to target previously attached
     * @return
     */
    bool Reconnect();

    bool bind(const char *u);

    bool listen(int backlog);

    /**
     * General purpose wrapper around ::setsockopt().
     * @param dom
     * @param opt
     * @param val
     * @return a bollean value indicating if the function succeeded.
     */
    bool setopt(int dom, int opt, int val);

    /**
     * Wraps setopt() with IPPROTO_TCP/TCP_NODELAY
     * Disables Nagle algorithm, frames are not delayed. Reduces lantency as
     * well as the throughput.
     * @see setopt().
     */
    bool setnodelay();

    /**
     * Wraps setopt() with IPPROTO_TCP/TCP_CORK
     * Antagonist with NODELAY, doesn't allow sending partial frames. Increases
     * latency on small and chatty communications, but increases throughput on
     * large and fat transfers.
     * @see setopt().
     */
    bool setcork();

    /**
     * Wraps setopt() with IPPROTO_TCP/TCP_QUICKACK
     * Immediately replies to tcp PSH, so it doesn't agregate ACK, reduces latency
     * small and chatty communications, but increases the number of messages on
     * the network as well and the CPU on both sides.
     * @see setopt().
     */
    bool setquickack();

    bool setsndbuf(int size);

    bool setrcvbuf(int size);

    std::string Debug() const override;

    /**
     * Reads some bytes from the socket, and why at most until 'dl' is reached
     * if no byte is available.
     * @param buf the place to store the bytes
     * @param len the maximum number of bytes to be read (i.e. buf must be at
     * least 'len' bytes long)
     * @param dl the deadline.
     * @return -2 in case of connection closed, -1 in case of error, or number
     * of bytes written in the delay
     */
    ssize_t read(uint8_t *buf, size_t len, int64_t dl) override;

    /* Reads exactly max bytes from the current Socket */
    bool read_exactly(uint8_t *buf, const size_t len, int64_t dl) override;

    bool send(struct iovec *iov, unsigned int count, int64_t dl) override;

    bool send(const uint8_t *buf, size_t len, int64_t dl) override;

    virtual bool send(const char *str, size_t len, int64_t dl) {
        return this->send(reinterpret_cast<const uint8_t *>(str), len, dl);
    }

 private:
    FORBID_COPY_CTOR(Socket);

    FORBID_MOVE_CTOR(Socket);
};

/**
 * Socket extension that performs POSIX poll() when waiting for an event.
 */
class RegularSocket : public Socket {
 public:
    virtual ~RegularSocket() {}

    RegularSocket() : Socket() {}

    /** @see Socket.PollOut() */
    unsigned int PollOut(int64_t dl) override;

    /** @see Socket.PollIn() */
    unsigned int PollIn(int64_t dl) override;

    /**
     * Accepts a connection and build a RegularSocket around the file descriptor.
     * @return a managed valid pointer whatever, whose fileno() returns -1 in
     * case of error, and >= 0 in case of success.
     */
    Socket* accept() override;

 private:
    FORBID_MOVE_CTOR(RegularSocket);
    FORBID_COPY_CTOR(RegularSocket);
};

/**
 * Socket extension that performs coroutine based-waiting.
 * Be careful, since you use such objects, you cannot use threads anymore in
 * your application.
 */
class MillSocket : public Socket {
 public:
    virtual ~MillSocket() {}

    MillSocket() : Socket() {}

    /** @see Socket.PollOut() */
    unsigned int PollOut(int64_t dl) override;

    /** @see Socket.PollIn() */
    unsigned int PollIn(int64_t dl) override;

    /**
     * Accepts a connection and build a MillSocket around the file descriptor.
     * @return a managed valid pointer whatever, whose fileno() returns -1 in
     * case of error, and >= 0 in case of success.
     */
    Socket* accept() override;

    /**
     * Unregister the socket from the libMill pool, then forward the close() to
     * the parent object.
     */
    void close() override;

    /**
     * Yield another coroutine.
     */
    void switch_context() override;

 private:
    FORBID_MOVE_CTOR(MillSocket);
    FORBID_COPY_CTOR(MillSocket);
};

};  // namespace net

#endif  // SRC_UTILS_NET_HPP_
