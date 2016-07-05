/** Copyright (c) 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */
#include <cassert>
#include <fcntl.h>

#include <glog/logging.h>
#include <libmill.h>

#include <oio/local/blob.h>
#include <utils/utils.h>

using oio::local::blob::DownloadBuilder;

class LocalDownload : public oio::api::blob::Download {
    friend class DownloadBuilder;

  public:
    ~LocalDownload() {
        DLOG(INFO) << __FUNCTION__;
        if (fd >= 0) {
            ::close(fd);
            fd = -1;
        }
    }

    Status Prepare() override {
        if (fd >= 0) {
            LOG(ERROR) << "Calling Prepare() on a ready object";
            return oio::api::blob::Download::Status::InternalError;
        }

        fd = ::open(path.c_str(), O_RDONLY | O_NONBLOCK);
        if (0 > fd) {
            if (errno == ENOENT)
                return oio::api::blob::Download::Status::NotFound;
            return oio::api::blob::Download::Status::InternalError;
        }

        return oio::api::blob::Download::Status::OK;
    }

    bool IsEof() override {
        return done;
    }

    int32_t Read(std::vector<uint8_t> &buf) override {
        if (buffer.size() <= 0) {
            if (!LoadBuffer())
                return 0;
        }
        buffer.swap(buf);
        buffer.resize(0);
        return buf.size();
    }

  private:
    FORBID_MOVE_CTOR(LocalDownload);
    FORBID_COPY_CTOR(LocalDownload);
    LocalDownload() : path(), buffer(), fd{-1}, done{false} {
        DLOG(INFO) << __FUNCTION__;
    }

    bool LoadBuffer() {
        assert(fd >= 0);
        // TODO make the buffer size configurable
        buffer.resize(1024*1024);
    retry:
        ssize_t rc = ::read(fd, buffer.data(), buffer.size());
        if (rc < 0) {
            if (errno == EAGAIN) {
                int evt = fdwait(fd, FDW_IN, mill_now()+1000);
                if (evt & FDW_IN)
                    goto retry;
                return false;
            }
            return false;
        }
        if (rc == 0) {
            done = true;
            return true;
        }
        buffer.resize(rc);
        return true;
    }

  private:
    std::string path;
    std::vector<uint8_t> buffer;
    int fd;
    bool done;
};

DownloadBuilder::DownloadBuilder() {
}

DownloadBuilder::~DownloadBuilder() {
}

void DownloadBuilder::Path(const std::string &p) {
    path.assign(p);
}

std::unique_ptr<oio::api::blob::Download> DownloadBuilder::Build() {
    auto dl = new LocalDownload;
    dl->path.assign(path);
    return std::unique_ptr<LocalDownload>(dl);
}