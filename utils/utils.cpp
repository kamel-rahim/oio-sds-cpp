/** Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#include <random>
#include <netinet/in.h>
#include <openssl/hmac.h>
#include <openssl/sha.h>

#include "utils.h"

std::vector<uint8_t>
compute_sha1(const std::vector<uint8_t> &val) noexcept {
    unsigned int len = SHA_DIGEST_LENGTH;
    std::vector<uint8_t> result(len);

    SHA1(val.data(), val.size(), result.data());
    return result;
}

std::vector<uint8_t>
compute_sha1_hmac(const std::string &key, const std::string &val) noexcept {
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

std::random_device rand_dev;

void append_string_random(std::string &dst, unsigned int len,
                          const std::string &chars) noexcept {
    static std::random_device rand_dev;
    static std::default_random_engine prng(rand_dev());

    std::uniform_int_distribution<int> uniform_dist(0, chars.size() - 1);
    while (len-- > 0)
        dst.push_back(chars[uniform_dist(prng)]);
}

std::string generate_string_random(unsigned int len,
                                   const std::string &chars) noexcept {
    std::string s;
    append_string_random(s, len, chars);
    return s;
}
