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

#include <fcntl.h>

#include <libmill.h>
#include <liberasurecode/erasurecode.h>
#include <glog/logging.h>

#include <algorithm>
#include <cassert>
#include <iostream>
#include <map>
#include <vector>

// JFS: Under certain circumstances, the subsequent header declare an str()
// macro colliding with a glog usage as with a iostream method declaration. One
// way to cope with this is to include glog/logging.h BEFORE, but it would place
// a C++ header before the C header (thus breaking a cpplint rule). I prefer the
// case where the exception is the problematic header.
#include <liberasurecode/erasurecode_helpers.h>  // NOLINT

#include "utils/utils.h"
#include "oio/ec/blob.h"
#include "oio/rawx/blob.h"

using oio::ec::blob::DownloadBuilder;
using oio::ec::blob::RemovalBuilder;
using oio::ec::blob::UploadBuilder;
using oio::api::blob::Status;
using oio::api::blob::Errno;
using oio::api::blob::Cause;

namespace blob = ::oio::api::blob;

oio::ec::blob::SocketMap TheScoketMap;

class EcDownload : public oio::api::blob::Download {
    friend class DownloadBuilder;

 public:
    bool Target(const oio::ec::blob::rawxSet &to) {
        targets.insert(to);
        return true;
    }

    void SetXattr(const std::string &k, const std::string &v) {
        xattr[k] = v;
        DLOG(INFO) << "Received xattr [" << k << "]";
    }

    ~EcDownload() override { DLOG(INFO) << __FUNCTION__; }

    uint64_t add_item_to_missing_mask(uint64_t mask, int pos) {
        if (pos < 0)
            return mask;
        register uint64_t f = 1L << pos;
        return mask | f;
    }

    static int create_frags_array_set(struct oio::ec::blob::frag_array_set *set,
            char **data,
            unsigned int num_data_frags,
            char **parity,
            unsigned int num_parity_frags,
            uint64_t missing_mask) {
        int rc = 0;
        unsigned int num_frags = 0;
        fragment_header_t *header = NULL;
        size_t size = (num_data_frags + num_parity_frags) * sizeof(char *);
        char **array = reinterpret_cast<char **>(malloc(size));

        if (array == NULL) {
            rc = -1;
            goto out;
        }

        // add data frags
        memset(array, 0, size);
        for (unsigned int i = 0; i < num_data_frags; i++) {
            if ((missing_mask | 1L << i) == 1)
                continue;
            header = reinterpret_cast<fragment_header_t *>(data[i]);
            if (header == NULL ||
                header->magic != LIBERASURECODE_FRAG_HEADER_MAGIC) {
                continue;
            }
            array[num_frags++] = data[i];
        }

        // add parity frags
        for (unsigned int i = 0; i < num_parity_frags; i++) {
            if ((missing_mask | 1L << (i + num_data_frags)) == 1)
                continue;
            header = reinterpret_cast<fragment_header_t *>(parity[i]);
            if (header == NULL ||
                header->magic != LIBERASURECODE_FRAG_HEADER_MAGIC) {
                continue;
            }
            array[num_frags++] = parity[i];
        }

        set->num_fragments = num_frags;
        set->array = array;
out:
        return rc;
    }

