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

#ifndef SRC_OIO_DIRECTORY_BLOB_H_
#define SRC_OIO_DIRECTORY_BLOB_H_

#include <string>
#include <memory>
#include <map>

#include "oio/api/blob.h"
#include "oio/http/blob.h"
#include "oio/api/serialize_def.h"
#include "command.h"

enum calltype { META, METAS, PROPERTIES } ;

class directory {
private :
	_dir_param DirParam ;
	std::shared_ptr<net::Socket> _socket ;

	oio_err HttpCall (string TypeOfCall) ;
	oio_err HttpCall (string TypeOfCall, string data ) ;
	oio_err HttpCall (string method, string TypeOfCall, calltype type );

public:
	directory ( std::string account, std::string container, std::string type ) {
		DirParam.account = account;
		DirParam.container = container;
		DirParam.type = "meta2." +  type;
	}

	void SetSocket (std::shared_ptr<net::Socket> socket ) {
		_socket = socket ;
	}

	void SetData (_dir_param &Param ) {
		DirParam = Param ;
	}

	void AddProperties (string key, string value) {
		DirParam.Properties [key] = value;
	}

	_dir_param &GetData () {
		return DirParam;
	}

	oio_err Create ();
	oio_err Link ();
	oio_err Unlink ();
	oio_err Destroy ();
	oio_err SetProperties ();
	oio_err GetProperties ();
	oio_err DelProperties ();
	oio_err Renew ();
	oio_err Show();
};

#endif  // SRC_OIO_DIRECTORY_BLOB_H_
