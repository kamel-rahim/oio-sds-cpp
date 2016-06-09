/** Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#ifndef OIO_KINETIC_UTILS__ADDR_H
#define OIO_KINETIC_UTILS__ADDR_H

#include <sys/types.h>

struct NetAddr {
    int32_t family_;
    uint8_t addr[16];
    uint16_t port;
    uint8_t padding[2];
};

struct Addr {
    NetAddr ss_;
    uint32_t len_;

    Addr () noexcept { reset(); }
    Addr (const Addr &o) noexcept; // copy
    Addr (Addr &&o) noexcept; // move

    bool parse (const char *url) noexcept;
    void reset () noexcept;
};


#endif
