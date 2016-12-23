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

#ifndef SRC_OIO_BLOB_MEM_BLOB_H_
#define SRC_OIO_BLOB_MEM_BLOB_H_

#include <memory>
#include <vector>
#include <map>
#include <string>

#include "utils/macros.h"
#include "oio/api/blob.h"

/**
 * This implementation is for currently for testing purposes only!
 * No thread-safety at all, here.
 */

namespace oio {
namespace mem {
namespace blob {

class Cache {
 private:
    struct Item {
        std::map<std::string, std::string> xattr;
        std::vector<uint8_t> data;
        bool pending;

        ~Item() {}

        Item() : xattr(), data(), pending{true} {}

        Item(Item &&o) : xattr(), data(), pending{o.pending} {
            data.swap(o.data);
        }

        Item(const Item &o) : xattr(), data(o.data), pending{o.pending} {}

        void Validate() { pending = false; }

        void Xattr(const std::string &k, const std::string &v) { xattr[k] = v; }

        void Write(const uint8_t *buf, size_t len);

        std::vector<uint8_t> Get() const {
            return std::vector<uint8_t>(data.begin(), data.end());
        }
    };

    std::map<std::string, Item> items;

 public:
    ~Cache() {}

    Cache() {}

    /**
     * Create an entry in pending state. No data will be associated
     * @param name the name of the entry
     * @return
     */
    oio::api::Status Create(const std::string &name);

    /**
     * Append data to an entry in pending state
     * @param name the name of the entry
     * @param buf
     * @param len
     * @return
     */
    oio::api::Status Write(const std::string &name,
            const uint8_t *buf, uint32_t len);

    /**
     * Set attributes on a pending entry
     * @param name the name of the entry
     * @param k
     * @param v
     * @return
     */
    oio::api::Status Xattr(const std::string &name,
            const std::string &k, const std::string &v);

    /**
     * Remove the pending state on en entry
     * @param name the name of the entry
     * @return
     */
    oio::api::Status Commit(const std::string &name);

    /**
     * Removes an entry in pending state
     * @param name the name of the entry
     * @return
     */
    oio::api::Status Abort(const std::string &name);

    /**
     * Returns the value associated to the given key in the cache
     * @param name
     * @param out
     * @return
     */
    oio::api::Status Get(const std::string &name,
                               std::vector<uint8_t> *out);

    /**
     * Check if an item is present in the cache (and with which status).
     * @param name the name if the item to be checked
     * @param present present or not
     * @param pending false if absent, pendign or not if present
     */
    void Stat(const std::string &name, bool *present, bool *pending);

    /**
     * Remove the entry whith the given name, if any.
     * @param name the name of the item to be removed
     */
    void Remove(const std::string &name);
};

class UploadBuilder {
 public:
    explicit UploadBuilder(std::shared_ptr<Cache> c) : cache_(c) {}

    ~UploadBuilder() {}

    /**
     * Mandatory
     * @param path
     */
    void Name(const std::string &name) { name_.assign(name); }

    std::unique_ptr<oio::api::blob::Upload> Build();

 private:
    std::shared_ptr<Cache> cache_;
    std::string name_;
};

class DownloadBuilder {
 public:
    explicit DownloadBuilder(std::shared_ptr<Cache> c) : cache_(c) {}

    ~DownloadBuilder() {}

    void Name(const std::string &name) { name_.assign(name); }

    std::unique_ptr<oio::api::blob::Download> Build();

 private:
    std::shared_ptr<Cache> cache_;
    std::string name_;
};

class RemovalBuilder {
 public:
    explicit RemovalBuilder(std::shared_ptr<Cache> c) : cache_(c) {}

    ~RemovalBuilder() {}

    void Name(const std::string &name) { name_.assign(name); }

    std::unique_ptr<oio::api::blob::Removal> Build();

 private:
    std::string name_;
    std::shared_ptr<Cache> cache_;
};

class ListingBuilder {
 public:
    ListingBuilder();

    ~ListingBuilder();

    void Start(const std::string &name);

    void End(const std::string &name);

    std::unique_ptr<oio::api::blob::Listing> Build();

 private:
    std::string start, end;
};

}  // namespace blob
}  // namespace mem
}  // namespace oio


#endif  // SRC_OIO_BLOB_MEM_BLOB_H_
