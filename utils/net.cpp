/** Copyright (c) 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#include <string>
#include <sstream>
#include <iostream>

#include <cassert>
#include <cstring>

#include <unistd.h>
#include <poll.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>

#include <glog/logging.h>
#include <libmill.h>

#include "utils.h"
#include "net.h"

#define ADDR(P)  reinterpret_cast<const struct sockaddr*>(P)
#define ADDR4(P) reinterpret_cast<const struct sockaddr_in*>(P)
#define ADDR6(P) reinterpret_cast<const struct sockaddr_in6*>(P)


bool net::default_reuse_port = true;

using net::Socket;
using net::Addr;
using net::NetAddr;
using net::RegularSocket;
using net::MillSocket;

static bool _port_parse(const char *start, uint16_t *res) {
    char *_end{nullptr};
    uint64_t u64port = ::strtoull(start, &_end, 10);

    if (!u64port && errno == EINVAL) {
        errno = EINVAL;
        return false;
    }
    if (u64port >= 65536) {
        errno = ERANGE;
        return false;
    }
    if (_end && *_end) {
        errno = EINVAL;
        return false;
    }

    *res = static_cast<uint16_t>(u64port);
    return true;
}

//-----------------------------------------------------------------------------

NetAddr::NetAddr() : family_{0} {
}

Addr::Addr() {
    reset();
}

Addr::Addr(const Addr &o) : len_{o.len_} {
    ::memcpy(&ss_, &o.ss_, sizeof(ss_));
}

Addr::Addr(Addr &&o) : len_{o.len_} {
    ::memcpy(&ss_, &o.ss_, sizeof(ss_));
}

void Addr::reset() {
    ::memset(&ss_, 0, sizeof(ss_));
    len_ = sizeof(ss_);
}

bool Addr::parse(const char *url) {
    assert(url != nullptr);
    return parse(std::string(url));
}

bool Addr::parse(const std::string addr) {
    assert (!addr.empty());

    auto colon = addr.rfind(':');
    if (colon == std::string::npos) {
        errno = EINVAL;
        return false;
    }

    std::string sa = addr.substr(0, colon);
    std::string sp = addr.substr(colon + 1);

    uint16_t u16port = 0;
    if (!_port_parse(sp.c_str(), &u16port)) {
        return false;
    }

    if (sa[0] == '[') {
        struct sockaddr_in6 *sin6 = reinterpret_cast<struct sockaddr_in6 *>(&ss_);
        len_ = sizeof(*sin6);
        if (sa.back() == ']')
            sa.pop_back();
        if (0 < ::inet_pton(AF_INET6, sa.c_str() + 1, &sin6->sin6_addr)) {
            sin6->sin6_family = AF_INET6;
            sin6->sin6_port = htons(u16port);
            return true;
        }
    } else {
        struct sockaddr_in *sin = reinterpret_cast<struct sockaddr_in *>(&ss_);
        len_ = sizeof(*sin);
        if (0 < ::inet_pton(AF_INET, sa.c_str(), &sin->sin_addr)) {
            sin->sin_family = AF_INET;
            sin->sin_port = htons(u16port);
            return true;
        }
    }

    errno = EINVAL;
    return false;
}

std::string Addr::Debug() const {
    char tmp[128];
    std::stringstream ss;
    switch (ADDR(&ss_)->sa_family) {
        case 0:
            return ss.str();
        case AF_INET:
            inet_ntop(AF_INET, &ADDR4(&ss_)->sin_addr, tmp, sizeof(tmp));
            ss << tmp << ':'
               << ntohs(ADDR4(&ss_)->sin_port);
            return ss.str();
        case AF_INET6:
            inet_ntop(AF_INET6, &ADDR6(&ss_)->sin6_addr, tmp, sizeof(tmp));
            ss << '[' << tmp << ']' << ':'
               << ntohs(ADDR6(&ss_)->sin6_port);
            return ss.str();
        default:
            return std::string("?");
    }
}

std::string Socket::Debug() const {
    std::stringstream ss;
    ss << "Socket{fd:" << fd_
       << ",local:" << local_.Debug()
       << ",remote:" << peer_.Debug()
       << "}";
    return ss.str();
}

void Socket::reset() {
    fd_ = -1;
    peer_.reset();
    local_.reset();
}

void Socket::close() {
    if (fd_ >= 0)
        ::close(fd_);
    this->reset();
}

void Socket::init(int family) {
    assert (fd_ < 0);

    fd_ = ::socket(family, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    assert (fd_ >= 0);

    setopt(SOL_SOCKET, SO_REUSEADDR, 1);
#ifdef SO_NOSIGPIPE
    setopt (SOL_SOCKET, SO_NOSIGPIPE, 1);
#endif
}

bool Socket::setopt(int dom, int opt, int val) {
    int rc = ::setsockopt(fd_, dom, opt, (void *) &val, sizeof(val));
    return (rc == 0);
}

bool Socket::setnodelay() {
    return setopt(IPPROTO_TCP, TCP_NODELAY, 1);
}

bool Socket::setcork() {
    return setopt(IPPROTO_TCP, TCP_CORK, 1);
}

bool Socket::setquickack() {
    return setopt(IPPROTO_TCP, TCP_QUICKACK, 1);
}

bool Socket::setsndbuf(int size) {
    return setopt(SOL_SOCKET, SO_SNDBUF, size);
}

bool Socket::setrcvbuf(int size) {
    return setopt(SOL_SOCKET, SO_RCVBUF, size);
}

bool Socket::listen(int backlog) {
    assert(fd_ >= 0);
    auto rc = ::listen(fd_, backlog);
    return rc == 0;
}

bool Socket::bind(const char *url) {
    assert(fd_ < 0);
    if (!local_.parse(url))
        return false;
    this->init(reinterpret_cast<struct sockaddr *>(&local_.ss_)->sa_family);
    auto rc = ::bind(fd_,
                     reinterpret_cast<struct sockaddr *>(&local_.ss_),
                     reinterpret_cast<socklen_t>(local_.len_));
    if (rc < 0)
        return false;

#ifdef SO_REUSEPORT
    if (default_reuse_port)
        setopt(SOL_SOCKET, SO_REUSEPORT, 1);
#endif
    return true;
}

bool Socket::connect(const char *u) {
    assert(u != nullptr);
    return this->connect(std::string(u));
}

bool Socket::connect(const std::string url) {
    assert(fd_ < 0);
    assert(!url.empty());
    if (!peer_.parse(url)) {
        return false;
    }
    this->init(reinterpret_cast<struct sockaddr *>(&peer_.ss_)->sa_family);

    auto rc = ::connect(fd_,
                        reinterpret_cast<struct sockaddr *>(&peer_.ss_),
                        reinterpret_cast<socklen_t>(peer_.len_));
    if (rc < 0 && errno != EINPROGRESS)
        return false;

    (void) ::getsockname(fd_,
                         reinterpret_cast<struct sockaddr *>(&local_.ss_),
                         reinterpret_cast<socklen_t *>(&local_.len_));
    return true;
}

int Socket::accept_fd(Addr &peer, Addr &local) {
    assert(fd_ >= 0);
    int fd = ::accept4(fd_,
                        reinterpret_cast<struct sockaddr *>(&peer),
                        reinterpret_cast<socklen_t *>(&peer),
                        SOCK_NONBLOCK | SOCK_CLOEXEC);
    if (fd < 0)
        return -1;

    (void) ::getsockname(fd_,
                         reinterpret_cast<struct sockaddr *>(&local),
                         reinterpret_cast<socklen_t *>(&local));
    return fd;
}

ssize_t Socket::read (uint8_t *buf, size_t len, int64_t dl) {
    assert(dl > 0);
    for (;;) {
        ssize_t rc = ::read(fd_, buf, len);
        if (rc > 0)
            return rc;
        if (rc == 0)
            return -2;
        if (errno == EINTR)
            continue;
        if (errno != EAGAIN)
            return -1;

        if (dl < mill_now()) {
            errno = ETIMEDOUT;
            return -1;
        }

        auto evt = PollIn(dl);
        if (evt & MILLSOCKET_ERROR)
            return -1;
        if (!(evt & MILLSOCKET_EVENT)) {
            errno = ETIMEDOUT;
            return -1;
        }
    }
}

bool Socket::read_exactly(uint8_t *buf, const size_t len0, int64_t dl) {
    assert(dl > 0);
    size_t len{len0};
    while (len > 0) {
        auto rc = ::read(fd_, buf, len);
        if (rc > 0) {
            len -= rc;
            buf += rc;
        }
        else if (rc == 0) {
            errno = ECONNRESET;
            return false;
        }
        else {
            if (errno == EINTR) {
				continue;
			}
            if (errno == EAGAIN) {
                if (dl < mill_now()) {
                    errno = (len == len0) ? EAGAIN : ETIMEDOUT;
                    return false;
                }
                auto evt = PollIn(dl);
				if (evt & MILLSOCKET_ERROR) {
                    // let read() set errno
                    continue;
                }
				if (!(evt & MILLSOCKET_EVENT)) {
                    errno = (len == len0) ? EAGAIN : ETIMEDOUT;
					return false;
                }
			} else {
                // errno set by read()
				return false;
			}
        }
    }
    return true;
}

bool Socket::send (struct iovec *iov, unsigned int count, int64_t dl) {

    assert(dl > 0);

    size_t total = 0, sent = 0;
    for (unsigned int i=0; i<count ;++i)
        total += iov[i].iov_len;
    ssize_t rc;
    while (sent < total) {
        rc = ::writev(fd_, iov, count);
        if (rc < 0) {
            if (errno == EINTR)
                continue;
            else if (errno == EAGAIN) {
                if (dl < mill_now()) {
                    errno = ETIMEDOUT;
                    return false;
                }
				auto evt = PollOut(dl);
				if (evt & MILLSOCKET_ERROR) {
                    /* let writev() set the errno */
                    continue;
                }
				if (!(evt & MILLSOCKET_EVENT)) {
                    errno = ETIMEDOUT;
                    return false;
                }
			} else {
                // errno already set by writev()
				return false;
			}
        }
        if (rc > 0) {
            sent += rc;
            // advance in the iov
            while (rc > 0) {
                assert (count > 0);
                if (static_cast<size_t>(rc) > iov[0].iov_len) {
                    rc -= iov[0].iov_len;
                    iov++;
                    count--;
                } else {
                    iov[0].iov_base = static_cast<uint8_t*>(iov[0].iov_base) + rc;
                    iov[0].iov_len -= rc;
                    rc = 0;
                }
            }
        }
    }
    return true;
}

