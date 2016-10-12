/** Copyright (c) 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <attr/xattr.h>

#include <cassert>
#include <string>
#include <map>

#include <glog/logging.h>
#include <libmill.h>

#include <oio/ec/blob.h>
#include <utils/utils.h>
#include <vector>

#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

#include "erasurecode.h"
#include "erasurecode_helpers.h"

#include "erasure.h"

//#include "erasurecode_helpers_ext.h"

using oio::ec::blob::DownloadBuilder;
using oio::ec::blob::RemovalBuilder;
using oio::ec::blob::UploadBuilder;
using oio::api::blob::Status;
using oio::api::blob::Errno;
using oio::api::blob::Cause;


namespace blob = ::oio::api::blob;

#include <oio/local/blob.h>


class EcDownload : public oio::api::blob::Download {
    friend class DownloadBuilder;

  public:
    ~EcDownload() {
        DLOG(INFO) << __FUNCTION__;

    }

    Status Prepare() override {
        return Status(Cause::OK);
    }

    bool IsEof() override {
        return done;
    }

    int32_t Read(std::vector<uint8_t> &buf) override {
        if (buffer.size() <= 0) {
            if (!LoadBuffer())
                return 0;
        }
        buffer.swap(buf);
        buffer.resize(0);
        return buf.size();
    }

  private:
    FORBID_MOVE_CTOR(EcDownload);
    FORBID_COPY_CTOR(EcDownload);
//    EcDownload() : path(), buffer(), fd{-1}, done{false} {
    	EcDownload() : buffer(), done{false} {
        DLOG(INFO) << __FUNCTION__;
    }

    bool LoadBuffer() {
/*
        assert(fd >= 0);
        // TODO make the buffer size configurable
        buffer.resize(1024*1024);
        retry:
        ssize_t rc = ::read(fd, buffer.data(), buffer.size());
        if (rc < 0) {
            if (errno == EAGAIN) {
                int evt = fdwait(fd, FDW_IN, mill_now()+1000);
                if (evt & FDW_IN)
                    goto retry;
                return false;
            }
            return false;
        }
        if (rc == 0) {
            done = true;
            return true;
        }
        buffer.resize(rc);
*/
        return true;
    }

  private:
//    std::string path;
    std::vector<uint8_t> buffer;
    bool done;
};

DownloadBuilder::DownloadBuilder() {
}

DownloadBuilder::~DownloadBuilder() {
}


class EcRemoval : public oio::api::blob::Removal {
    friend class RemovalBuilder;

  public:
    EcRemoval() {
    }

    ~EcRemoval() {
    }

    Status Prepare() override {
        struct stat st;
        if (0 == ::stat(path.c_str(), &st))
            return Status(Cause::OK);
        if (errno == ENOENT)
            return Status(Cause::NotFound);
        return Status (Cause::InternalError);
    }

    Status Commit() override {
    	return Status();
    }

    Status Abort() override {
    	return Status();
    }

  private:
    std::string path;
    int fd;
};

RemovalBuilder::RemovalBuilder() {
}

RemovalBuilder::~RemovalBuilder() {
}

class EcUpload : public oio::api::blob::Upload {
    friend class UploadBuilder;

  public:
    bool Target(const oio::ec::blob::rayxSet &to)   { targets.insert(to);  return true; } ;
    void SetXattr (const std::string &k, const std::string &v) override	{ xattr[k] = v; DLOG(INFO) << "Received xattr [" << k << "]"; } ;
    std::vector<std::vector<int>> failure_combs;
    int num_failure_combs;

    int GenFailureCombs (int k, int m, std::vector<std::vector<int>> &array2D) {
    	int no_of_rows = 0 ;
    	int no_of_cols = 4 ;
    	for (int i = 0 ; i <= k+m; i++ )
    		no_of_rows =  no_of_rows + i ;

    	array2D.resize(no_of_rows, std::vector<int>(no_of_cols, -1));

    	int itr = 0 ;
    	for (int i = 0 ; i < k + m; i++, itr++)
    	{
    		array2D [itr] [0] =  i ;
//    		array2D [itr] [1] = -1 ;
//    		array2D [itr] [2] = -1 ;
//    		array2D [itr] [3] = -1 ;
//    		DLOG(ERROR) << "{" << array2D [itr] [0] << "," << array2D [itr] [1] << "," << array2D [itr] [2] << "," << array2D [itr] [3] << "}" ;
    	}

    	for (int i = 0 ; i < k + m; i ++)
    		for (int j = i+1 ; j < k + m; j++, itr++)
    		{
    			array2D [itr] [0] =  i ;
    			array2D [itr] [1] =  j ;
//    			array2D [itr] [2] = -1 ;
//    			array2D [itr] [3] = -1 ;
//    			DLOG(ERROR) << "{" << array2D [itr] [0] << "," << array2D [itr] [1] << "," << array2D [itr] [2] << "," << array2D [itr] [3] << "}" ;
    		}
    	return no_of_rows ;
    }

