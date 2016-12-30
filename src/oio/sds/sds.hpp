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

#ifndef  SRC_OIO_SDS_SDS_HPP_
#define  SRC_OIO_SDS_SDS_HPP_

#include <string>
#include <memory>
#include <map>
#include <vector>

#include "oio/api/blob.hpp"
#include "oio/blob/http/blob.hpp"
#include "oio/directory/dir.hpp"

namespace oio {
namespace sds {

class SdsClient {
 private :
    using OioUrl = ::oio::directory::OioUrl;
    OioUrl url;

 public:
    explicit SdsClient(const OioUrl &file_id) : url(file_id) {}

    oio::api::OioError upload(std::string filepath, bool autocreate);

    oio::api::OioError upload(std::vector<uint8_t> buffer, bool autocreate);

    oio::api::OioError download(std::string filepath);

    oio::api::OioError download(std::vector<uint8_t> buffer);

    oio::api::OioError destroy(std::string filepath);
};

}  // namespace sds
}  // namespace oio

#endif  //  SRC_OIO_SDS_SDS_HPP_