bool Socket::send (const uint8_t *buf, size_t len, int64_t dl) {
    assert(dl > 0);

    while (len) {
        ssize_t rc = ::write(fd_, buf, len);
        if (rc > 0) {
            len -= rc, buf += rc;
        } else if (rc == 0) {
            /* nothing written */
        } else {
            if (errno == EINTR)
                continue;
            else if (errno == EAGAIN) {
                if (dl < mill_now()) {
                    errno = ETIMEDOUT;
                    return false;
                }
                auto evt = PollOut(dl);
				if (evt & MILLSOCKET_ERROR) {
                    // let writev() set errno
                    continue;
                }
				if (!(evt & MILLSOCKET_EVENT)) {
                    errno = ETIMEDOUT;
                    return false;
                }
			} else {
                // errno set by write()
				return false;
			}
        }
    }
    return true;
}


static unsigned int _poll(int fd, short int evt, int64_t dl UNUSED) {
    struct pollfd pfd{fd, evt, 0};
    auto rc = ::poll(&pfd, 1, 1000);
    if (rc < 0)
        return MILLSOCKET_ERROR;
    return ((pfd.revents & (evt|POLLHUP)) ? MILLSOCKET_EVENT : 0U)
           | (pfd.revents & POLLERR ? MILLSOCKET_ERROR : 0U);
}

