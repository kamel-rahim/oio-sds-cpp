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

#ifndef SRC_UTILS_SERIALIZE_DEF_H_
#define SRC_UTILS_SERIALIZE_DEF_H_

#define MAX_DELAY 5000

#define OPENIO_STR  "OpenIO_chk"

// write data + delimiter mark
#define write_any(ss, str, del)\
        ss << str;\
        ss << del;

// read data + delimiter mark
#define read_any(ss, str, del)\
     getline(ss, str, del)

// write delimiter string + delimiter mark
#define write_safty_str(ss) {\
     write_any(ss, OPENIO_STR, ';')\
}

// read OPENIO_STR string + delimiter mark >>>>> And validate! <<<<<
#define read_safty_str(ss, str)\
     read_and_validate(ss, str, OPENIO_STR, ';')

// read string + delimiter mark >>>>> And validate! <<<<<
#define read_and_validate(ss, str, validation, del) {\
     getline(ss, str, del);\
     if (strcmp (str.data(), validation))\
         return false ;\
     str.clear(); \
}

// remove character from string
#define remove_p(str, p)\
     str.erase(remove(str.begin(), str.end(), p), str.end())

// write string + delimiter mark
#define write_str(ss, str)\
     write_any(ss, str, ';')

// read string + delimiter mark
#define read_str(ss, str)\
     read_any(ss, str, ';')

// write number + delimiter mark
#define write_num(ss, num)\
     write_any(ss, num, ' ')

// read number + delimiter mark
#define read_num_with_del(ss, str, num, del)\
     read_any(ss, str, del);\
     num = atoi(str.data())

// write data + delimiter mark
#define read_num(ss, num)\
        ss >> num;

#endif  // SRC_UTILS_SERIALIZE_DEF_H_
