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

#ifndef SRC_OIO_API_COMMON_H_
#define SRC_OIO_API_COMMON_H_

#include <string>

namespace oio {
namespace api {

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

std::ostream &operator<<(std::ostream &out, const Cause c);

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

std::ostream &operator<<(std::ostream &out, const Status s);

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

}  // namespace api
}  // namespace oio

#endif  // SRC_OIO_API_COMMON_H_