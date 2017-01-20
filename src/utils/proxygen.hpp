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

#ifndef SRC_UTILS_PROXYGEN_HPP_
#define SRC_UTILS_PROXYGEN_HPP_

#include <cstdint>

#include <future>
#include <memory>
#include <vector>

#include <proxygen/lib/http/HTTPConnector.h>
#include <proxygen/lib/http/session/HTTPTransaction.h>

#include "http.hpp"

#include "utils.hpp"

namespace http {
  
  /**
   * Handle a Connection (Download and Upload) to a HTTPServer
   */
  template<class Slice>
  class ProxygenHTTP : public proxygen::HTTPConnector::Callback,
		       public proxygen::HTTPTransactionHandler {
  public:

    /*
    *Setter of the Proxygen class
    */
    void Trailer(std::string trailer){
      this->sendTrailers.insert(trailer);
    }

    void EventBase(const std::shared_ptr<folly::EventBase> eventBase){
      this->eventBase = eventBase;
    }

    void Timer(folly::HHWheelTimer::SharedPtr timer){
      this->timer = timer;
    }

    void OptionMap(const folly::AsyncSocket::OptionMap &optionMap){
      this->optionMap = optionMap;
    }

    void SocketAddress(std::unique_ptr<folly::SocketAddress> socketAddress){
      this->socketAddress = std::move(socketAddress);
    }

    void Timeout(const std::chrono::milliseconds &timeout){
      this->promiseTimeout = timeout;
    }	  

    void SSL(std::string certPath,std::list<std::string> protocols) noexcept{
      sslCertPath = certPath;
      sslProtocols = protocols;
      sslEnabled = true;
    }
    
    void Field(const std::string &key,const std::string &value) noexcept{
      fields[key] = value;
    }

    void Method(const proxygen::HTTPMethod method) noexcept{
      this->method = method;

    }
    // The functions that permit to control ProxygenHTTP
    /**
     * 
     * 
     */
    void Connect() noexcept;
    /**
     * Send a Slice to the other socket
     */
    void Write(const std::shared_ptr<Slice> slice) noexcept;
    /**
     * Send the end the of message, no Write should be done after this point
     */
    void SendEOM() noexcept;
    /**
     * Return a promise of a Slice 
     * if there is no Slice to send the Read will not answer
     * it is up to the caller to add a timeout 
     */
    void Read(Slice* slice) noexcept;
    /**
     * Return a promise of a bool
     */
    bool isEof() noexcept;
    /**
     *
     */
    void Abort() noexcept;
    void initializeSSL() noexcept;
    void sslHandshakeFollowup(proxygen::HTTPUpstreamSession* session) noexcept;
    /**
     * Return a shared_ptr of a slice 
     * or a nullptr if there is a problem or the timeout ended
     */  
    std::shared_ptr<Slice> getReturnSlice();
    /**
     * Return the Code to the last function called
     * or return Code::InternalError if the timeOut happened
     */
    Code getReturnCode();
    /** 
     * Return the Header received from the transaction
     * or a nullptr if there is a problem or the timeout ended
     */
    std::shared_ptr<proxygen::HTTPMessage> getReturnHeader();
    
    ~ProxygenHTTP();
    void setHeader() ;
    void ReadHeader() noexcept;
    // function of the Callback class
    void connectSuccess(proxygen::HTTPUpstreamSession* session);
    void connectError(const folly::AsyncSocketException& exception);
    // function of the HTTPTransactionHandler class
    void onEOM() noexcept;
    void setTransaction(proxygen::HTTPTransaction* txn) noexcept;
    void detachTransaction() noexcept;
    void onHeadersComplete(std::unique_ptr<proxygen::HTTPMessage> msg) noexcept;
    void onEgressPaused() noexcept;
    void onEgressResumed() noexcept;
    void onBody(std::unique_ptr<folly::IOBuf> chain) noexcept;
    void onTrailers(std::unique_ptr<proxygen::HTTPHeaders> trailers) noexcept;
    void onUpgrade(proxygen::UpgradeProtocol protocol) noexcept;
    void onError(const proxygen::HTTPException& error) noexcept;
  private:
    void CoalesceAndSetValue();
    void addFieldsOnHeader(proxygen::HTTPHeaders &headers);
    // Common
     std::promise<std::shared_ptr<Slice>> slicePromise;
     std::promise<Code> codePromise;
     std::promise<bool> eofPromise;
     std::promise<std::shared_ptr<proxygen::HTTPMessage>> headerPromise;
     std::chrono::milliseconds promiseTimeout {0};
     std::shared_ptr<proxygen::HTTPMessage> headerReceived {nullptr};
     proxygen::HTTPMethod method;
     std::map<std::string,std::string> fields;
     std::string sslCertPath;
     std::list<std::string> sslProtocols;
     std::set<std::string> sendTrailers;
     // Reading
     std::unique_ptr<folly::IOBuf> readBuffer {nullptr};
     bool eof;
     Slice readSlice {nullptr};
     // Writing
     std::queue<std::shared_ptr<Slice>> writeQueue;
     // Param for setting Connection
     proxygen::HTTPException* error {nullptr};
     bool sslEnabled;
     std::shared_ptr<folly::EventBase> eventBase {nullptr};
     proxygen::HTTPConnector* connector {nullptr}; 
     std::shared_ptr<folly::SSLContext> sslContext {nullptr};
     folly::HHWheelTimer::SharedPtr timer {nullptr};
     folly::AsyncSocket::OptionMap optionMap;
     std::unique_ptr<folly::SocketAddress> socketAddress {nullptr};
     std::chrono::milliseconds connectionTimeout {0};    
     proxygen::HTTPTransaction* transaction {nullptr};
     std::string serverName;
     proxygen::UpgradeProtocol protocol;
     std::unique_ptr<proxygen::HTTPHeaders> trailers {nullptr};
     folly::AsyncSocketException::AsyncSocketExceptionType exception;
  };
    

}  // namespace http

#endif  // SRC_UTILS_PROXYGEN_HPP_
