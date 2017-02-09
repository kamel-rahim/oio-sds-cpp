/**
 * This file is part of the OpenIO client libraries
 * Copyright (C) 2017 OpenIO SAS
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

#include "proxygen.hpp"

#include <folly/io/async/SSLContext.h>
#include <proxygen/lib/http/session/HTTPUpstreamSession.h>
#include <wangle/ssl/SSLContextConfig.h>
#include <iostream>

using folly::AsyncSSLSocket;
using folly::AsyncSocketException;
using folly::IOBuf;
using oio::api::blob::Slice;
using http::ProxygenHTTP;
using proxygen::HTTPConnector;
using proxygen::HTTPException;
using proxygen::HTTPHeaders;
using proxygen::HTTPMessage;
using proxygen::HTTPTransaction;
using proxygen::HTTPTransaction;
using proxygen::HTTPUpstreamSession;
using proxygen::UpgradeProtocol;
using std::shared_ptr;
using std::unique_ptr;


void ProxygenHTTP::initializeSSL() noexcept {
  sslContext = std::make_shared<folly::SSLContext>();
  sslContext->setOptions(SSL_OP_NO_COMPRESSION);
  wangle::SSLContextConfig config;
  sslContext->ciphers(config.sslCiphers);
  sslContext->loadTrustedCertificates(sslCertPath.c_str());
  sslContext->setAdvertisedNextProtocols(sslProtocols);
}


void ProxygenHTTP::sslHandshakeFollowup(
    HTTPUpstreamSession* session) noexcept {
  auto sslSocket = dynamic_cast<AsyncSSLSocket*>(session->getTransport());
  const unsigned char* nextProto = nullptr;
  unsigned nextProtoLength = 0;
  sslSocket->getSelectedNextProtocol(&nextProto, &nextProtoLength);
}


void ProxygenHTTP::Connect() noexcept {
    connector = unique_ptr<HTTPConnector>(new HTTPConnector(
        reinterpret_cast<Callback*>(this), timer.get()));
    if (!sslEnabled) {
        connector.get()->connect(eventBase.get(), *socketAddress.get(),
                                 connectionTimeout, optionMap);
    } else {
        initializeSSL();
        connector.get()->connectSSL(eventBase.get(), *socketAddress.get(),
                                    sslContext, nullptr, connectionTimeout,
                                    optionMap, folly::AsyncSocket::anyAddress(),
                                    serverName);
    }
}


void ProxygenHTTP::connectSuccess(HTTPUpstreamSession* session) {
    DLOG(INFO) <<  "Connection successful";
    if (sslEnabled)
        sslHandshakeFollowup(session);
    this->transaction = session->newTransaction(this);

    proxygen::HTTPMessage message {};
    message.setMethod(method);
    message.setHTTPVersion(1, 1);
    addFieldsOnHeader(&(message.getHeaders()));
    message.setURL(url_);
    this->transaction->sendHeaders(message);
    ReturnCode(Code::OK);
    DLOG(INFO) << "Header sent";
 
}

void ProxygenHTTP::addFieldsOnHeader(proxygen::HTTPHeaders *headers) {
  for (auto field : fields) {
    headers->add(field.first, field.second);
  }
}

void ProxygenHTTP::onEgressPaused() noexcept {
  DLOG(INFO) << "Egress is Paused";
  isEgressPaused = true;
}

void ProxygenHTTP::onEgressResumed() noexcept {
  DLOG(INFO) << "Egress resumed";
  isEgressPaused = false;
    while (!writeQueue.empty()) {
        if (isEgressPaused)
            return;
        std::shared_ptr<Slice> slice = writeQueue.front();
        writeQueue.pop();
        if (slice->size() > 0) {
            transaction->sendBody(IOBuf::wrapBuffer(
                slice->data(), (unsigned int)slice->size()));
            sent += slice->size();
        }
    }
}

void ProxygenHTTP::Abort() noexcept {
  transaction->sendAbort();
}

void ProxygenHTTP::SendEOM() noexcept {
    if (contentLength >= 0) {
        if (sent != contentLength) {
            LOG(ERROR) << "Too few bytes have been sent";
            ReturnCode(Code::ClientError);
        }
    } else {
        transaction->sendEOM();
        DLOG(INFO) << "End Of Message sent";
        ReturnCode(Code::OK);
    }
}

void ProxygenHTTP::Write(const std::shared_ptr<Slice> slice) noexcept {
    DLOG(INFO) << "Executing Write";
    if (!isEgressPaused) {
        writeQueue.push(slice);
        if(slice == nullptr)
            DLOG(ERROR) << "Write Slice Null";
    }
    else {
        if (slice->size() > 0) {
            transaction->sendBody(folly::IOBuf::wrapBuffer(slice->data(),
                                                           slice->size()));
            sent += slice->size();
        }
    }
    ReturnCode(Code::OK);
    DLOG(INFO) << "Writing done";
}


void ProxygenHTTP::connectError(const AsyncSocketException& exception) {
  ReturnCode(Code::NetworkError);
  this->exception = exception.getType();
}

void ProxygenHTTP::onEOM() noexcept {
  eof = true;
  DLOG(INFO) << "End Of Message received";
}


void ProxygenHTTP::ReadHeader() noexcept {
  headerPromise.set_value(headerReceived);
}


void ProxygenHTTP::setTransaction(HTTPTransaction* txn) noexcept {
    DLOG(INFO) << "Set Transaction" ;
    if (transaction) {
        DLOG(INFO) << "Transaction already set";
    }else{
        if(false)
            transaction = txn;
    }
}


void ProxygenHTTP::onHeadersComplete(
    unique_ptr<HTTPMessage> msg) noexcept {
  headerReceived = std::move(msg);
  ReturnHeader(headerReceived);
  DLOG(INFO) << "Response header received";
}


void ProxygenHTTP::detachTransaction() noexcept {
  //   delete transaction;
}


void ProxygenHTTP::onBody(std::unique_ptr<folly::IOBuf> chain) noexcept {
  if (!readSlice) {
    if (readBuffer)
      readBuffer.get()->prependChain(std::move(chain));
    else
      readBuffer = std::move(chain);
  } else {
    CoalesceAndSetValue();
  }
}


void ProxygenHTTP::CoalesceAndSetValue() {
  readBuffer.get()->coalesce();
  readSlice.get()->append(std::move(readBuffer.get()->writableData()),
  readBuffer.get()->length());
  ReturnCode(Code::OK);
  readSlice = nullptr;
  readBuffer = nullptr;
}


void ProxygenHTTP::Read(std::shared_ptr<Slice> slice) noexcept {
  readSlice = slice;
  if (readBuffer) {
    CoalesceAndSetValue();
  }
}


void ProxygenHTTP::onTrailers(unique_ptr<HTTPHeaders> trailers)
        noexcept {
  this->trailers = std::move(trailers);
}


void ProxygenHTTP::onUpgrade(UpgradeProtocol protocol) noexcept {
  this->protocol = protocol;
}


void  ProxygenHTTP::isEof() noexcept {
  boolPromise.set_value(eof);
}


void ProxygenHTTP::onError(const HTTPException& error) noexcept {
  this->error = new proxygen::HTTPException(error);
  errorHappened = true;
}


ProxygenHTTP::~ProxygenHTTP() {
}
