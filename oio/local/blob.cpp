/** Copyright (c) 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <attr/xattr.h>

#include <cassert>
#include <string>
#include <map>

#include <libmill.h>

#include <utils/macros.h>
#include <utils/utils.h>
#include <oio/local/blob.h>

using oio::local::blob::DownloadBuilder;
using oio::local::blob::RemovalBuilder;
using oio::local::blob::UploadBuilder;
using oio::api::blob::Status;

DEFINE_uint64(mode_mkdir, 0755, "Mode for freshly create directories");
DEFINE_uint64(mode_create, 0644, "Mode for freshly created files");

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
            LOG(ERROR) << "Calling WriteHeaders() on a ready object";
            return Status::InternalError;
        }

        fd = ::open(path.c_str(), O_RDONLY | O_NONBLOCK);
        if (0 > fd) {
            if (errno == ENOENT)
                return Status::NotFound;
            return Status::InternalError;
        }

        return Status::OK;
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
            return Status::OK;
        if (errno == ENOENT)
            return Status::NotFound;
        return Status::InternalError;
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


class LocalUpload : public oio::api::blob::Upload {
    friend class UploadBuilder;

  public:
    void SetXattr(const std::string &k, const std::string &v) override {
        attributes[k] = v;
        DLOG(INFO) << "Received xattr [" << k << "]";
    }

    Status Prepare() override {
        bool retryable{true};
        struct stat st;

        if (fd >= 0)
            return Status::InternalError;
retry:
        fd = ::open(path_temp.c_str(),
                    O_WRONLY | O_CREAT | O_EXCL | O_NONBLOCK,
                    fmode);
        if (fd < 0) {
            if (errno == ENOENT) {
                // TODO Lazy directory creation
                if (!retryable)
                    return Status::Forbidden;
                retryable = false;
                int rc = createParent(path_temp);
                if (rc != 0)
                    return Status::Forbidden;
                goto retry;
            } else if (errno == EEXIST) {
                return Status::Already;
            } else if (errno == EPERM || errno == EACCES) {
                return Status::Forbidden;
            } else {
                // TODO have a code for storage full or unavailable
                return Status::InternalError;
            }
        }

        int rc = ::stat(path_final.c_str(), &st);
        if (rc == 0) {
            return Status::Already;
        }

        return Status::OK;
    }

    bool Commit() override {

        if (fd < 0)
            return false;

        // TODO allow seeting all the xattr in a single record
        for (auto e: attributes) {
            std::string k = "user.grid." + e.first;
            int rc = ::fsetxattr(fd, k.c_str(),
                                 e.second.data(), e.second.size(), 0);
            if (0 != rc) {
                LOG(ERROR) << "fsetxattr(" << path_temp.c_str() << ") failed: ("
                           << errno << ") " << ::strerror(errno);
            }
        }

        if (!resetFD())
            return false;
        if (0 == ::rename(path_temp.c_str(), path_final.c_str()))
            return true;
        LOG(ERROR) << "rename(" << path_final << ") failed: (" << errno <<
                   ") " << ::strerror(errno);
        return false;
    }

    bool Abort() override {
        if (!resetFD())
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

    ~LocalUpload() {}

  private:
    FORBID_COPY_CTOR(LocalUpload);

    FORBID_MOVE_CTOR(LocalUpload);

    LocalUpload(const std::string &p) : path_final(p), path_temp(), fd{-1} {
        path_temp = path_final + ".pending";
        LOG(INFO) << "Upload(" << path_final << ")";
    }

    bool resetFD() {
        if (fd < 0)
            return false;
        ::close(fd);
        fd = -1;
        return true;
    }

    int createParent(std::string path) {
        // get the parent path
        auto slash = path.rfind('/');
        if (slash == 0) // the VFS root already exists
            return 0;
        if (slash == std::string::npos) // relative path head, CWD exists
            return 0;
        auto parent = path.substr(0, slash);

        int rc = ::mkdir(parent.c_str(), dmode);
        if (rc == 0)
            return 0;

        return errno != ENOENT ? errno : createParent(std::move(parent));
    }

  private:
    std::string path_final;
    std::string path_temp;
    int fd;
    unsigned int fmode, dmode;
    std::map<std::string,std::string> attributes;
};

UploadBuilder::UploadBuilder(): path(),
                                fmode(FLAGS_mode_create),
                                dmode(FLAGS_mode_mkdir) {
}

UploadBuilder::~UploadBuilder() {
}

void UploadBuilder::Path(const std::string &p) {
    path.assign(p);
}

void UploadBuilder::FileMode(unsigned int mode) {
    fmode = mode;
}

void UploadBuilder::DirMode(unsigned int mode) {
    dmode = mode;
}

std::unique_ptr<oio::api::blob::Upload> UploadBuilder::Build() {
    auto ul = new LocalUpload(path);
    return std::unique_ptr<LocalUpload>(ul);
}