    Status Prepare() override {
        encoded_data = NULL;
        encoded_parity = NULL;
        encoded_fragment_len = 0;
        char *out_data = NULL;
        uint64_t out_data_len = 0;
        int rc_decode = 1;

        struct oio::ec::blob::frag_array_set frags;

        done = false;

        struct ec_args args;
        args.k = kVal;
        args.m = mVal;
        args.hd = 3;

        // Set up data and parity fragments.
        int desc = liberasurecode_instance_create(
                EC_BACKEND_LIBERASURECODE_RS_VAND, &args);
        if (desc <= 0) {
            LOG(ERROR) << "LIBERASURECODE: Could not create libec descriptor";
            return Status(Cause::InternalError);
        }

        // read data and parity
        encoded_data =
                reinterpret_cast<char **>(malloc(sizeof(char *) * kVal));
        if (NULL == *encoded_data) {
            LOG(ERROR) << "LIBERASURECODE: Could not allocate data buffer";
            return Status(Cause::InternalError);
        }

        encoded_parity =
                reinterpret_cast<char **>(malloc(sizeof(char *) * mVal));
        if (NULL == *encoded_parity) {
            LOG(ERROR) << "LIBERASURECODE: Could not allocate parity buffer";
            return Status(Cause::InternalError);
        }

        memset(encoded_data, 0, sizeof(char *) * kVal);
        memset(encoded_parity, 0, sizeof(char *) * mVal);

        int nbValid = 0;

        // read from rawx
        for (const auto &to : targets) {
            std::shared_ptr<net::Socket> *socket = TheScoketMap.GetSocket(
                    to.host);
            char *p = NULL;
            if (socket) {
                oio::rawx::blob::DownloadBuilder builder;

                builder.ChunkId(to.filename);
                builder.RawxId(to.host);
                auto dl = builder.Build(*socket);
                auto rc = dl->Prepare();

                if (rc.Ok()) {
                    std::vector<uint8_t> buf;
                    while (!dl->IsEof()) {
                        dl->Read(&buf);
                    }
                    int size = buf.size();
                    encoded_fragment_len = size;
                    p = reinterpret_cast<char *>(malloc(sizeof(char) * size));
                    memcpy(p, &buf[0], buf.size());
                }
            } else {
                LOG(ERROR) << "LIBERASURECODE: failed to connect to rawx-"
                           << to.chunk_number;
            }

            // sanity check
            if (p && is_invalid_fragment(desc, p)) {
                free(p);
                p = NULL;
            }

            if (p) {
                nbValid++;
            }

            if (to.chunk_number < kVal)
                encoded_data[to.chunk_number] = p;
            else
                encoded_parity[to.chunk_number - kVal] = p;

            if (nbValid >= kVal) {  // give it a try, we have enough data
                // setup mask ;
                uint64_t mask = 0;

                for (int i = 0; i < (kVal + mVal); i++) {
                    if (i < kVal) {
                        if (!encoded_data[i])
                            mask = add_item_to_missing_mask(
                                    mask, kVal + mVal - 1 - i);
                    } else {
                        if (!encoded_parity[i])
                            mask = add_item_to_missing_mask(
                                    mask, kVal + mVal - 1 - i);
                    }
                }

                // Run Decode
                create_frags_array_set(&frags, encoded_data, args.k,
                                       encoded_parity,
                                       args.m, mask);
                rc_decode = liberasurecode_decode(desc, frags.array,
                                                  frags.num_fragments,
                                                  encoded_fragment_len, 1,
                                                  &out_data, &out_data_len);

                if (rc_decode == 0) {  // decode ok we are done!
                    if (out_data_len < size_expected) {
                        buffer.resize(out_data_len);
                        memcpy(&buffer[0], out_data, out_data_len);
                    } else {
                        buffer.resize(size_expected);
                        memcpy(&buffer[0], &out_data[offset], size_expected);
                    }
                    break;
                }
            }
        }

        free(frags.array);
        free(out_data);

        CleanUp();
        liberasurecode_instance_destroy(desc);

        if (rc_decode == 0)  // decode ok we are done!
            return Status(Cause::OK);
        else
            return Status(Cause::InternalError);
    }

    bool IsEof() override { return done; }

    int32_t Read(std::vector<uint8_t> *buf) override {
        assert(buf != nullptr);
        if (buffer.size() <= 0) {
            return 0;
        }

        buf->swap(buffer);
        buffer.resize(0);
        done = true;
        return buf->size();
    }

    void CleanUp() {
        if (encoded_data) {
            for (int j = 0; j < kVal; j++) {
                if (encoded_data[j])
                    free(encoded_data[j]);
            }
            free(encoded_data);
            encoded_data = NULL;
        }

        if (encoded_parity) {
            for (int j = 0; j < mVal; j++) {
                if (encoded_parity[j])
                    free(encoded_parity[j]);
            }
            free(encoded_parity);
            encoded_parity = NULL;
        }
    }

