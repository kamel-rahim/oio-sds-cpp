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

#ifndef SRC_OIO_SOCKET_MAP_H_
#define SRC_OIO_SOCKET_MAP_H_

#define MAX_DELAY 5000

class SocketElement {
 public:
    void Close() {
        if (_bopen) {
            _socket->close();
            _bopen = false;
        }
    }

    void ReconnectSocket() {
        if (is_open())
            _bopen = _socket->Reconnect();
    }

    std::shared_ptr<net::Socket> *GetSocket(std::string host) {
        if (!is_open()) {
            _socket.reset(new net::MillSocket);
            _bopen = _socket->connect(host);
        } else {
            if (mill_now() > timer)
                _bopen = _socket->Reconnect();
        }

        timer = mill_now() + MAX_DELAY;
        if (is_open())
            return &_socket;
        else
            return NULL;
    }

    bool is_open() { return _bopen; }

 private:
    std::shared_ptr<net::Socket> _socket;
    bool _bopen;
    int64_t timer;
};


class SocketMap {
 public:
    ~SocketMap() {
        for (const auto &e : _SocketMap) {
            SocketElement elm = e.second;
            elm.Close();
        }
    }

    std::shared_ptr<net::Socket> *GetSocket(std::string host) {
        return _SocketMap[host].GetSocket(host);
    }

 private:
    std::map<std::string, SocketElement> _SocketMap;
};

#endif  // SRC_OIO_SOCKET_MAP_H_
