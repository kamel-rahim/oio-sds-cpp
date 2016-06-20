/** Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#ifndef OIO_KINETIC_BLOB_H
#define OIO_KINETIC_BLOB_H

#include <cstdint>
#include <string>
#include <vector>

namespace oio {
namespace api {
namespace blob {

/**
 * Usage:
 * Listing *list = ...; // get an instance
 * auto rc = list->Prepare(); // mandatory step
 * if (rc != Listing::Status::Ok) {
 *   std::cerr << Listing::Status2Str(rc) << std::endl;
 * } else {
 *   std::string id, key;
 *   while (list->Next(id, key)) {
 *     std::cerr << id << " has " << key << std::endl;
 *   }
 * }
 */
class Listing {
  public:
    enum class Status {
        OK, NotFound, NetworkError, ProtocolError
    };

    static inline const char *Status2Str(Status s) {
        switch (s) {
            case Status::OK:
                return "OK";
            case Status::NotFound:
                return "Not found";
            case Status::NetworkError:
                return "Network error";
            case Status::ProtocolError:
                return "Protocol error";
            default:
                return "***invalid status***";
        }
    }

  public:
    virtual ~Listing() noexcept { }

    virtual Status Prepare() noexcept = 0;

    virtual bool Next(std::string &id, std::string &key) noexcept = 0;
};

class Removal {
  public:
    enum class Status {
        OK, NotFound, NetworkError, ProtocolError
    };

    static inline const char *Status2Str(Status s) {
        switch (s) {
            case Status::OK:
                return "OK";
            case Status::NotFound:
                return "Not found";
            case Status::NetworkError:
                return "Network error";
            case Status::ProtocolError:
                return "Protocol error";
            default:
                return "***invalid status***";
        }
    }

  public:
    virtual ~Removal() noexcept { }

    virtual Status Prepare() noexcept = 0;

    virtual bool Commit() noexcept = 0;

    virtual bool Abort() noexcept = 0;

    virtual bool Ok() noexcept = 0;
};

class Upload {
  public:
    enum class Status {
        OK, Already, NetworkError, ProtocolError, InternalError
    };

    static inline const char *Status2Str(Status s) {
        switch (s) {
            case Status::OK:
                return "OK";
            case Status::Already:
                return "Already";
            case Status::NetworkError:
                return "Network error";
            case Status::ProtocolError:
                return "Protocol error";
            case Status::InternalError:
                return "Internal error";
            default:
                return "***invalid status***";
        }
    }

  public:
    virtual ~Upload() { }

    virtual void SetXattr(const std::string &k, const std::string &v) = 0;

    virtual Status Prepare() = 0;

    virtual bool Commit() = 0;

    virtual bool Abort() = 0;

    // buffer copied
    virtual void Write(const uint8_t *buf, uint32_t len) = 0;

    void Write(const char *str, uint32_t len) {
        return this->Write(reinterpret_cast<const uint8_t *>(str), len);
    }

    void Write(const std::string &s) {
        return this->Write(reinterpret_cast<const uint8_t *>(s.data()), s.size());
    }

    void Write(const std::vector<uint8_t> &s) {
        return this->Write(s.data(), s.size());
    }

    virtual void Flush() = 0;
};

class Download {
  public:
    enum class Status {
        OK, NotFound, NetworkError, ProtocolError
    };

    static inline const char *Status2Str(Status s) {
        switch (s) {
            case Status::OK:
                return "OK";
            case Status::NotFound:
                return "Not found";
            case Status::NetworkError:
                return "Network error";
            case Status::ProtocolError:
                return "Protocol error";
            default:
                return "***invalid status***";
        }
    }

  public:
    virtual ~Download() noexcept { }

    virtual Status Prepare() noexcept = 0;

    virtual bool IsEof() noexcept = 0;

    virtual int32_t Read(std::vector<uint8_t> &buf) noexcept = 0;
};

} // namespace blob
} // namespace api
} // namespace oio

#endif //OIO_KINETIC_BLOB_H
