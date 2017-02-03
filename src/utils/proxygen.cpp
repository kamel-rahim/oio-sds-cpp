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

template<class Slice>
void ProxygenHTTP<Slice>::initializeSSL() noexcept {
  sslContext = std::make_shared<folly::SSLContext>();
  sslContext->setOptions(SSL_OP_NO_COMPRESSION);
  wangle::SSLContextConfig config;
  sslContext->ciphers(config.sslCiphers);
  sslContext->loadTrustedCertificates(sslCertPath.c_str());
  sslContext->setAdvertisedNextProtocols(sslProtocols);
}

template<class Slice>
void ProxygenHTTP<Slice>::sslHandshakeFollowup(
    HTTPUpstreamSession* session) noexcept {
  auto sslSocket = dynamic_cast<AsyncSSLSocket*>(session->getTransport());
  const unsigned char* nextProto = nullptr;
  unsigned nextProtoLength = 0;
  sslSocket->getSelectedNextProtocol(&nextProto, &nextProtoLength);
}

template<class Slice>
void ProxygenHTTP<Slice>::Connect() noexcept {
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

template<class Slice>
void ProxygenHTTP<Slice>::connectSuccess(HTTPUpstreamSession* session) {
  DLOG(INFO) <<  "Connection successful";
  if (sslEnabled)
    sslHandshakeFollowup(session);
  transaction = session->newTransaction(this);

  proxygen::HTTPMessage message {};
  message.setMethod(method);
  message.setHTTPVersion(1, 1);
  addFieldsOnHeader(&(message.getHeaders()));
  message.setURL(url_);
  transaction->sendHeaders(message);
  ReturnCode(Code::OK);
  DLOG(INFO) << "Header sent";
}
template<class Slice>
void ProxygenHTTP<Slice>::addFieldsOnHeader(proxygen::HTTPHeaders *headers) {
  for (auto field : fields) {
    headers->add(field.first, field.second);
  }
}
template<class Slice>
void ProxygenHTTP<Slice>::onEgressPaused() noexcept {
  // not needed because transaction.isEgressPaused() already
  // give us this information
}
template<class Slice>
void ProxygenHTTP<Slice>::onEgressResumed() noexcept {
    while (!writeQueue.empty()) {
        if (transaction->isEgressPaused())
            return;
        std::shared_ptr<Slice> slice = writeQueue.front();
        writeQueue.pop();
        if (slice->size() >= 0) {
            transaction->sendBody(IOBuf::wrapBuffer(
                slice->data(), (unsigned int)slice->size()));
            sent += slice->size();
        }
    }
}
template<class Slice>
void ProxygenHTTP<Slice>::Abort() noexcept {
  transaction->sendAbort();
}
template<class Slice>
void ProxygenHTTP<Slice>::SendEOM() noexcept {
  if (content_length >= 0) {
    if (sent != content_length) {
      LOG(ERROR) << "Too few bytes have been sent";
      ReturnCode(Code::ClientError);
    }
  } else {
    transaction->sendEOM();
    ReturnCode(Code::OK);
  }
}
template<class Slice>
void ProxygenHTTP<Slice>::Write(const std::shared_ptr<Slice> slice) noexcept {
  if (transaction->isEgressPaused()) {
    writeQueue.push(slice);
  } else {
    if (slice->size() >= 0) {
      transaction->sendBody(folly::IOBuf::wrapBuffer(slice.get()->data(),
     slice.get()->size()));
      sent += slice.get()->size();
    }
  }
  ReturnCode(Code::OK);
}

template<class Slice>
void ProxygenHTTP<Slice>::connectError(const AsyncSocketException& exception) {
  ReturnCode(Code::NetworkError);
  this->exception = exception.getType();
}
template<class Slice>
void ProxygenHTTP<Slice>::onEOM() noexcept {
  eof = true;
  DLOG(INFO) << "End Of Message received";
}

template<class Slice>
void ProxygenHTTP<Slice>::ReadHeader() noexcept {
  headerPromise.set_value(headerReceived);
}

template<class Slice>
void ProxygenHTTP<Slice>::setTransaction(HTTPTransaction* txn) noexcept {
  if (!transaction) {
    delete transaction;
  }
  transaction = txn;
}

template<class Slice>
void ProxygenHTTP<Slice>::onHeadersComplete(
    unique_ptr<HTTPMessage> msg) noexcept {
  headerReceived = std::move(msg);
  ReturnHeader(headerReceived);
  DLOG(INFO) << "Response header received";
}

template<class Slice>
void ProxygenHTTP<Slice>::detachTransaction() noexcept {
  delete transaction;
}

template<class Slice>
void ProxygenHTTP<Slice>::onBody(std::unique_ptr<folly::IOBuf> chain) noexcept {
  if (!readSlice) {
    if (readBuffer)
      readBuffer.get()->prependChain(std::move(chain));
    else
      readBuffer = std::move(chain);
  } else {
    CoalesceAndSetValue();
  }
}

template<class Slice>
void ProxygenHTTP<Slice>::CoalesceAndSetValue() {
  readBuffer.get()->coalesce();
  readSlice.get()->append(std::move(readBuffer.get()->writableData()),
  readBuffer.get()->length());
  ReturnCode(Code::OK);
  readSlice = nullptr;
  readBuffer = nullptr;
}

template<class Slice>
void ProxygenHTTP<Slice>::Read(std::shared_ptr<Slice> slice) noexcept {
  readSlice = slice;
  if (readBuffer) {
    CoalesceAndSetValue();
  }
}

template<class Slice>
void ProxygenHTTP<Slice>::onTrailers(unique_ptr<HTTPHeaders> trailers)
        noexcept {
  this->trailers = std::move(trailers);
}

template<class Slice>
void ProxygenHTTP<Slice>::onUpgrade(UpgradeProtocol protocol) noexcept {
  this->protocol = protocol;
}

template<class Slice>
void  ProxygenHTTP<Slice>::isEof() noexcept {
  boolPromise.set_value(eof);
}

template<class Slice>
void ProxygenHTTP<Slice>::onError(const HTTPException& error) noexcept {
  this->error = new proxygen::HTTPException(error);
  errorHappened = true;
}

template<class Slice>
ProxygenHTTP<Slice>::~ProxygenHTTP() {
}
