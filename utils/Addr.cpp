/** Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#include <string>
#include <cassert>
#include <cstring>

#include <arpa/inet.h>

#include "Addr.h"

static bool
_port_parse (const char *start, uint16_t *res) noexcept {
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

Addr::Addr (const Addr &o) noexcept: len_{o.len_} {
    ::memcpy(&ss_, &o.ss_, sizeof(ss_));
}

Addr::Addr (Addr &&o) noexcept: len_{o.len_} {
    ::memcpy(&ss_, &o.ss_, sizeof(ss_));
}

void
Addr::reset() noexcept {
    ::memset(&ss_, 0, sizeof(ss_));
    len_ = sizeof(ss_);
}

bool
Addr::parse (const char *url) noexcept {
    assert (url != NULL);
    assert (*url != '\0');

    std::string addr(url);

    auto colon = addr.rfind(':');
    if (colon == std::string::npos) {
        errno = EINVAL;
        return false;
    }

    std::string sa = addr.substr(0, colon);
    std::string sp = addr.substr(colon+1);

    uint16_t u16port = 0;
    if (!_port_parse (sp.c_str(), &u16port)) {
        return false;
    }

    if (sa[0] == '[') {
        struct sockaddr_in6 *sin6 = reinterpret_cast<struct sockaddr_in6*>(&ss_);
        len_ = sizeof (*sin6);
        if (sa.back() == ']')
            sa.pop_back();
        if (0 < ::inet_pton (AF_INET6, sa.c_str()+1, &sin6->sin6_addr)) {
            sin6->sin6_family = AF_INET6;
            sin6->sin6_port = htons(u16port);
            return true;
        }
    } else {
        struct sockaddr_in *sin = reinterpret_cast<struct sockaddr_in*>(&ss_);
        len_ = sizeof (*sin);
        if (0 < ::inet_pton (AF_INET, sa.c_str(), &sin->sin_addr)) {
            sin->sin_family = AF_INET;
            sin->sin_port = htons(u16port);
            return true;
        }
    }

    errno = EINVAL;
    return false;
}
