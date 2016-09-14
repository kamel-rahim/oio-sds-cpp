/** Copyright (c) 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#ifndef OIO_KINETIC_NET_H
#define OIO_KINETIC_NET_H

#include <cstdint>
#include <string>
#include <memory>

#include "utils.h"

namespace net {

#define MILLSOCKET_EVENT 1U
#define MILLSOCKET_ERROR 2U

extern bool default_reuse_port;

/**
 * Large enough for a IPv6 and IPv4 addresses
 */
struct NetAddr {
    int32_t family_;
    uint16_t port;
    uint8_t addr[16];
    uint8_t padding[2];

    NetAddr();
    FORBID_COPY_CTOR(NetAddr);
    FORBID_MOVE_CTOR(NetAddr);
};

/**
 * Gathers all the fields necessary as a parameter for bind() and connect()
 */
struct Addr {
    NetAddr ss_;
    uint32_t len_;

    Addr(); // default constructor
    Addr(const Addr &o); // copy
    Addr(Addr &&o); // move

    bool parse(const char *url);

    bool parse(const std::string u);

    void reset();

    std::string Debug() const;
};

/**
 * An interface for two-way communication channels.
 */
class Channel {
  public:
    virtual ~Channel() {}
    virtual ssize_t read (uint8_t *buf, size_t len, int64_t dl) = 0;

    /* Reads exactly max bytes from the current Socket */
    virtual bool read_exactly(uint8_t *buf, size_t len, int64_t dl) = 0;

    virtual bool send (struct iovec *iov, unsigned int count, int64_t dl) = 0;

    virtual bool send (const uint8_t *buf, size_t len, int64_t dl) = 0;

    virtual bool send (const char *str, size_t len, int64_t dl) {
        return this->send(reinterpret_cast<const uint8_t *>(str), len, dl);
    }
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

    virtual std::unique_ptr<Socket> accept() = 0;

    int accept_fd(Addr &peer, Addr &local);

    inline int fileno() const { return fd_; }

    void reset();

    void init(int family);

    virtual void close();

    bool connect(const std::string s);

    bool connect(const char *u);

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

    std::string Debug() const;

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
    virtual ssize_t read (uint8_t *buf, size_t len, int64_t dl) override;

    /* Reads exactly max bytes from the current Socket */
    virtual bool read_exactly(uint8_t *buf, const size_t len, int64_t dl) override;

	virtual bool send (struct iovec *iov, unsigned int count, int64_t dl) override;

    virtual bool send (const uint8_t *buf, size_t len, int64_t dl) override;

	virtual bool send (const char *str, size_t len, int64_t dl) {
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
    RegularSocket(): Socket() {}

    /** @see Socket.PollOut() */
    unsigned int PollOut(int64_t dl) override;

    /** @see Socket.PollIn() */
    unsigned int PollIn(int64_t dl) override;

    /**
     * Accepts a connection and build a RegularSocket around the file descriptor.
     * @return a managed valid pointer whatever, whose fileno() returns -1 in
     * case of error, and >= 0 in case of success.
     */
    virtual std::unique_ptr<Socket> accept() override;

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
    MillSocket(): Socket() {}

    /** @see Socket.PollOut() */
    virtual unsigned int PollOut(int64_t dl) override;

    /** @see Socket.PollIn() */
    virtual unsigned int PollIn(int64_t dl) override;

    /**
     * Accepts a connection and build a MillSocket around the file descriptor.
     * @return a managed valid pointer whatever, whose fileno() returns -1 in
     * case of error, and >= 0 in case of success.
     */
    virtual std::unique_ptr<Socket> accept() override;

	/**
	 * Unregister the socket from the libMill pool, then forward the close() to
	 * the parent object.
	 */
	virtual void close() override;

  private:
    FORBID_MOVE_CTOR(MillSocket);
    FORBID_COPY_CTOR(MillSocket);
};

}; // namespace net

#endif //OIO_KINETIC_NET_H