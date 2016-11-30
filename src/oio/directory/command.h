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

#ifndef SRC_OIO_DIRECTORY_COMMAND_H_
#define SRC_OIO_DIRECTORY_COMMAND_H_

#include "oio/api/serialize_def.h"
#include <string.h>
#include <algorithm>

using namespace std;

enum metatype { META0, META1, META2 } ;

class meta_host {
public:
    string host;
    int port;
    int seq;

    meta_host () {} ;
    meta_host(string _host, int _port, int _seq) {
		host = _host;
		port = _port;
		seq  = _seq ;
	}

public:
    meta_host& operator=(const meta_host& arg) {
		host = arg.host;
		port = arg.port;
		seq  = arg.seq ;
		return *this;
	}
};

class _meta_data {
public:
	metatype    MetaType;
	meta_host   MetaHost;
	string      args;
public:

	_meta_data () {} ;
	_meta_data(metatype _MetaType, string host, int port, int seq) {
		MetaType = _MetaType;
		MetaHost = meta_host(host, port, seq) ;
	}

	_meta_data& operator=(const _meta_data& arg) {
		MetaType = arg.MetaType;
		MetaHost = arg.MetaHost;
		args     = arg.args;
		return *this;
	}

    bool operator<(const _meta_data &a) const {
        return MetaType < a.MetaType;
    }

    bool operator==(const _meta_data &a) const {
        return MetaType == a.MetaType;
    }

};

class _dir_param {
public:
    string account;
    string container;
    string type;
	set<_meta_data> metas;
	std::map<string, string> Properties;

private:
    bool put_meta (stringstream &ss, metatype MetaType) {
    	_meta_data MetaData;
     	string tmpStr ;

        read_and_validate(ss, tmpStr, "seq", ':');  // read seq keyword
        read_num_with_del(ss, tmpStr, MetaData.MetaHost.seq, ',') ;

        read_and_validate(ss, tmpStr, "type", ':'); // read type keyword
        read_any(ss, type, ',') ;

        read_and_validate(ss, tmpStr, "host", ':'); // read host keyword
        read_any(ss, MetaData.MetaHost.host, ':') ;
        read_num_with_del(ss, tmpStr, MetaData.MetaHost.port, ',') ;

        read_and_validate(ss, tmpStr, "args", ':'); // read args keywork
        if (MetaType == META2)
        	ss >> MetaData.args ;
        else
            read_any(ss, MetaData.args, ',') ;

      	MetaData.MetaType = MetaType;
        metas.insert(MetaData);

        return true ;
    }

public:
    _dir_param& operator=(const _dir_param& arg) {
       account      = arg.account;
       container    = arg.container;
       type         = arg.type;
       for (const auto &to : arg.metas)
    	   metas.insert(to);
       return *this;
    }

    bool put_metas (string s) {
     	string tmpStr ;

     	size_t pos = 0 ;
        string v = s;

     	int i = 0;
     	for ( ; i < 3; i++ )
     	{
     		pos= v.find("seq", pos);
            v = v.substr(pos, v.size() - pos);
     	 	stringstream ss (v) ;
     	 	put_meta (v, (metatype) i);
     	 	pos += 5 ;
     	}
       	return true ;
    }

    bool put_meta (string s, metatype MetaType) {
        string v = s;
        string u = "{}[]\" " ;
        for (const auto &p : u)  // strip  {}[]"
        remove_p (v, p);

 	 	stringstream ss (v) ;
      	put_meta (ss,MetaType);
      	return true ;
    }

    bool put_properties (string s) {
        string v = s;
        string u = "{}\"" ;
        for (const auto &p : u)  // strip  {}"
        remove_p (v, p);
	 	stringstream ss (v) ;

	 	string tmpStr ;
        read_and_validate(ss, tmpStr, "properties", ':');  // read properties keyword

        string key ;
        string value ;
        Properties.clear () ;
        for (;;) {
            read_any(ss, key, ':') ;
            read_any(ss, value, ',') ;
            if (!key.size())
            	break;
            Properties [key] = value ;
            key.clear();
            value.clear();
        }
      	return true ;
    }

    bool get_properties (string &s) {
     	stringstream ss ;
     	ss << "{\"properties\":{\"" ;

     	bool bfirst = true;
        for (const auto &e : Properties) {
        	if (!bfirst)
        		ss << ",\"" ;
            ss << e.first << "\":\"" << e.second << "\"" ;
            bfirst = false ;
        }

        ss << "}}" ;

        s = ss.str();
      	return true ;
     }
} ;

class oio_err {
public:
	int status ;
	string message ;
public:
	oio_err () { status = 0 ; }

	bool put_message(string s) {
		string v = s;
		string u = "{}\"" ;
		for (const auto &p : u)  // strip  {}"
			remove_p (v, p);
		stringstream ss (v) ;

		string tmpStr ;
		read_and_validate(ss, tmpStr, "status", ':');   // read status keyword
		read_num_with_del(ss, tmpStr, status, ',') ;
		read_and_validate(ss, tmpStr, "message", ':');  // read message keyword
		read_any(ss, message, ',') ;
		return true ;
	}

	void get_message (int st, string msg) {
		status = st;
		message = msg ;
	}
} ;


#endif  // SRC_OIO_DIRECTORY_COMMAND_H_
