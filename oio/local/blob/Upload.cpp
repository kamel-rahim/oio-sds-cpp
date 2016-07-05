/** Copyright (c) 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */
#include <cerrno>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <attr/xattr.h>

#include <glog/logging.h>

#include <oio/local/blob.h>
#include <utils/utils.h>
#include <libmill.h>

using oio::local::blob::UploadBuilder;

class LocalUpload : public oio::api::blob::Upload {
    friend class UploadBuilder;

  public:
    void SetXattr(const std::string &k, const std::string &v) override {
        if (fd < 0)
            return;
        if (0 != ::fsetxattr(fd, k.c_str(), v.data(), v.size(), 0)) {
            LOG(ERROR) << "fsetxattr(" << path_temp.c_str() << ") failed: (" <<
            errno << ") " << ::strerror(errno);
        }
    }

    Status Prepare() override {
        if (fd >= 0)
            return oio::api::blob::Upload::Status::InternalError;

        fd = ::open(path_temp.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_NONBLOCK, 0644);
        if (fd < 0) {
            if (errno == ENOENT) {
                // TODO Lazy directory creation
                return oio::api::blob::Upload::Status::InternalError;
            } else if (errno == EEXIST) {
                return oio::api::blob::Upload::Status::Already;
            } else {
                // TODO have a code for storage full or unavailable
                return oio::api::blob::Upload::Status::InternalError;
            }
        }

        struct stat st;
        int rc = ::stat(path_final.c_str(), &st);
        if (rc == 0) {
            return oio::api::blob::Upload::Status::Already;
        }

        return oio::api::blob::Upload::Status::OK;
    }

    bool Commit() override {
        if (!ResetFD())
            return false;
        if (0 == ::rename(path_temp.c_str(), path_final.c_str()))
            return true;
        LOG(ERROR) << "rename(" << path_final << ") failed: (" << errno <<
        ") " << ::strerror(errno);
        return false;
    }

    bool Abort() override {
        if (!ResetFD())
            return false;
        if (0 == ::unlink(path_temp.c_str()))
            return true;
        LOG(ERROR) << "unlink(" << path_temp << ") failed: (" << errno <<
        ") " << ::strerror(errno);
        return false;

    }

    void Write(const uint8_t *buf, uint32_t len) override {
        ssize_t rc;
        retry:
        rc = ::write(fd, buf, len);
        if (rc < 0) {
            if (errno == EINTR)
                goto retry;
            if (errno == EAGAIN) {
                int evt = fdwait(fd, FDW_OUT, mill_now()+1000);
                if (evt & FDW_OUT)
                    goto retry;
                LOG(ERROR) << "write failed, polled mentions an error";
            } else {
                LOG(ERROR) << "write(" << len << ") error: (" << errno << ") " <<
                ::strerror(errno);
            }
        } else if (rc < len){
            buf += rc;
            len -= rc;
            goto retry;
        }
    }

    ~LocalUpload() {
        DLOG(INFO) << __FUNCTION__;
    }

  private:
    FORBID_COPY_CTOR(LocalUpload);

    FORBID_MOVE_CTOR(LocalUpload);

    LocalUpload() : path_final(), path_temp(), fd{-1} {
        DLOG(INFO) << __FUNCTION__;
    }

    bool ResetFD() {
        if (fd < 0)
            return false;
        ::close(fd);
        fd = -1;
        return true;
    }

  private:
    std::string path_final;
    std::string path_temp;
    int fd;
};

UploadBuilder::UploadBuilder() {
}

UploadBuilder::~UploadBuilder() {
}

void UploadBuilder::Path(const std::string &p) {
    path.assign(p);
}

std::unique_ptr<oio::api::blob::Upload> UploadBuilder::Build() {
    auto ul = new LocalUpload();
    ul->path_final.assign(path);
    ul->path_temp.assign(path + ".pending");
    return std::unique_ptr<LocalUpload>(ul);
}
