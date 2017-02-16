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

#ifndef SRC_OIO_API_BLOB_HPP_
#define SRC_OIO_API_BLOB_HPP_

#include <cstdint>
#include <memory>
#include <string>
#include <vector>


#include "oio/api/common.hpp"

namespace oio {
namespace api {
namespace blob {

enum class TransactionStep {
    Init, Prepared, Done
};

class Slice {
 public:
    virtual uint8_t *data() = 0;
    virtual uint32_t size() = 0;
    virtual void append(uint8_t *data, uint32_t length) = 0;
};

std::ostream& operator<<(std::ostream &out, const TransactionStep s);

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

    virtual Status Write(std::shared_ptr<Slice> s) {
        this->Write(s->data(), s->size());
        return Status();
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

    virtual Status Read(std::shared_ptr<Slice> slice) {
        std::vector<uint8_t> tmp(slice->data(), slice->data() + slice->size());
        if ( Read(&tmp) < 0 )
            return Status(Cause::InternalError);
        return Status();
    }
};


}  // namespace blob
}  // namespace api
}  // namespace oio

#endif  // SRC_OIO_API_BLOB_HPP_