 private:
    FORBID_MOVE_CTOR(EcDownload);
    FORBID_COPY_CTOR(EcDownload);

    EcDownload() {}

 private:
    std::vector<uint8_t> buffer;
    std::set<oio::ec::blob::rawxSet> targets;
    std::map<std::string, std::string> xattr;

    int kVal, mVal, nbChunks, EncodingMethod;
    int64_t chunkSize;
    char **encoded_data;
    char **encoded_parity;
    uint64_t encoded_fragment_len;
    bool done;
    std::string req_id;
    uint64_t offset, size_expected;
};

DownloadBuilder::DownloadBuilder() {}

DownloadBuilder::~DownloadBuilder() {}

std::unique_ptr<blob::Download> DownloadBuilder::Build() {
    auto ul = new EcDownload();
    ul->kVal = kVal;
    ul->mVal = mVal;
    ul->EncodingMethod = EncodingMethod;
    ul->nbChunks = nbChunks;
    ul->chunkSize = chunkSize;
    ul->offset = offset;
    ul->req_id = req_id;
    ul->size_expected = size_expected;
    for (const auto &to : targets)
        ul->Target(to);
    for (const auto &e : xattrs)
        ul->SetXattr(e.first, e.second);
    return std::unique_ptr<EcDownload>(ul);
}


class EcRemoval : public oio::api::blob::Removal {
    friend class RemovalBuilder;

 public:
    EcRemoval() {}

    ~EcRemoval() {}

    Status Prepare() override { return Status(Cause::OK); }

    Status Commit() override { return Status(); }

    Status Abort() override { return Status(); }
};

RemovalBuilder::RemovalBuilder() {}

RemovalBuilder::~RemovalBuilder() {}


class EcUpload : public oio::api::blob::Upload {
    friend class UploadBuilder;

 public:
    bool Target(const oio::ec::blob::rawxSet &to) {
        targets.insert(to);
        return true;
    }

    void SetXattr(const std::string &k, const std::string &v) override {
        xattr[k] = v;
        DLOG(INFO) << "Received xattr [" << k << "]";
    }

    Status Prepare() {
        encoded_data = NULL;
        encoded_parity = NULL;
        encoded_fragment_len = 0;
        data = NULL;
        parity = NULL;
        return Status(Cause::OK);
    }

    Status Commit() override {
        uint ChunkSize = buffer.size();
        struct ec_args args;
        args.k = kVal;
        args.m = mVal;
        args.hd = 3;

        int err = ::posix_memalign(
                reinterpret_cast<void **>(&data), 16, ChunkSize);
        if (err != 0 || !data) {
            LOG(ERROR) << "LIBERASURECODE: Could not allocate memory for data";
            return Status(Cause::InternalError);
        }
        memcpy(data, &buffer[0], ChunkSize);

        /* Get handle */

        int desc = liberasurecode_instance_create(
                EC_BACKEND_LIBERASURECODE_RS_VAND, &args);
        if (desc <= 0) {
            LOG(ERROR) << "LIBERASURECODE: Could not create libec descriptor";
            return Status(Cause::InternalError);
        }

        /* Get encode */

        int rc = liberasurecode_encode(desc, data, ChunkSize,
                                       &encoded_data, &encoded_parity,
                                       &encoded_fragment_len);
        if (rc != 0) {
            LOG(ERROR) << "LIBERASURECODE: encode error";
            return Status(Cause::InternalError);
        }

        // write to Rawx
        for (const auto &to : targets) {
            std::shared_ptr<net::Socket> *socket = TheScoketMap.GetSocket(
                    to.host);
            if (socket) {
                oio::rawx::blob::UploadBuilder builder;
                builder.ChunkId(to.filename);
                builder.ChunkPosition(offset_pos, 0);
                builder.RawxId(to.host);
                builder.ContainerId(xattr.find("container-id")->second);
                builder.ContentPath(xattr.find("content-path")->second);
                builder.ContentId(xattr.find("content-id")->second);
                int64_t v;
                std::istringstream(xattr.find("content-version")->second) >> v;
                builder.ContentVersion(v);
                builder.StoragePolicy("SINGLE");
                builder.MimeType(xattr.find("content-mime-type")->second);
                builder.ChunkMethod("plain/nb_copy=1");
                auto ul = builder.Build(*socket);
                auto rc = ul->Prepare();
                if (rc.Ok()) {
                    const char *tmp = to.chunk_number < kVal
                                      ? encoded_data[to.chunk_number]
                                      : encoded_parity[to.chunk_number - kVal];

                    std::string s(tmp, encoded_fragment_len);
                    ul->Write(s);
                    ul->Commit();
                } else {
                    ul->Abort();
                }
            } else {
                LOG(ERROR) << "LIBERASURECODE: failed to connect to rawx-"
                           << to.chunk_number;
            }
        }

        CleanUp();

        liberasurecode_instance_destroy(desc);

        return Status(Cause::OK);
    }

