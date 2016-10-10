/** Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#ifndef OIO_KINETIC_UTILS_H
#define OIO_KINETIC_UTILS_H

#include <vector>
#include <string>
#include <memory>
#include <cstdint>

#include "macros.h"

#define BUFLEN_IOV0(B, L) ((void*)(B)),L
#define BUFLEN_IOV(B, L)  {BUFLEN_IOV0(B,L)}

#define BUF_IOV(S)    BUFLEN_IOV((S),sizeof(S)-1)
#define STR_IOV(S)    BUFLEN_IOV((S),strlen(S))
#define STRING_IOV(S) BUFLEN_IOV((S).data(),(S).size())

#define BUF_IOV0(S)    BUFLEN_IOV0((S),sizeof(S)-1)
#define STR_IOV0(S)    BUFLEN_IOV0((S),strlen(S))
#define STRING_IOV0(S) BUFLEN_IOV0((S).data(),(S).size())

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

void append_string_random(std::string &dst, unsigned int len,
                          const std::string &chars);

std::string generate_string_random(unsigned int len,
                                   const std::string &chars);

#endif //OIO_KINETIC_UTILS_H
