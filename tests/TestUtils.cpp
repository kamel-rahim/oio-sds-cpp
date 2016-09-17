/** Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#include <cassert>
#include <array>

#include <glog/logging.h>
#include <utils/utils.h>

int main (int argc UNUSED, char **argv) {
    google::InitGoogleLogging(argv[0]);
    FLAGS_logtostderr = true;

    do {
        auto checksum = checksum_make_MD5();
        checksum->Update("JFS", 3);
        auto h = checksum->Final();
        assert("26d80754ef7129ffae60b3fe018ba53a" == h);
    } while (0);

    do {
        std::array<uint8_t,3> bin;
        bin.fill(0);
        assert("000000" == bin2hex(bin.data(), bin.size()));
        bin.fill(1);
        assert("010101" == bin2hex(bin.data(), bin.size()));
        bin.fill(255);
        assert("ffffff" == bin2hex(bin.data(), bin.size()));
    } while (0);

    return 0;
}