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

#ifndef  SRC_OIO_SDS_SDS_H_
#define  SRC_OIO_SDS_SDS_H_

#include <string>
#include <memory>
#include <map>
#include <vector>

#include "oio/api/blob.h"
#include "oio/http/blob.h"
#include "oio/api/serialize_def.h"
#include "oio/directory/command.h"
#include "oio/sds/command.h"
#include "utils/command.h"


class oio_sds {
 private :
    oio_sds_param oio_sds_Param;

 public:
    explicit oio_sds(_file_id &file_id) : oio_sds_Param(file_id) { }
    oio_sds(std::string _name_space, std::string _account,
            std::string _container, std::string _type = "",
            std::string _filename = "") :
            oio_sds_Param(_name_space, _account, _container,
                          _type, _filename) { }

    oio_err upload(std::string filepath, bool autocreate);
    oio_err upload(std::vector <uint8_t> buffer, bool autocreate);

    oio_err download(std::string filepath);
    oio_err download(std::vector <uint8_t> buffer);

    oio_err destroy(std::string filepath);
};

#endif  //  SRC_OIO_SDS_SDS_H_
