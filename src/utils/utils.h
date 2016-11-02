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

#ifndef SRC_UTILS_UTILS_H_
#define SRC_UTILS_UTILS_H_

#include <vector>
#include <string>
#include <memory>
#include <cstdint>

#include "./macros.h"

#define BUFLEN_IOV0(B, L) ((void*)(B)), L  // NOLINT
#define BUFLEN_IOV(B, L)  {BUFLEN_IOV0(B, L)}

#define BUF_IOV(S)    BUFLEN_IOV((S), sizeof(S)-1)
#define STR_IOV(S)    BUFLEN_IOV((S), strlen(S))
#define STRING_IOV(S) BUFLEN_IOV((S).data(), (S).size())

#define BUF_IOV0(S)    BUFLEN_IOV0((S), sizeof(S)-1)
#define STR_IOV0(S)    BUFLEN_IOV0((S), strlen(S))
#define STRING_IOV0(S) BUFLEN_IOV0((S).data(), (S).size())

#define ON_ENUM(D, F) case D::F: return #F

class Checksum {
 public:
    virtual void Update(const uint8_t *b, size_t l) = 0;
    virtual void Update(const char *b, size_t l) = 0;
    virtual std::string Final() = 0;
};

Checksum* checksum_make_MD5();
Checksum* checksum_make_SHA1();

std::string bin2hex(const uint8_t *b, size_t l);

std::vector<uint8_t> compute_sha1(const std::vector<uint8_t> &val);

std::vector<uint8_t> compute_sha1(const void *buf, size_t len);

std::vector<uint8_t> compute_sha1_hmac(const std::string &key,
                                       const std::string &val);

/**
 * Appends 'len' characters to the string pointed by 'dst'
 * @param dst cannot be null
 * @param len how many chars must be appended
 * @param chars a sequence of possible characters
 */
void append_string_random(std::string *dst, unsigned int len,
                          const std::string &chars);

std::string generate_string_random(unsigned int len,
                                   const std::string &chars);


#endif  // SRC_UTILS_UTILS_H_
