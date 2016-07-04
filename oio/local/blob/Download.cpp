/** Copyright (c) 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#include <sys/stat.h>
#include <glog/logging.h>
#include <oio/local/blob.h>
#include <utils/utils.h>
#include <fcntl.h>

using oio::local::blob::DownloadBuilder;

class Download : public oio::api::blob::Download {
    friend class DownloadBuilder;

  public:
    ~Download() {}

    Status Prepare() override {
        if (fd >= 0) {
            LOG(ERROR) << "Calling Prepare() on a ready object";
            return oio::api::blob::Download::Status::InternalError;
        }

        fd = ::open(path.c_str(), O_RDONLY);
        if (0 > fd) {
            if (errno == ENOENT)
                return oio::api::blob::Download::Status::NotFound;
            return oio::api::blob::Download::Status::InternalError;
        }

        return oio::api::blob::Download::Status::OK;
    }

    bool IsEof() override {
        return true;
    }

    int32_t Read(std::vector<uint8_t> &buf) override {
        (void) buf;
        return -1;
    }

  private:
    FORBID_MOVE_CTOR(Download);
    FORBID_COPY_CTOR(Download);
    Download() : path(), fd{-1} {}

  private:
    std::string path;
    int fd;
};

DownloadBuilder::DownloadBuilder() {
}

DownloadBuilder::~DownloadBuilder() {
}

void DownloadBuilder::Path(const std::string &p) {
    path.assign(p);
}

std::unique_ptr<oio::api::blob::Download> DownloadBuilder::Build() {
    auto dl = new Download;
    dl->path.assign(path);
    return std::unique_ptr<Download>(dl);
}