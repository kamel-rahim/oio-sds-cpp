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

#include "oio/blob/local/blob.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <attr/xattr.h>

#include <libmill.h>

#include <cassert>
#include <map>
#include <vector>

#include "utils/macros.h"

using oio::api::Status;
using oio::api::Errno;
using oio::api::Cause;
using oio::local::blob::DownloadBuilder;
using oio::local::blob::RemovalBuilder;
using oio::local::blob::UploadBuilder;
using Step = oio::api::blob::TransactionStep;

DEFINE_uint64(mode_mkdir, 0755, "Mode for freshly create directories");
DEFINE_uint64(mode_create, 0644, "Mode for freshly created files");
DEFINE_uint64(read_batch_size, 1024 * 1024, "Default size of the read buffer");
DEFINE_uint64(read_eintr_attempts, 5, "Number of attempts when interrupted");

/**
 *
 */
class LocalDownload : public oio::api::blob::Download {
    friend class DownloadBuilder;

 private:
    std::string path;
    std::vector<uint8_t> buffer;
    off64_t offset_, size_expected_, size_read_;
    int fd_;
    Step step;

 private:
    FORBID_MOVE_CTOR(LocalDownload);
    FORBID_COPY_CTOR(LocalDownload);

    LocalDownload() : path(), buffer(), offset_{0}, size_expected_{0},
                      size_read_{0}, fd_{-1}, step{Step::Init} {}

    unsigned int loadBufferAndRetry(int nb_attempts) {
        ssize_t rc = ::read(fd_, buffer.data(), buffer.size());
        if (rc < 0) {
            if (errno == EAGAIN) {
                int evt = fdwait(fd_, FDW_IN, mill_now() + 1000);
                if (evt & FDW_IN) {
                    if (nb_attempts > 0)
                        return loadBufferAndRetry(nb_attempts - 1);
                    return 0;
                }
                return 0;
            }
            return 0;
        } else {
            if (rc == 0)
                step = Step::Done;
            else
                size_read_ += rc;
            return rc;
        }
    }

    bool loadBuffer() {
        assert(fd_ >= 0);
        assert(step == Step::Prepared);

        // Bound the buffer size to both the expected size and the general
        // batch size.
        buffer.resize(FLAGS_read_batch_size);
        if (size_expected_ > 0) {
            const auto remaining = size_expected_ - size_read_;
            if (remaining > 0 &&
                static_cast<uint64_t>(remaining) < FLAGS_read_batch_size)
                buffer.resize(remaining);
        }

        auto rc = loadBufferAndRetry(FLAGS_read_eintr_attempts);
        if (rc > 0)
            buffer.resize(rc);
        return rc > 0;
    }

    Status closeAndErrno() { return closeAndErrno(errno); }

    Status closeAndErrno(int err) {
        Errno result(err);
        ::close(fd_);
        fd_ = -1;
        return result;
    }

 public:
    ~LocalDownload() {
        if (fd_ >= 0) {
            ::close(fd_);
            fd_ = -1;
        }
    }

    Status Prepare() override {
        if (fd_ >= 0 || step != Step::Init)
            return Status(Cause::InternalError);

        fd_ = ::open(path.c_str(), O_RDONLY | O_NONBLOCK);
        if (0 > fd_) {
            if (errno == ENOENT)
                return Status(Cause::NotFound);
            return Status(Cause::InternalError);
        }

        // Seek at the expected range
        if (offset_ > 0) {
            int rc = ::lseek64(fd_, offset_, SEEK_SET);
            if (rc != 0)
                closeAndErrno();
        }

        // If a particular size has been configured, then check it is available.
        // If the offset is too big, the previous check should already have
        // reported it.
        if (size_expected_ > 0) {
            struct stat64 st;
            int rc = ::fstat64(fd_, &st);
            if (rc != 0)
                return closeAndErrno();
            if (st.st_size < size_expected_ + offset_)
                return closeAndErrno(ENXIO);
        }

        step = Step::Prepared;
        return Status(Cause::OK);
    }

    bool IsEof() override { return step == Step::Done; }

