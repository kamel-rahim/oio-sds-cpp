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

#ifndef SRC_OIO_RAWX_COMMAND_H_
#define SRC_OIO_RAWX_COMMAND_H_


class _Range {
public:
    uint64_t range_start ;
    uint64_t range_size ;

public:
    _Range& operator=(const _Range& arg) {
       range_start = arg.range_start;
       range_size  = arg.range_size;
       return *this;
    }
} ;

class _rawx {
public:
    std::string chunk_id;
    std::string host;
    int port;

public:
    _rawx& operator=(const _rawx& arg) {
       chunk_id     = arg.chunk_id;
       host         = arg.host;
       port         = arg.port;
       return *this;
    }
} ;

class rawx_cmd {
public:
    _rawx rawx ;
    _Range range ;
     uint32_t ChunkSize;

public:

    void Clear () {
        rawx.chunk_id     =
        rawx.host         =
        rawx.port         =
        ChunkSize         =
        range.range_start =
        range.range_size  = 0 ;
    }

    rawx_cmd& operator=(const rawx_cmd& arg) {
       rawx      = arg.rawx;
       range     = arg.range;
       ChunkSize = arg.ChunkSize ;
       return *this;
    }

    bool get (std::vector<uint8_t> &buf) {

    	std::stringstream ss;

        // write safety string
    	write_safty_str (ss);

        // write data
    	write_str (ss, rawx.chunk_id     );
    	write_str (ss, rawx.host         );
    	write_num (ss, rawx.port         );
    	write_num (ss, ChunkSize         );
   		write_num (ss, range.range_start );
   		write_num (ss, range.range_size  );

        // write safety string
        write_safty_str (ss);

        std::string s = ss.str();
        buf.resize(s.size()) ;
        memcpy(&buf[0], s.data(), s.size());

        return true ;
    }

    bool put (std::vector<uint8_t> &buf) {
    	std::string s ((const char*) buf.data(), buf.size());
    	std::stringstream ss (s) ;

    	// sanity check
		std::string OpenIoStr ;
      	read_safty_str (ss, OpenIoStr);

		// read data
		read_str (ss, rawx.chunk_id      );
      	read_str (ss, rawx.host          );
      	read_num (ss, rawx.port          );
      	read_num (ss, ChunkSize          );
      	read_num (ss, range.range_start  );
      	read_num (ss, range.range_size   );

	   	// sanity check
      	read_safty_str (ss, OpenIoStr);
    	return true ;
    }
} ;

#endif  // SRC_OIO_RAWX_COMMAND_H_
