//
// Created by jfs on 17/06/16.
//

#ifndef OIO_CLIENT_API_OIO_API_BLOB_UPLOAD_H
#define OIO_CLIENT_API_OIO_API_BLOB_UPLOAD_H

#include <map>
#include <memory>
#include <string>
#include <oio/api/blob/Upload.h>

class UploadBuilder {
  public:
    UploadBuilder();
    ~UploadBuilder();
    void Host(const std::string &s);
    void Name(const std::string &s);
    void Xattr(const std::string &k, const std::string &v);
    std::shared_ptr<oio::api::blob::Upload> Build();
  private:
    std::string host;
    std::string chunkid;
    std::map<std::string,std::string> xattr;
};

#endif //OIO_CLIENT_API_OIO_API_BLOB_UPLOAD_H