    int32_t Read(std::vector<uint8_t> *buf) override {
        assert(buf != nullptr);
        if (step != Step::Prepared)
            return 0;
        if (buffer.size() <= 0) {
            if (!loadBuffer())
                return 0;
        }
        buf->swap(buffer);
        buffer.resize(0);
        return buf->size();
    }

    Status SetRange(uint32_t offset, uint32_t size) override {
        if (step != Step::Prepared)
            return Status(Cause::Forbidden);
        offset_ = offset;
        size_expected_ = size;
        return Status();
    }
};

DownloadBuilder::DownloadBuilder() {}

DownloadBuilder::~DownloadBuilder() {}

void DownloadBuilder::Path(const std::string &p) { path.assign(p); }

std::unique_ptr<oio::api::blob::Download> DownloadBuilder::Build() {
    auto dl = new LocalDownload;
    dl->path.assign(path);
    return std::unique_ptr<LocalDownload>(dl);
}


/**
 *
 */
class LocalRemoval : public oio::api::blob::Removal {
    friend class RemovalBuilder;

 private:
    std::string path;
    Step step_;

 public:
    LocalRemoval(): step_{Step::Init} {}

    ~LocalRemoval() {}

    Status Prepare() override {
        if (step_ != Step::Init)
            return Status(Cause::InternalError);
        struct stat st;
        if (0 == ::stat(path.c_str(), &st)) {
            step_ = Step::Prepared;
            return Status();
        }
        return Errno();
    }

    Status Commit() override {
        if (step_ != Step::Prepared)
            return Status(Cause::InternalError);
        step_ = Step::Done;
        if (0 == ::unlink(path.c_str()))
            return Status();
        return Errno();
    }

    Status Abort() override {
        if (step_ != Step::Prepared)
            return Status(Cause::InternalError);
        step_ = Step::Done;
        return Status();
    }
};

RemovalBuilder::RemovalBuilder() {}

RemovalBuilder::~RemovalBuilder() {}

void RemovalBuilder::Path(const std::string &p) { path.assign(p); }

std::unique_ptr<oio::api::blob::Removal> RemovalBuilder::Build() {
    auto rem = new LocalRemoval;
    rem->path.assign(path);
    return std::unique_ptr<LocalRemoval>(rem);
}


/**
 *
 */
class LocalUpload : public oio::api::blob::Upload {
    friend class UploadBuilder;

 private:
    std::string path_final;
    std::string path_temp;
    std::map<std::string, std::string> attributes;
    int fd;
    mode_t fmode, dmode;
    Step step_;

 private:
    FORBID_COPY_CTOR(LocalUpload);

    FORBID_MOVE_CTOR(LocalUpload);

    explicit LocalUpload(const std::string &p) : path_final(p), path_temp(),
                                                 fd{-1},
                                                 fmode(FLAGS_mode_create),
                                                 dmode(FLAGS_mode_mkdir),
                                                 step_{Step::Init} {
        path_temp = path_final + ".pending";
    }

    bool resetFD() {
        if (fd < 0)
            return false;
        ::close(fd);
        fd = -1;
        return true;
    }

    Status unlinkTempAndReset(Status rc) {
        if (0 != ::unlink(path_temp.c_str()))
            LOG(ERROR) << "Leaving pending file " << path_temp;
        resetFD();
        return rc;
    }

    int createParent(std::string path) {
        // get the parent path
        auto slash = path.rfind('/');
        if (slash == 0)  // the VFS root already exists
            return 0;
        if (slash == std::string::npos)  // relative path head, CWD exists
            return 0;
        auto parent = path.substr(0, slash);

        int rc = ::mkdir(parent.c_str(), dmode);
        if (rc == 0)
            return 0;

        return errno != ENOENT ? errno : createParent(std::move(parent));
    }

