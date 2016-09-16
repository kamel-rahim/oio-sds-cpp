/** Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#ifndef OIO_KINETIC_OIO_API_BLOB_H
#define OIO_KINETIC_OIO_API_BLOB_H

#include <cstdint>
#include <string>
#include <vector>

namespace oio {
namespace api {
namespace blob {

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
    virtual ~Listing() { }

    virtual Status Prepare() = 0;

    virtual bool Next(std::string &id, std::string &key) = 0;
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
    enum class Status {
        OK, NotFound, NetworkError, ProtocolError, InternalError
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
            case Status::InternalError:
                return "Internal error";
            default:
                return "***invalid status***";
        }
    }

  public:
    virtual ~Removal() { }

    virtual Status Prepare() = 0;

    virtual bool Commit() = 0;

    virtual bool Abort() = 0;
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
    enum class Status {
        OK, NotFound, NetworkError, ProtocolError, InternalError
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
            case Status::InternalError:
                return "Internal error";
            default:
                return "***invalid status***";
        }
    }

  public:
    virtual ~Download() { }

    virtual Status Prepare() = 0;

    virtual bool IsEof() = 0;

    virtual int32_t Read(std::vector<uint8_t> &buf) = 0;
};

} // namespace blob
} // namespace api
} // namespace oio

#endif //OIO_KINETIC_OIO_API_BLOB_H
