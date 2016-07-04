/** Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#include <string>
#include <sstream>
#include <iostream>

#include <unistd.h>
#include <assert.h>
#include <sys/socket.h>

#include "Socket.h"

bool default_reuse_port = true;
int default_backlog = 8192;

std::string Socket::debug_string () const {
    std::stringstream ss;
    ss << "Socket{fd:" << fd_ << '}';
    return ss.str();
}

void Socket::reset() {
	peer_.reset();
	local_.reset();
	fd_ = -1;
}

void Socket::close() {
	if (fd_ >= 0)
		::close(fd_);
	this->reset();
}

void Socket::init (int family) {
    assert (fd_ < 0);

    fd_ = ::socket (family, SOCK_STREAM|SOCK_NONBLOCK|SOCK_CLOEXEC, 0);
    assert (fd_ >= 0);

    setopt (SOL_SOCKET, SO_REUSEADDR, 1);
#ifdef SO_NOSIGPIPE
    setopt (SOL_SOCKET, SO_NOSIGPIPE, 1);
#endif
}

bool Socket::setopt (int dom, int opt, int val) {
	int rc = ::setsockopt (fd_, dom, opt, (void*)&val, sizeof (val));
	return (rc == 0);
}

bool Socket::listen(int backlog) {
    assert(fd_ >= 0);
    auto rc = ::listen(fd_, backlog);
    return rc == 0;
}

bool Socket::bind (const char *url) {
    assert(fd_ < 0);
    if (!local_.parse(url))
        return false;
    this->init (reinterpret_cast<struct sockaddr*>(&local_.ss_)->sa_family);
    auto rc = ::bind (fd_,
                      reinterpret_cast<struct sockaddr*>(&local_.ss_),
                      reinterpret_cast<socklen_t>(local_.len_));
	if (rc < 0)
        return false;

#ifdef SO_REUSEPORT
    if (default_reuse_port)
	    setopt (SOL_SOCKET, SO_REUSEPORT, 1);
#endif
	return true;
}

bool Socket::connect (const char *url) {
    assert(fd_ < 0);
    if (!peer_.parse(url)) {
        return false;
    }
    this->init (reinterpret_cast<struct sockaddr*>(&peer_.ss_)->sa_family);

    auto rc = ::connect (fd_,
                         reinterpret_cast<struct sockaddr*>(&peer_.ss_),
                         reinterpret_cast<socklen_t>(peer_.len_));
	if (rc < 0 && errno != EINPROGRESS)
        return false;

    (void) ::getsockname(fd_,
                         reinterpret_cast<struct sockaddr*>(&local_.ss_),
                         reinterpret_cast<socklen_t*>(&local_.len_));
    return true;
}

bool Socket::accept(Socket &cli) {
    assert(fd_ >= 0);
    assert(cli.fd_ < 0);
    cli.fd_ = ::accept4(fd_,
                        reinterpret_cast<struct sockaddr*>(&cli.peer_.ss_),
                        reinterpret_cast<socklen_t *>(&cli.peer_.len_),
                        SOCK_NONBLOCK|SOCK_CLOEXEC);
    if (cli.fd_ < 0)
        return false;

    (void) ::getsockname(cli.fd_,
                       reinterpret_cast<struct sockaddr*>(&cli.local_.ss_),
                         reinterpret_cast<socklen_t*>(&cli.local_.len_));
    return true;
}