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

#ifndef SRC_UTILS_PROXYGEN_HPP_
#define SRC_UTILS_PROXYGEN_HPP_

#include <proxygen/lib/http/HTTPConnector.h>
#include <proxygen/lib/http/session/HTTPTransaction.h>

#include <cstdint>
#include <future>
#include <list>
#include <map>
#include <memory>
#include <queue>
#include <set>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "http.hpp"
#include "utils.hpp"


namespace http {

template<class Slice>
class ControllerProxygenHTTP;

/**
 * Handle a Connection (Download and Upload) to a HTTPServer
 */
template<class Slice>
class ProxygenHTTP : public proxygen::HTTPConnector::Callback,
                     public proxygen::HTTPTransactionHandler {
    friend class ControllerProxygenHTTP<Slice>;
 public:
    /*
     *Setter of the Proxygen class
     */
    void Trailer(std::string trailer) {
        this->sendTrailers.insert(trailer);
    }

    void EventBase(const std::shared_ptr<folly::EventBase> eventBase) {
        this->eventBase = eventBase;
    }

    void Timer(folly::HHWheelTimer::SharedPtr timer) {
        this->timer = timer;
    }

    void OptionMap(const folly::AsyncSocket::OptionMap &optionMap) {
        this->optionMap = optionMap;
    }

    void SocketAddress(std::unique_ptr<folly::SocketAddress> socketAddress) {
        this->socketAddress = std::move(socketAddress);
    }

    void Timeout(const std::chrono::milliseconds &timeout) {
        this->promiseTimeout = timeout;
    }

    void SSL(std::string certPath, std::list<std::string> protocols) noexcept {
        sslCertPath = certPath;
        sslProtocols = protocols;
        sslEnabled = true;
    }

    void Field(const std::string &key, const std::string &value) noexcept {
        fields[key] = value;
    }

    void Method(const proxygen::HTTPMethod method) noexcept {
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
    void Read(std::shared_ptr<Slice> slice) noexcept;
    /**
     * Return a promise of a bool
     */
    void isEof() noexcept;
    /**
     *
     */
    inline void ReturnHeader(std::shared_ptr<proxygen::HTTPMessage> msg) {
        headerPromise.set_value(msg);
    }

    inline void ReturnCode(Code code) {
        codePromise.set_value(code);
    }

    void URL(std::string url) {
        url_ = url;
    }
    void Abort() noexcept;
    void initializeSSL() noexcept;
    void sslHandshakeFollowup(proxygen::HTTPUpstreamSession* session) noexcept;
    ~ProxygenHTTP();
    void setHeader();
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
    void addFieldsOnHeader(proxygen::HTTPHeaders *headers);
    // Common
    std::promise<bool> boolPromise;
    std::promise<Code> codePromise;
    std::promise<std::shared_ptr<proxygen::HTTPMessage>> headerPromise;
    std::chrono::milliseconds promiseTimeout {5000};
    std::shared_ptr<proxygen::HTTPMessage> headerReceived {nullptr};
    proxygen::HTTPMethod method;
    std::map<std::string, std::string> fields;
    bool errorHappened;
    // Reading
    std::shared_ptr<folly::IOBuf> readBuffer {nullptr};
    bool eof;
    std::shared_ptr<Slice> readSlice;
    // Writing
    int64_t content_length {-1};
    std::queue<std::shared_ptr<Slice>> writeQueue;
    int64_t sent {-1};
    // Param for setting Connection
    std::string url_;
    std::string sslCertPath;
    std::list<std::string> sslProtocols;
    std::set<std::string> sendTrailers;
    proxygen::HTTPException* error {nullptr};
    bool sslEnabled {false};
    std::shared_ptr<folly::EventBase> eventBase {nullptr};
    std::unique_ptr<proxygen::HTTPConnector> connector {nullptr};
    std::shared_ptr<folly::SSLContext> sslContext {nullptr};
    folly::HHWheelTimer::SharedPtr timer {nullptr};
    folly::AsyncSocket::OptionMap optionMap;
    std::unique_ptr<folly::SocketAddress> socketAddress {nullptr};
    std::chrono::milliseconds connectionTimeout {5000};
    proxygen::HTTPTransaction* transaction {nullptr};
    std::string serverName;
    proxygen::UpgradeProtocol protocol;
    std::shared_ptr<proxygen::HTTPHeaders> trailers {nullptr};
    folly::AsyncSocketException::AsyncSocketExceptionType exception;
};

template<class Slice>
class ControllerProxygenHTTP{
 public:
    ControllerProxygenHTTP() {}

    explicit ControllerProxygenHTTP(std::shared_ptr<ProxygenHTTP<Slice>>
                                     proxygenHTTP)
            :proxygenHTTP_{proxygenHTTP} {}

    void SetProxygenHTTP(std::shared_ptr<ProxygenHTTP<Slice>> proxygenHTTP) {
        proxygenHTTP_ = proxygenHTTP;
    }

    void Abort() {
        proxygenHTTP_->eventBase->template
                runInEventBaseThread<ProxygenHTTP<Slice>>
                (proxygenHTTPAbort, proxygenHTTP_.get());
    }

    void Connect() {
        if (proxygenHTTP_.get()->errorHappened)
            return;
        proxygenHTTP_->eventBase->template
                runInEventBaseThread<ProxygenHTTP<Slice>>
                (proxygenHTTPConnect, proxygenHTTP_.get());
    }

    void SendEOM() {
        if (proxygenHTTP_.get()->errorHappened)
            return;
        proxygenHTTP_->eventBase->template
                runInEventBaseThread<ProxygenHTTP<Slice>>
                (proxygenHTTPSendEOM, proxygenHTTP_.get());
    }

    void Read(std::shared_ptr<Slice> slice) {
        if (proxygenHTTP_.get()->errorHappened)
            return;
        std::pair<ProxygenHTTP<Slice>*, std::shared_ptr<Slice>> tmp_pair =
                {proxygenHTTP_.get(), slice};
        proxygenHTTP_->eventBase->template
                runInEventBaseThread
                <std::pair<ProxygenHTTP<Slice>*, std::shared_ptr<Slice>>>
                (proxygenHTTPRead, &tmp_pair);
    }

    void Write(std::shared_ptr<Slice> slice) {
        if (proxygenHTTP_.get()->errorHappened)
            return;
        std::pair<ProxygenHTTP<Slice>*, std::shared_ptr<Slice>> tmp_pair =
                {proxygenHTTP_.get(), slice};
        proxygenHTTP_->eventBase->template
                runInEventBaseThread
                <std::pair<ProxygenHTTP<Slice>*, std::shared_ptr<Slice>>>
                (proxygenHTTPWrite, &tmp_pair);
    }

    Code ReturnCode() {
        if (proxygenHTTP_.get()->errorHappened)
            return Code::NetworkError;
        auto codeFuture = proxygenHTTP_->codePromise.get_future();
        std::future_status status = codeFuture.wait_for(
            proxygenHTTP_->promiseTimeout);
        proxygenHTTP_->codePromise = std::promise<Code>();
        if (status == std::future_status::ready)
            return codeFuture.get();
        return Code::Timeout;
    }

    bool ReturnIsEof() {
        if (proxygenHTTP_.get()->errorHappened)
            return true;
        auto boolFuture = proxygenHTTP_.get()->codePromise.get_future();
        std::future_status status = boolFuture.wait_for(
            proxygenHTTP_->promiseTimeout);
        proxygenHTTP_.get()->boolPromise = std::promise<bool>();
        if (status == std::future_status::ready)
            return boolFuture.get();
        return true;
    }

    std::shared_ptr<proxygen::HTTPMessage> ReturnHeader() {
        auto headerFuture = proxygenHTTP_->headerPromise.get_future();
        std::future_status status = headerFuture.wait_for(
            proxygenHTTP_->promiseTimeout);
        proxygenHTTP_->headerPromise =  std::promise<
            std::shared_ptr<proxygen::HTTPMessage>>();
        if (status == std::future_status::ready)
            return headerFuture.get();
        DLOG(INFO) << "Timeout of the response of the Header";
        return nullptr;
    }

 private:
    static void proxygenHTTPIsEof(ProxygenHTTP<Slice>* proxygenHTTP) {
        proxygenHTTP->IsEof();
    }

    static void proxygenHTTPConnect(ProxygenHTTP<Slice>* proxygenHTTP) {
        proxygenHTTP->Connect();
    }

    static void proxygenHTTPAbort(ProxygenHTTP<Slice>* proxygenHTTP) {
        proxygenHTTP->Abort();
    }

    static void proxygenHTTPSendEOM(ProxygenHTTP<Slice>* proxygenHTTP) {
        proxygenHTTP->SendEOM();
    }

    static void proxygenHTTPWrite(std::pair<ProxygenHTTP<Slice>*,
                                  std::shared_ptr<Slice>>* pair) {
        pair->first->Write(pair->second);
    }

    static void proxygenHTTPRead(std::pair<ProxygenHTTP<Slice>*,
                                 std::shared_ptr<Slice>>* pair) {
        pair->first->Write(pair->second);
    }
    std::shared_ptr<ProxygenHTTP<Slice>> proxygenHTTP_ {nullptr};
};

}  // namespace http

#endif  // SRC_UTILS_PROXYGEN_HPP_
