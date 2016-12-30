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

#include "utils/utils.hpp"

#include <netinet/in.h>

#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <openssl/md5.h>

#include <random>
#include <iomanip>

static std::random_device rand_dev;

class ChecksumMD5 : public Checksum {
 public:
    ChecksumMD5() { MD5_Init(&ctx); }

    virtual ~ChecksumMD5() {}

    void Update(const uint8_t *b, size_t l) override { MD5_Update(&ctx, b, l); }

    void Update(const char *b, size_t l) override { MD5_Update(&ctx, b, l); }

    std::string Final() override {
        std::array<uint8_t, MD5_DIGEST_LENGTH> tmp;
        MD5_Final(tmp.data(), &ctx);
        return bin2hex(tmp.data(), tmp.size());
    }

 private:
    MD5_CTX ctx;
};

class ChecksumSHA1 : public Checksum {
 public:
    ChecksumSHA1() { SHA1_Init(&ctx); }

    virtual ~ChecksumSHA1() {}

    void Update(const uint8_t *b, size_t l) override {
        SHA1_Update(&ctx, b, l);
    }

    void Update(const char *b, size_t l) override { SHA1_Update(&ctx, b, l); }

    std::string Final() override {
        std::array<uint8_t, SHA_DIGEST_LENGTH> tmp;
        SHA1_Final(tmp.data(), &ctx);
        return bin2hex(tmp.data(), tmp.size());
    }

 private:
    SHA_CTX ctx;
};

Checksum *checksum_make_MD5() { return new ChecksumMD5; }

Checksum *checksum_make_SHA1() { return new ChecksumSHA1; }

std::string bin2hex(const uint8_t *b, size_t l) {
    std::stringstream ss;
    for (; l > 0; --l, ++b) {
        ss << std::hex << std::setw(2) << std::setfill('0')
           << (unsigned int) (*b);
    }
    return ss.str();
}

std::vector<uint8_t> compute_sha1(const std::vector<uint8_t> &val) {
    return compute_sha1(val.data(), val.size());
}

std::vector<uint8_t> compute_sha1(const void *buf, size_t buflen) {
    unsigned int len = SHA_DIGEST_LENGTH;
    std::vector<uint8_t> result(len);

    SHA1(static_cast<const unsigned char *>(buf), buflen, result.data());
    return result;
}

std::vector<uint8_t>
compute_sha1_hmac(const std::string &key, const std::string &val) {
    HMAC_CTX ctx;
    HMAC_CTX_init(&ctx);
    HMAC_Init_ex(&ctx, key.c_str(), key.length(), EVP_sha1(), NULL);

    if (val.length() > 0) {
        uint32_t be = ::htonl(val.length());
        HMAC_Update(&ctx, reinterpret_cast<unsigned char *>(&be),
                    sizeof(uint32_t));
        HMAC_Update(&ctx, reinterpret_cast<const unsigned char *>(val.c_str()),
                    val.length());
    }

    unsigned int len = SHA_DIGEST_LENGTH;
    std::vector<uint8_t> result(len);

    HMAC_Final(&ctx, result.data(), &len);
    HMAC_CTX_cleanup(&ctx);

    return result;
}

void append_string_random(std::string *dst, unsigned int len,
        const std::string &chars) {
    static std::random_device rand_dev;
    static std::default_random_engine prng(rand_dev());

    std::uniform_int_distribution<int> uniform_dist(0, chars.size() - 1);
    while (len-- > 0)
        dst->push_back(chars[uniform_dist(prng)]);
}

std::string generate_string_random(unsigned int len,
        const std::string &chars) {
    std::string s;
    append_string_random(&s, len, chars);
    return s;
}