    Status Prepare() {
    	num_failure_combs = GenFailureCombs (kVal, mVal, failure_combs);

        return Status(Cause::OK);
    }

    Status Commit() override {

        uint ChunkSize = buffer.size() ;
        char **encoded_data = NULL;
        char **encoded_parity = NULL;
        uint64_t encoded_fragment_len = 0;

        encode(kVal, mVal, 3, ChunkSize, &buffer [0], buffer.size(), encoded_parity, encoded_data, &encoded_fragment_len ) ;

        for (const auto &to: targets)
        {
        	const char *homedir;

        	if ((homedir = getenv("HOME")) == NULL) {
        	    homedir = getpwuid(getuid())->pw_dir;
        	}

        	std::string path =  std::string (homedir) + "/oio/rawx-" + std::to_string(to.chunk_number) + "/" + to.target ;
        	oio::local::blob::UploadBuilder builder ;

        	builder.Path(path);
        	builder.FileMode(0755);
        	builder.DirMode(0644);

        	auto ul = builder.Build();
        	auto rc = ul->Prepare();

        	if (rc.Ok()) {

        		std::string str (to.chunk_number < kVal ? encoded_data [to.chunk_number] : encoded_parity [to.chunk_number-kVal], encoded_fragment_len) ;
//                std::string blah;
//                append_string_random(blah, ChunkSize / (kVal + mVal) , "0123456789ABCDEF");
//        		ul->Write (blah) ;

        		ul->Write (str) ;
        		ul->Commit () ;
        	}
        	else
        		ul->Abort() ;
        }
    	return Status(Cause::OK) ;
    }

    Status Abort() override {
/*
        if (!resetFD())
            return false;
        if (0 == ::unlink(path_temp.c_str()))
            return true;
        LOG(ERROR) << "unlink(" << path_temp << ") failed: (" << errno <<
                   ") " << ::strerror(errno);
        return false;
*/
    	return Status();
    }

    void Write(const uint8_t *buf, uint32_t len) override {
        while (len > 0) {
            const auto oldsize = buffer.size();
            const uint32_t avail = buffer_limit - oldsize;
            const uint32_t local = std::min(avail, len);
            if (local > 0) {
                buffer.resize(oldsize + local);
                memcpy(buffer.data() + oldsize, buf, local);
                buf += local;
                len -= local;
            }
        }
        yield();
    }

    ~EcUpload() {}

  private:
    FORBID_COPY_CTOR(EcUpload);

    FORBID_MOVE_CTOR(EcUpload);

    EcUpload() {}

/*
    EcUpload(const std::string &p) : path_final(p), path_temp(), fd{-1} {
        path_temp = path_final + ".pending";
        LOG(INFO) << "Upload(" << path_final << ")";
    }
*/

    bool resetFD() {
/*
        if (fd < 0)
            return false;
        ::close(fd);
        fd = -1;
*/
        return true;
    }

    int createParent(std::string path) {
/*
        // get the parent path
        auto slash = path.rfind('/');
        if (slash == 0) // the VFS root already exists
            return 0;
        if (slash == std::string::npos) // relative path head, CWD exists
            return 0;
        auto parent = path.substr(0, slash);

        int rc = ::mkdir(parent.c_str(), dmode);
        if (rc == 0)
            return 0;

        return errno != ENOENT ? errno : createParent(std::move(parent));
        */
    	return 0 ;
    }

  private:
/*
    std::string path_final;
    std::string path_temp;
    int fd;
    unsigned int fmode, dmode;
    std::map<std::string,std::string> attributes;
*/

  private:
    std::vector<uint8_t> buffer;
    uint32_t buffer_limit;
    std::string chunkid;
    std::set<oio::ec::blob::rayxSet> targets;
    std::map<std::string,std::string> xattr;

    int kVal, mVal , nbChunks ;
    int64_t offset_pos ;
};

UploadBuilder::UploadBuilder() {
}

UploadBuilder::~UploadBuilder() {
}

std::unique_ptr<blob::Upload> UploadBuilder::Build() {
    EcUpload *ul = new EcUpload();

    ul->buffer_limit = block_size;
    ul->kVal         = kVal;
    ul->mVal         = mVal;
    ul->nbChunks     = nbChunks ;
    ul->offset_pos   = offset_pos ;

    for (const auto &to: targets)
        ul->Target(to);

    for (const auto &e: xattrs)
        ul->SetXattr(e.first, e.second);

    return std::unique_ptr<EcUpload>(ul);
}



