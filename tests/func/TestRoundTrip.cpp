/**
 * This file is part of the test tools for the OpenIO client libraries
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


#include <cassert>
#include "gtest/gtest.h"
#include "gflags/gflags.h"
#include "oio/sds/sds.h"


static void _load_env(std::string *dst, const char *key) {
    assert(dst != nullptr);
    const char *v = ::getenv(key);
    if (v) {
        dst->assign(v);
    } else {
        LOG(INFO) << "Missing environment key " << key;
    }
}

int main(int argc, char **argv) {
    google::InitGoogleLogging(argv[0]);
    FLAGS_logtostderr = true;

    if (argc != 2) {
        LOG(INFO) << "Usage: " << argv[0] << " fileName";
        return 1;
    }

    std::string ns, account, container, filepath, filename, type;

    _load_env(&ns, "OIO_NS");
    _load_env(&account, "OIO_ACCOUNT");
    _load_env(&container, "OIO_CONTAINER");
    type = "";

    filepath.assign(argv[1]);
    filename = filepath.substr(filepath.find_last_of("/\\") + 1);
    std::string::size_type const p(filename.find_last_of('.'));
    std::string file_without_extension = filename.substr(0, p);

    oio_sds oiosds(ns, account, container, type, file_without_extension);
    oiosds.upload(filepath, true /*autocreate*/);

    std::string tmpfilename = "/tmp/" + filename;
    oiosds.download(tmpfilename);

    return 0;
}
