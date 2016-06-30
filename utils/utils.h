/** Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#ifndef OIO_KINETIC_UTILS_H
#define OIO_KINETIC_UTILS_H

#include <vector>
#include <string>
#include <cstdint>

#define FORBID_DEFAULT_CTOR(T) T() = delete
#define FORBID_COPY_CTOR(T) T(T &o) = delete; T(const T &o) = delete
#define FORBID_MOVE_CTOR(T) T(T &&o) = delete

#define FORBID_ALL_CTOR(T) \
    FORBID_DEFAULT_CTOR(T); \
    FORBID_COPY_CTOR(T); \
    FORBID_MOVE_CTOR(T)


#define BUFLEN_IOV(B, L) {.iov_base=((void*)(B)),.iov_len=L}

#define BUF_IOV(S) BUFLEN_IOV((S),sizeof(S)-1)
#define STR_IOV(S) BUFLEN_IOV((S),strlen(S))
#define STRING_IOV(S) BUFLEN_IOV((S).data(),(S).size())

#define ON_ENUM(D, F) case D::F: return #F

#if defined __GNUC__ || defined __clang__
#define UNUSED __attribute__ ((unused))
#define NOINLINE __attribute__((noinline))
#else
#error "Unsupported compiler!"
#endif

std::vector<uint8_t> compute_sha1(const std::vector<uint8_t> &val) noexcept;

std::vector<uint8_t> compute_sha1_hmac(const std::string &key,
                                       const std::string &val) noexcept;

void append_string_random(std::string &dst, unsigned int len,
                          const std::string &chars) noexcept;

std::string generate_string_random(unsigned int len,
                                   const std::string &chars) noexcept;

#endif //OIO_KINETIC_UTILS_H
