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

#ifndef SRC_OIO_SDS_COMMAND_H_
#define SRC_OIO_SDS_COMMAND_H_

#include <set>
#include <map>
#include <string>
#include <algorithm>

#include "oio/directory/command.h"

class oio_sds_param : public FileId {
 public:
    oio_sds_param() { }

    explicit oio_sds_param(FileId &file_id) : FileId(file_id) { }
    oio_sds_param(std::string _name_space, std::string _account,
                  std::string _container, std::string _type = "",
                  std::string _filename = "") :
                  FileId(_name_space, _account, _container, _type,
                           _filename) { }

    oio_sds_param& operator=(const oio_sds_param& arg) {
        FileId::operator =(arg);
        return *this;
    }
};

#endif  // SRC_OIO_SDS_COMMAND_H_
