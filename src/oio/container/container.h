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

#ifndef SRC_CONTAINER_H_
#define SRC_CONTAINER_H_

#include <string>
#include <memory>
#include <map>

#include "oio/api/blob.h"
#include "oio/http/blob.h"
#include "oio/api/serialize_def.h"
#include "oio/directory/command.h"
#include "oio/container/command.h"

namespace user_container {

enum calltype { META, METAS, PROPERTIES } ;

class container {
private :
	_container_param ContainerParam ;
	std::shared_ptr<net::Socket> _socket ;

	oio_err HttpCall (string TypeOfCall) ;
	oio_err HttpCall (string TypeOfCall, string data, bool autocreate ) ;
	oio_err HttpCall (string method, string TypeOfCall, calltype type );

public:
	container ( std::string account, std::string container, std::string type ) {
		ContainerParam.account = account;
		ContainerParam.container = container;
		ContainerParam.type = type;
	}

	void SetSocket (std::shared_ptr<net::Socket> socket ) {
		_socket = socket ;
	}

	void SetData (_container_param &Param ) {
		ContainerParam = Param ;
	}

	void AddProperties (string key, string value) {
		ContainerParam.properties [key] = value;
	}

	_container_param &GetData () {
		return ContainerParam;
	}

	oio_err Create ();
	oio_err Show ();
	oio_err List ();
	oio_err Destroy ();
	oio_err Touch ();
	oio_err Dedup ();
	oio_err GetProperties ();
	oio_err SetProperties ();
	oio_err DelProperties ();

};

}  // namespace container

#endif  // SRC_CONTAINER_H_