unsigned int RegularSocket::PollIn(int64_t dl) {
    return _poll(fd_, POLLIN, dl);
}

unsigned int RegularSocket::PollOut(int64_t dl) {
    return _poll(fd_, POLLOUT, dl);
}

std::unique_ptr<Socket> RegularSocket::accept() {
    auto cli = new RegularSocket();
    cli->fd_ = accept_fd(cli->local_, cli->peer_);
    return std::unique_ptr<Socket>(cli);
}


static unsigned int _fdwait (int fd, unsigned int evt, int64_t dl) {
    unsigned int revents = fdwait(fd, evt, dl);
    return ((revents & evt) ? MILLSOCKET_EVENT : 0U)
           | ((revents & FDW_ERR) ? MILLSOCKET_ERROR : 0U);
}

unsigned int MillSocket::PollIn(int64_t dl) {
    return _fdwait(fd_, FDW_IN, dl);
}

unsigned int MillSocket::PollOut(int64_t dl) {
    return _fdwait(fd_, FDW_OUT, dl);
}

void MillSocket::close() {
	if (fd_ >= 0) {
		::fdclean(fd_);
		::close(fd_);
		this->reset();
	}
}

std::unique_ptr<Socket> MillSocket::accept() {
    auto cli = new MillSocket();
    cli->fd_ = accept_fd(cli->local_, cli->peer_);
    return std::unique_ptr<Socket>(cli);
}