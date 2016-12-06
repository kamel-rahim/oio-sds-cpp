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


#include <cassert>
#include <string>
#include <functional>
#include <sstream>
#include <memory>
#include <vector>
#include <iomanip>
#include <cstring>

#include "utils/macros.h"
#include "utils/net.h"
#include "utils/Http.h"
#include "oio/api/blob.h"

#include "oio/directory/dir.h"

#define REF_DEF   std::string ("?acct=") + DirParam.account +\
				  std::string ("&ref=") +  DirParam.container +\
				  (DirParam.type.size() ? std::string ("&type=meta2.") + DirParam.type : "&type=meta2")

static http::Code HTTP_exec(std::shared_ptr<net::Socket> socket, std::string method, std::string op, std::string &in, std::string &out) {
    http::Call call;
    call.Socket(socket)
            .Method(method)
            .Selector("/v3.0/OPENIO/reference/" + op)
            .Field("Connection", "keep-alive");
    http::Code rc = call.Run(in, &out);
    LOG(INFO) << method << " rc=" << rc << " reply=" << out;
    return rc ;
}

oio_err directory::HttpCall (string TypeOfCall ) {
    string in;
    string out;
	oio_err err;
    http::Code rc = HTTP_exec(_socket, "POST", TypeOfCall, in, out) ;
	if (!rc == http::Code::OK)
		err.get_message (-1,"HTTP_exec exec error") ;
	else
		err.put_message (out) ;
	return err;
}

oio_err directory::HttpCall (string TypeOfCall, string data ) {
    string out;
	oio_err err;
    http::Code rc = HTTP_exec(_socket, "POST", TypeOfCall, data, out) ;
	if (!rc == http::Code::OK)
		err.get_message (-1,"HTTP_exec exec error") ;
	else
		err.put_message (out) ;
	return err;
}

oio_err directory::HttpCall (string method, string TypeOfCall, calltype type ) {
    string in;
    std::string out;
	oio_err err;
    http::Code rc = HTTP_exec(_socket, method,  TypeOfCall, in, out) ;
	if (!rc == http::Code::OK)
		err.get_message (-1,"HTTP_exec exec error") ;
	else {
		bool ret ;
		switch (type) {
		case calltype::META:
		ret = DirParam.put_meta (out, metatype::META2) ;
			break ;
		case calltype::METAS:
			ret = DirParam.put_metas (out) ;
			break ;
		case calltype::PROPERTIES:
			ret = DirParam.put_properties (out);
				break ;
		}
	    if (!ret)
	    	err.put_message (out) ;
	}
	return err;
}

oio_err directory::Create () { return HttpCall ("create" + REF_DEF) ; }

oio_err directory::Unlink () { return HttpCall ("unlink" + REF_DEF) ; }

oio_err directory::Destroy () { return HttpCall ("destroy" + REF_DEF) ; }

oio_err directory::DelProperties () { return HttpCall ("del_properties" + REF_DEF) ; }

oio_err directory::Renew () { return HttpCall ("Renew" + REF_DEF) ; }

oio_err directory::Show() {	return HttpCall ("GET", "show" + REF_DEF, calltype::METAS ); }

oio_err directory::GetProperties () { return HttpCall ("POST", "get_properties" + REF_DEF, calltype::PROPERTIES ); }

oio_err directory::Link () { return HttpCall ("POST", "link" + REF_DEF, calltype::META ); }

oio_err directory::SetProperties () {
	string Props;
	DirParam.get_properties (Props) ;
	return HttpCall ("set_properties" + REF_DEF, Props);
}


