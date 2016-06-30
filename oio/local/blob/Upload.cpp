/** Copyright (c) 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#include <oio/local/blob.h>

using oio::local::blob::UploadBuilder;

class LocalUpload : public oio::api::blob::Upload {

};

UploadBuilder::UploadBuilder() noexcept { }

UploadBuilder::~UploadBuilder() noexcept { }

void UploadBuilder::Path(const std::string &p) noexcept { path.assign(p); }

std::unique_ptr<oio::api::blob::Upload> UploadBuilder::Build() noexcept {

}