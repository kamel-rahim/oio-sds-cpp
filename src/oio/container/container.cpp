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

#include "oio/container/container.h"

using user_container::container;

#define REF_DEF   std::string ("?acct=") + ContainerParam.account +\
				  std::string ("&ref=") +  ContainerParam.container +\
				  (ContainerParam.type.size() ? std::string ("&type=meta2.") + ContainerParam.type :  "&type=meta2")

static http::Code HTTP_exec(std::shared_ptr<net::Socket> socket, std::string method, std::string op, bool autocreate,
		                    std::string &in, std::string &out) {
    http::Call call;
    call.Socket(socket)
            .Method(method)
            .Selector("/v3.0/OPENIO/container/" + op)
            .Field("Connection", "keep-alive");
    if (autocreate)
    	call.Field("x-oio-action-mode", "autocreate");
    http::Code rc = call.Run(in, &out);

    LOG(INFO) << method << " rc=" << rc << " reply=" << out;
    return rc ;
}

static http::Code HTTP_exec(std::shared_ptr<net::Socket> socket, std::string method, std::string op, bool autocreate,
		                    std::string &in, std::string &out, std::map<string, string> &system) {

    http::Call call;
    call.Socket(socket)
            .Method(method)
            .Selector("/v3.0/OPENIO/container/" + op)
            .Field("Connection", "keep-alive");
    if (autocreate)
    	call.Field("x-oio-action-mode", "autocreate");
    http::Code rc = call.Run(in, &out);

    if ( rc == http::Code::OK )
    	rc = call.GetReplyHeaders(system, "x-oio-container-meta-sys-");
    return rc ;
}


oio_err container::HttpCall (string TypeOfCall ) {
    string in;
    string out;
	oio_err err;
    http::Code rc = HTTP_exec(_socket, "POST", TypeOfCall, false, in, out) ;
	if (!rc == http::Code::OK)
		err.get_message (-1,"HTTP_exec exec error") ;
	else
		err.put_message (out) ;
	return err;
}

oio_err container::HttpCall (string TypeOfCall, string data, bool autocreate ) {
    string out;
	oio_err err;
    http::Code rc = HTTP_exec(_socket, "POST", TypeOfCall, autocreate, data, out) ;
	if (!rc == http::Code::OK)
		err.get_message (-1,"HTTP_exec exec error") ;
	else
		err.put_message (out) ;
	return err;
}

oio_err container::HttpCall (string method, string TypeOfCall, calltype type ) {
    string in;
    std::string out;
	oio_err err;
    http::Code rc = HTTP_exec(_socket, method,  TypeOfCall, false, in, out, ContainerParam.system) ;
	if (!rc == http::Code::OK)
		err.get_message (-1,"HTTP_exec exec error") ;
	else {
		bool ret ;
		switch (type) {
		case calltype::META:
//			ret = DirParam.put_meta (out, metatype::META2) ;
			break ;
		case calltype::METAS:
//			ret = DirParam.put_metas (out) ;
			break ;
		case calltype::PROPERTIES:
			ret = ContainerParam.put_properties (out);
				break ;
		}
	    if (!ret)
	    	err.put_message (out) ;
	}
	return err;
}

oio_err container::GetProperties () { return HttpCall ("POST", "get_properties" + REF_DEF, calltype::PROPERTIES ); }

oio_err container::Create () {
	string Props;
	ContainerParam.get_properties (Props) ;
	return HttpCall ("create" + REF_DEF, Props, true);
}

oio_err container::SetProperties () {
	string Props;
	ContainerParam.get_properties (Props) ;
	return HttpCall ("set_properties" + REF_DEF, Props, true);
}

oio_err container::DelProperties () {
	string Props ;
	ContainerParam.get_properties_key (Props) ;
	return HttpCall ("del_properties" + REF_DEF, Props, true);
}



oio_err container::Show () { return HttpCall ("GET", "show" + REF_DEF, calltype::META ); }
oio_err container::List () { return HttpCall ("GET", "list" + REF_DEF, calltype::META ); }
oio_err container::Destroy () { return HttpCall ("destroy" + REF_DEF) ; }
oio_err container::Touch () { return HttpCall ("touch" + REF_DEF) ; }
oio_err container::Dedup () { return HttpCall ("dedup" + REF_DEF) ; }