    Status prepare_without_check_on_step() {
        bool retryable{true};
        struct stat st;

        if (fd >= 0)
            return Status(Cause::InternalError);
retry:
        fd = ::open(path_temp.c_str(),
                    O_WRONLY | O_CREAT | O_EXCL | O_NONBLOCK,
                    fmode);
        if (fd < 0) {
            if (errno == ENOENT) {
                // TODO(jfs) Lazy directory creation
                if (!retryable)
                    return Status(Cause::Forbidden);
                retryable = false;
                int rc = createParent(path_temp);
                if (rc != 0)
                    return Status(Cause::Forbidden);
                goto retry;
            } else if (errno == EEXIST) {
                return Status(Cause::Already);
            } else if (errno == EPERM || errno == EACCES) {
                return Status(Cause::Forbidden);
            } else {
                // TODO(jfs) have a code for storage full or unavailable
                return Status(Cause::InternalError);
            }
        } else {
            int rc = ::stat(path_final.c_str(), &st);
            if (rc != 0) {
                if (errno == ENOENT)
                    return Status(Cause::OK);
                return unlinkTempAndReset(Errno());
            }
            return unlinkTempAndReset(Status(Cause::Already));
        }
    }

    Status commit_without_check_on_step() {
        if (fd < 0)
            return Status(Cause::InternalError);

        // TODO(jfs) allow seeting all the xattr in a single record
        for (auto e : attributes) {
            std::string k = "user.grid." + e.first;
            int rc = ::fsetxattr(fd, k.c_str(),
                                 e.second.data(), e.second.size(), 0);
            if (0 != rc) {
                LOG(ERROR) << "fsetxattr(" << path_temp.c_str() << ") failed: ("
                           << errno << ") " << ::strerror(errno);
            }
        }

        if (!resetFD())
            return Status(Cause::InternalError);
        if (0 == ::rename(path_temp.c_str(), path_final.c_str()))
            return Status();

        LOG(ERROR) << "rename(" << path_final << ") failed: (" << errno <<
                   ") " << ::strerror(errno);
        return unlinkTempAndReset(Errno());
    }

    Status abort_without_check_on_step() {
        if (!resetFD())
            return Status();
        return unlinkTempAndReset(Status());
    }

 public:
    ~LocalUpload() override {}

    void SetXattr(const std::string &k, const std::string &v) override {
        attributes[k] = v;
        DLOG(INFO) << "Received xattr [" << k << "]";
    }

    Status Prepare() override {
        if (step_ != Step::Init)
            return Status(Cause::InternalError);
        auto rc = prepare_without_check_on_step();
        if (rc.Ok())
            step_ = Step::Prepared;
        return rc;
    }

    Status Commit() override {
        if (step_ != Step::Prepared)
            return Status(Cause::InternalError);
        step_ = Step::Done;
        return commit_without_check_on_step();
    }

    Status Abort() override {
        if (step_ != Step::Prepared)
            return Status(Cause::InternalError);
        step_ = Step::Done;
        return abort_without_check_on_step();
    }

    void Write(const uint8_t *buf, uint32_t len) override {
        assert(buf != nullptr);
        assert(step_ == Step::Prepared);
        assert(fd >= 0);

        ssize_t rc;
retry:
        rc = ::write(fd, buf, len);
        if (rc < 0) {
            if (errno == EINTR)
                goto retry;
            if (errno == EAGAIN) {
                int evt = fdwait(fd, FDW_OUT, mill_now() + 1000);
                if (evt & FDW_OUT)
                    goto retry;
                LOG(ERROR) << "write failed, polled mentions an error";
            } else {
                LOG(ERROR) << "write(" << len << ") error: (" << errno << ") "
                           <<
                           ::strerror(errno);
            }
        } else if (rc < len) {
            buf += rc;
            len -= rc;
            goto retry;
        }
    }
};

UploadBuilder::UploadBuilder() : path(),
                                 fmode(FLAGS_mode_create),
                                 dmode(FLAGS_mode_mkdir) {}

UploadBuilder::~UploadBuilder() {}

void UploadBuilder::Path(const std::string &p) { path.assign(p); }

void UploadBuilder::FileMode(unsigned int mode) { fmode = mode; }

void UploadBuilder::DirMode(unsigned int mode) { dmode = mode; }

std::unique_ptr<oio::api::blob::Upload> UploadBuilder::Build() {
    auto ul = new LocalUpload(path);
    ul->fmode = fmode;
    ul->dmode = dmode;
    return std::unique_ptr<LocalUpload>(ul);
}
