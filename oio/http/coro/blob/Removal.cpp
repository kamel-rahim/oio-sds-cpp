/** Copyright (c) 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#include <glog/logging.h>
#include <oio/api/blob.h>
#include <oio/http/coro/blob.h>
#include <utils/Http.h>

using oio::api::blob::Removal;
using oio::http::coro::RemovalBuilder;

class HttpRemoval : public Removal {
    friend class RemovalBuilder;

  public:
    HttpRemoval();

    virtual ~HttpRemoval();

    Status Prepare() override;

    bool Commit() override;

    bool Abort() override;

  private:
    http::Call rpc;
};

HttpRemoval::HttpRemoval() {
}

HttpRemoval::~HttpRemoval() {
}

/* TODO Manage prepare/commit/abort semantics.
 * For example, we could prepare the request here, just sending the headers with
 * the Transfer-Enncoding set to chunked. So we could wait for the commit to
 * finish the request */
Removal::Status HttpRemoval::Prepare() {
    return Removal::Status::OK;
}

bool HttpRemoval::Commit() {
    std::string out;
    auto rc = rpc.Run("", out);
    return (rc == http::Code::OK || rc == http::Code::Done);
}

bool HttpRemoval::Abort() {
    LOG(WARNING) << "Cannot abort a HTTP delete";
    return false;
}

RemovalBuilder::RemovalBuilder() {
}

RemovalBuilder::~RemovalBuilder() {
}

void RemovalBuilder::Name(const std::string &s) {
    name.assign(s);
}

void RemovalBuilder::Host(const std::string &s) {
    host.assign(s);
}

void RemovalBuilder::Field(const std::string &k, const std::string &v) {
    fields[k] = v;
}

void RemovalBuilder::Trailer(const std::string &k) {
    trailers.insert(k);
}

std::shared_ptr<oio::api::blob::Removal> RemovalBuilder::Build(
        std::shared_ptr<net::Socket> socket) {
    auto rm = new HttpRemoval;
    rm->rpc.Socket(socket).Method("DELETE").Selector(name).Field("Host", host);
    std::shared_ptr<Removal> shared(rm);
    return shared;
}