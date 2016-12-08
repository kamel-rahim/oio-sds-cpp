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

#ifndef SRC_OIO_EC_COMMAND_H_
#define SRC_OIO_EC_COMMAND_H_

struct rawxSet {
	_rawx rawx ;
    int chunk_number;

    bool operator<(const rawxSet &a) const {
        return chunk_number < a.chunk_number;
    }

    bool operator==(const rawxSet &a) const {
        return chunk_number == a.chunk_number;
    }
};

class ec_cmd {
public:
	std::set<rawxSet> targets;
    std::string req_id;
    int kVal, mVal, nbChunks, EncodingMethod;
    uint32_t ChunkSize;
    _Range range ;

public:

    void Clear () {
    	targets.clear() ;
    	req_id = "" ;
    	kVal = mVal = nbChunks = EncodingMethod = ChunkSize =
		range.range_size  =  range.range_start = 0;
    }

    ec_cmd& operator=(const ec_cmd& arg) {
    	for (const auto &to : arg.targets)
    	  targets.insert(to);

    	req_id            = arg.req_id ;

        kVal              = arg.kVal ;
        mVal              = arg.mVal ;
        nbChunks          = arg.nbChunks ;
 		EncodingMethod    = arg.EncodingMethod ;
 		ChunkSize         = arg.ChunkSize ;
    	range             = arg.range;

    	return *this;
    }

    bool get (std::vector<uint8_t> &buf) {
    	std::stringstream ss;

        // write safety string
    	write_safty_str (ss)

        // write data
    	ss << targets.size() ;
        for (const auto &to : targets) {
        	write_str (ss, to.rawx.chunk_id );
        	write_str (ss, to.rawx.host     );
        	write_num (ss, to.rawx.port     );
        	write_num (ss, to.chunk_number  );
        }

        write_str (ss, req_id           );
        write_num (ss, kVal             );
        write_num (ss, mVal             );
        write_num (ss, nbChunks         );
		write_num (ss, EncodingMethod   );
		write_num (ss, ChunkSize        );
   		write_num (ss, range.range_start);
        write_num (ss, range.range_size );

        // write safety string
    	write_safty_str (ss)

        std::string s = ss.str();
        buf.resize(s.size());
        memcpy(&buf[0], s.data(), s.size());

        return true ;
    }

    bool put (std::vector<uint8_t> &buf) {
    	std::string s ((const char*) buf.data(), buf.size());
    	std::stringstream ss (s) ;

	   	// sanity check
		std::string OpenIoStr ;
      	read_safty_str (ss, OpenIoStr)

    	// read data
		int size ;
		ss >> size ;
		for (int i = 0 ; i < size; i++) {
			rawxSet to ;
			read_str(ss, to.rawx.chunk_id);
			read_str(ss, to.rawx.host    );
			read_num(ss, to.rawx.port    );
			read_num(ss, to.chunk_number ) ;

			targets.insert(to) ;
		}

		read_str(ss, req_id );

		read_num(ss, kVal );
		read_num(ss, mVal );
		read_num(ss, nbChunks );
		read_num(ss, EncodingMethod );
		read_num(ss, ChunkSize );

		read_num(ss, range.range_start);
		read_num(ss, range.range_size );

	   	// sanity check
      	read_safty_str (ss, OpenIoStr)

		return true ;
    }
};

#endif  // SRC_OIO_EC_COMMAND_H_
