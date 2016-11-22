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

#ifndef SRC_OIO_SERIALIZE_DEF_BLOB_H_
#define SRC_OIO_SERIALIZE_BLOB_H_

#define MAX_DELAY 5000

#define OPENIO_STR  "OpenIO_chk"

// write string + ";" delimiter mark
#define write_safty_str(ss) {\
        ss << OPENIO_STR;\
        ss << ';';\
}

// write string + ";" delimiter mark
#define write_str(ss, str) {\
        ss << str;\
        ss << ';';\
}

#define read_str(ss, str) {\
        getline(ss, str, ';') ;\
}

// write number + " " delimiter mark
#define write_num(ss, num) {\
        ss << num;\
        ss << ' ';\
}

#define read_num(ss, num) {\
        ss >> num;\
}

#define read_safty_str(ss, OpenIoStr) {\
        getline(ss, OpenIoStr, ';');\
	    if (strcmp (OpenIoStr.data(), OPENIO_STR))\
           return false ;\
        OpenIoStr.clear();\
} ;

#endif  // SRC_OIO_SERIALIZE_BLOB_H_
