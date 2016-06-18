//
// Created by jfs on 17/06/16.
//

#include "Upload.h"

using oio::api::blob::Upload;

class HttpUpload : public Upload {
  public:
    virtual ~Upload() { }

    virtual void SetXattr (const std::string &k, const std::string &v) = 0;

    virtual Status Prepare() = 0;

    virtual bool Commit() = 0;

    virtual bool Abort() = 0;

    // buffer copied
    virtual void Write(const uint8_t *buf, uint32_t len) = 0;

    virtual void Write(const std::string &s) = 0;

    virtual void Flush() = 0;

  private:

};

