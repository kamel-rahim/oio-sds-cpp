/** Copyright (c) 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#include <sys/stat.h>
#include <unistd.h>
#include <oio/local/blob.h>

using oio::local::blob::RemovalBuilder;

class LocalRemoval : public oio::api::blob::Removal {
    friend class RemovalBuilder;

  public:
    LocalRemoval() {
    }

    ~LocalRemoval() {
    }

    Status Prepare() override {
        struct stat st;
        if (0 == ::stat(path.c_str(), &st))
            return oio::api::blob::Removal::Status::OK;
        if (errno == ENOENT)
            return oio::api::blob::Removal::Status::NotFound;
        return oio::api::blob::Removal::Status::InternalError;
    }

    bool Commit() override {
        return 0 == ::unlink(path.c_str());
    }

    bool Abort() override {
        return true;
    }

  private:
    std::string path;
    int fd;
};

RemovalBuilder::RemovalBuilder() {
}

RemovalBuilder::~RemovalBuilder() {
}

void RemovalBuilder::Path(const std::string &p) {
    path.assign(p);
}

std::unique_ptr<oio::api::blob::Removal> RemovalBuilder::Build() {
    auto rem = new LocalRemoval;
    rem->path.assign(path);
    return std::unique_ptr<LocalRemoval>(rem);
}