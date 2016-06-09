/** Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#include <cassert>
#include <sstream>
#include <unistd.h>
#include <sys/uio.h>

#include <libmill.h>

#include "MillSocket.h"
#include "utils.h"

std::string MillSocket::debug_string () const noexcept {
    std::stringstream ss;
    ss << "Mill{" << sock_.debug_string() << "}";
    return ss.str();
}

ssize_t MillSocket::read (uint8_t *buf, size_t len, int64_t dl) noexcept {
    int64_t real_dl = dl>0 ? dl : (mill_now()+1000);
    ssize_t rc;

    retry:
    rc = ::read(sock_.fileno(), buf, len);
    if (rc > 0)
        return rc;
    if (rc == 0)
        return -2;

    if (errno == EINTR)
        goto retry;

    if (errno != EAGAIN)
        return -1;

    auto evt = fdwait(sock_.fileno(), FDW_IN, real_dl);
    if (evt & FDW_ERR)
        return -1;
    if (evt & FDW_IN)
        goto retry;

    return 0;
}

bool MillSocket::read_exactly(uint8_t *buf, size_t len, int64_t dl) noexcept {
    while (len > 0) {
        auto rc = ::read(sock_.fileno(), buf, len);
        if (rc < 0) {
            if (errno == EINTR)
                continue;
            if (errno == EAGAIN) {
                int64_t real_dl = dl>0 ? dl : (mill_now()+1000);
                auto evt = fdwait(sock_.fileno(), FDW_IN, real_dl);
                if (evt & FDW_ERR)
                    return false;
                if (!evt)
                    return false;
                continue;
            }
            return false;
        }
        if (!rc)
            return false;
        len -= rc;
    }
    return true;
}

bool MillSocket::send (struct iovec *iov, unsigned int count, int64_t dl) noexcept {
    size_t total = 0, sent = 0;
    for (unsigned int i=0; i<count ;++i)
        total += iov[i].iov_len;
    ssize_t rc;
    while (sent < total) {
        rc = ::writev(sock_.fileno(), iov, count);
        if (rc < 0) {
            if (errno == EINTR)
                continue;
            if (errno == EAGAIN) {
                const auto real_dl = dl>0 ? dl : mill_now()+1000;
                auto evt = fdwait(sock_.fileno(), FDW_OUT, real_dl);
                if (evt & FDW_ERR)
                    return false;
                continue;
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

bool MillSocket::send (uint8_t *buf, size_t len, int64_t dl) noexcept {
    struct iovec iov = {.iov_base=buf, .iov_len=len};
    return send (&iov, 1, dl);
}

void MillSocket::close () noexcept {
    auto fd = sock_.fileno();
    if (fd > 0) {
        ::fdclean(fd);
        sock_.close();
    }
}