    void CleanUp() {
        if (encoded_data) {
            for (int j = 0; j < kVal; j++) {
                if (encoded_data[j])
                    free(encoded_data[j]);
            }
            free(encoded_data);
            encoded_data = NULL;
        }

        if (encoded_parity) {
            for (int j = 0; j < mVal; j++) {
                if (encoded_parity[j])
                    free(encoded_parity[j]);
            }
            free(encoded_parity);
            encoded_parity = NULL;
        }

        free(data);
        data = NULL;
    }

    Status Abort() override {
        CleanUp();

        // delete all saved files.
        for (const auto &to : targets) {
            std::shared_ptr<net::Socket> *socket = TheScoketMap.GetSocket(
                    to.host);
            if (socket) {
                oio::rawx::blob::RemovalBuilder builder;

                builder.ChunkId(to.filename);
                builder.RawxId(to.host);
                auto rm = builder.Build(*socket);
                auto rc = rm->Prepare();

                if (rc.Ok()) {
                    rm->Commit();
                }
            } else {
                LOG(ERROR) << "LIBERASURECODE: failed to connect to rawx-"
                           << to.chunk_number;
            }
        }
        return Status(Cause::OK);
    }

    void Write(const uint8_t *buf, uint32_t len) override {
        while (len > 0) {
            const auto oldsize = buffer.size();
            const uint32_t avail = buffer_limit - oldsize;
            const uint32_t local = std::min(avail, len);
            if (local > 0) {
                buffer.resize(oldsize + local);
                memcpy(buffer.data() + oldsize, buf, local);
                buf += local;
                len -= local;
            }
        }
        yield();
    }

    ~EcUpload() {}

 private:
    FORBID_COPY_CTOR(EcUpload);

    FORBID_MOVE_CTOR(EcUpload);

    EcUpload() {}

 private:
    std::vector<uint8_t> buffer;
    uint32_t buffer_limit;
    std::set<oio::ec::blob::rawxSet> targets;
    std::map<std::string, std::string> xattr;

    int kVal, mVal, nbChunks, EncodingMethod;
    std::string req_id;
    int64_t offset_pos;
    char **encoded_data = NULL;
    char **encoded_parity = NULL;
    uint64_t encoded_fragment_len;
    char *data, **parity;
};

UploadBuilder::UploadBuilder() {}

UploadBuilder::~UploadBuilder() {}

std::unique_ptr<blob::Upload> UploadBuilder::Build() {
    auto ul = new EcUpload();
    ul->buffer_limit = block_size;
    ul->kVal = kVal;
    ul->mVal = mVal;
    ul->EncodingMethod = EncodingMethod;
    ul->req_id = req_id;
    ul->nbChunks = nbChunks;
    ul->offset_pos = offset_pos;
    for (const auto &to : targets)
        ul->Target(to);
    for (const auto &e : xattrs)
        ul->SetXattr(e.first, e.second);
    return std::unique_ptr<EcUpload>(ul);
}
