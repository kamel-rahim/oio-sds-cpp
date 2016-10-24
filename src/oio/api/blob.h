/**
 * Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/
 */

#ifndef SRC_OIO_API_BLOB_H_
#define SRC_OIO_API_BLOB_H_

#include <cstdint>
#include <string>
#include <vector>

namespace oio {
namespace api {
namespace blob {

/**
 * High-level error cause.
 */
enum class Cause {
    OK,
    Already,
    Forbidden,
    NotFound,
    NetworkError,
    ProtocolError,
    Unsupported,
    InternalError
};

std::ostream& operator<<(std::ostream &out, const Cause c);

/**
 * Associates a high-level error classs (the Cause) and an explanation message.
 */
class Status {
 protected:
    Cause rc_;
    std::string msg_;

 public:
    ~Status() {}

    /** Success constructor */
    Status() : rc_{Cause::OK} {}

    /** Build a Status with a cause and a generic message */
    explicit Status(Cause rc) : rc_{rc} {}

    /**
     * Tells if the Status depicts a success (true) or an failure (false)
     * @return true for a success, false for a failure
     */
    inline bool Ok() const { return rc_ == Cause::OK; }

    /**
     * Returns the High-level error cause.
     * @return the underlyinng error cause
     */
    inline Cause Why() const { return rc_; }

    /**
     * Explain what happened, why it failed, etc.
     * The returned pointer validity doesn't exceed the current Status lifetime.
     * @return
     */
    inline const char *Message() const { return msg_.c_str(); }

    /**
     * Handy method to translate the underlying error cause into a pretty
     * string.
     * @return the textual representation of the underlying Cause
     */
    const char *Name() const;
};

std::ostream& operator<<(std::ostream &out, const Status s);

/**
 * Simple wrapper to initiate a Status based on the current errno value.
 * Usage:
 *
 * Status failing_method () {
 *   int rc = ::failing_syscall();
 *   if (rc == 0) return Errno(0);
 *   return Errno();
 * }
 */
class Errno : public Status {
 public:
    /**
     * Build a Status with the given errno.
     * @param err an errno value
     * @return a valid Status
     */
    explicit Errno(int err);

    /**
     * Build a Status with the system's errno
     * @return a valid Statuss
     */
    Errno();

    /**
     * Destructor
     */
    ~Errno() {}
};

/**
 * Usage:
 * Listing *list = ...; // get an instance
 * auto rc = list->WriteHeaders(); // mandatory step
 * if (rc != Listing::Status::Ok) {
 *   LOG(ERROR) << Listing::Status2Str(rc);
 * } else {
 *   std::string id, key;
 *   while (list->Next(id, key)) {
 *     LOG(INFO) << id << " has " << key;
 *   }
 * }
 */
class Listing {
 public:
    virtual ~Listing() {}

    virtual Status Prepare() = 0;

    /**
     * Get the next item of the list. Advances the iterator of one position.
     * @param id cannot be null
     * @param key cannot be null
     * @return true if 'id' and 'key' have been set
     */
    virtual bool Next(std::string *id, std::string *key) = 0;
};

/**
 * Usage:
 * Removal *rem = ...; // get an instance
 * auto rc = rem->WriteHeaders();
 * if (rc != Removal::Status::Ok) {
 *   LOG(ERROR) << Removal::Status2Str(rc);
 * } else {
 *   if (rem->Commit()) {
 *     LOG(INFO) << "Blob removed";
 *   } else {
 *     LOG(ERROR) << "Blob removal failed";
 *   }
 * }
 */
class Removal {
 public:
    virtual ~Removal() {}

    virtual Status Prepare() = 0;

    virtual Status Commit() = 0;

    virtual Status Abort() = 0;
};

/**
 * Usage:
 * LocalUpload *up = ...; // get an instance
 * auto rc = up->WriteHeaders();
 * if (rc != LocalUpload::Status::Ok) {
 *   LOG(ERROR) << "LocalUpload impossible: " << LocalUpload::Status2Str(rc);
 * } else {
 *   // Iterate on chunks of data as soon as they are ready
 *   while (custom_condition)
 *     up->Write("X", 1);
 *   // Validate the whole upload
 *   if (up->Commit()) {
 *     LOG(INFO) << "Blob uploaded";
 *   } else {
 *     LOG(ERROR) << "Blob upload failed";
 *   }
 * }
 */
class Upload {
 public:
    virtual ~Upload() {}

    virtual void SetXattr(const std::string &k, const std::string &v) = 0;

    virtual Status Prepare() = 0;

    virtual Status Commit() = 0;

    virtual Status Abort() = 0;

    // buffer copied
    virtual void Write(const uint8_t *buf, uint32_t len) = 0;

    void Write(const char *str, uint32_t len) {
        return this->Write(reinterpret_cast<const uint8_t *>(str), len);
    }

    void Write(const std::string &s) {
        return this->Write(reinterpret_cast<const uint8_t *>(s.data()),
                           s.size());
    }

    void Write(const std::vector<uint8_t> &s) {
        return this->Write(s.data(), s.size());
    }
};

/**
 * Usage:
 * LocalDownload *dl = ...; // get an instance
 * auto rc = dl->WriteHeaders();
 * if (rc != Downpload::Status::Ok) {
 *   LOG(ERROR) << "LocalDownload impossible: " << LocalDownload::Status2Str(rc);
 * } else {
 *   while (!dl->IsEof()) {
 *     std::vector<uint8_t> chunk;
 *     dl->Read(chunk);
 *     LOG(INFO) << "LocalDownload: " << chunk.size() << " bytes received";
 *   }
 * }
 */
class Download {
 public:
    virtual ~Download() {}

    virtual Status Prepare() = 0;

    /**
     * Are there still data to be read
     * @return false if no further data is to be expected, true elsewhere
     */
    virtual bool IsEof() = 0;

    /**
     * Tell the download to fetch only a given range of the original content.
     * Returns ENOTSUP by default, if not overriden by concrete classes.
     * To be called *before* Prepare(). When managed, the range is not
     * immediately applied, but it can be defered to the Prepare() call.
     * @param offset
     * @param size
     * @return 0 if the request has been taken into account, or the errno value
     * associated to the error that happened.
     */
    virtual Status SetRange(uint32_t offset, uint32_t size);

    /**
     * Steals the whole internal buffer.
     * @param buf the working buffer destined to be reset, in order to received
     * the fetched data.
     * @return the size of the buffer. A negative size means an error occured.
     */
    virtual int32_t Read(std::vector<uint8_t> *buf) = 0;
};

}  // namespace blob
}  // namespace api
}  // namespace oio

#endif  // SRC_OIO_API_BLOB_H_
