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

#include <iostream>
#include <string>
#include <sstream>


//#include "erasurecode_helpers_ext.h"

using oio::ec::blob::DownloadBuilder;
using oio::ec::blob::RemovalBuilder;
using oio::ec::blob::UploadBuilder;
using oio::api::blob::Status;
using oio::api::blob::Errno;
using oio::api::blob::Cause;


namespace blob = ::oio::api::blob;

#include <oio/local/blob.h>

struct frag_array_set {
    unsigned int num_fragments;
    char **array;
};

class EcDownload : public oio::api::blob::Download {
    friend class DownloadBuilder;

  public:
    bool Target(const oio::ec::blob::rawxSet &to)   { targets.insert(to);  return true; } ;
    void SetXattr (const std::string &k, const std::string &v) { xattr[k] = v; DLOG(INFO) << "Received xattr [" << k << "]"; } ;
    ~EcDownload() {
        DLOG(INFO) << __FUNCTION__;

    }

    size_t add_item_to_missing_mask(unsigned long mask, long pos)
    {
        if (pos < 0) {
            return mask;
        }
        unsigned long f = 1L << pos;
        mask |= f;
        return mask;
    }

    static int create_frags_array_set(struct frag_array_set *set,
            char **data,
            unsigned int num_data_frags,
            char **parity,
            unsigned int num_parity_frags,
            unsigned long missing_mask)
    {
        int rc =0;
        unsigned int num_frags = 0;
        unsigned long i = 0;
        fragment_header_t *header = NULL;
        size_t size = (num_data_frags + num_parity_frags) * sizeof(char *);
        char **array = (char **) malloc(size);

        if (array == NULL) {
            rc = -1;
            goto out;
        }

        //add data frags
        memset(array, 0, size);
        for (i = 0; i < num_data_frags; i++) {
            if ( (missing_mask | 1L << i) == 1) {
                continue;
            }
            header = (fragment_header_t*)data[i];
            if (header == NULL ||
                    header->magic != LIBERASURECODE_FRAG_HEADER_MAGIC) {
                continue;
            }
            array[num_frags++] = data[i];
        }

        //add parity frags
        for (i = 0; i < num_parity_frags; i++) {
            if ( (missing_mask | 1L << (i + num_data_frags)) == 1) {
                continue;
            }
            header = (fragment_header_t*)parity[i];
            if (header == NULL ||
                    header->magic != LIBERASURECODE_FRAG_HEADER_MAGIC) {
                continue;
            }
            array[num_frags++] = parity[i];
        }

        set->num_fragments = num_frags;
        set->array = array;
    out:
        return rc;
    }

    Status Prepare() override {
        encoded_data          = NULL;
        encoded_parity        = NULL;
        encoded_fragment_len  = 0 ;
		fragments_needed      = NULL ;
	    char *out_data        = NULL;
	    uint64_t out_data_len = 0;
	    unsigned long mask = 0;
	    int rc = 0;

	    struct frag_array_set frags;

    	done = false ;

        struct ec_args args;
        args.k = kVal ;
        args.m = mVal ;
        args.hd = 3 ;

        /*
         * Set up data and parity fragments.
         */

        fragments_needed = (int*)malloc(kVal*mVal*sizeof(int));
        if (!fragments_needed) {
        	LOG(ERROR) << "LIBERASURECODE: Could not allocate memory for fragments" ;
        	  return Status(Cause::InternalError);
        }
        memset(fragments_needed, 0, kVal*mVal*sizeof(int));

        int desc = liberasurecode_instance_create(EC_BACKEND_LIBERASURECODE_RS_VAND, &args);
        if (desc <= 0) {
        	LOG(ERROR) << "LIBERASURECODE: Could not create libec descriptor" ;
        	return Status(Cause::InternalError);
        }

        /*
         * read data and parity
         */
        encoded_data = (char **) malloc(sizeof(char *) * kVal);
        if (NULL == *encoded_data) {
        	LOG(ERROR) << "LIBERASURECODE: Could not allocate data buffer" ;
        	return Status(Cause::InternalError);
        }

        encoded_parity = (char **) malloc(sizeof(char *) * mVal);
        if (NULL == *encoded_parity) {
        	LOG(ERROR) << "LIBERASURECODE: Could not allocate parity buffer" ;
        	return Status(Cause::InternalError);
        }

        // read from rawx
        for (const auto &to: targets)
        {
        	const char *homedir;

        	if ((homedir = getenv("HOME")) == NULL) {
        	    homedir = getpwuid(getuid())->pw_dir;
        	}

        	std::string path =  std::string (homedir) + "/oio/rawx-" + std::to_string(to.chunk_number+1) + "/" + to.target ;
        	oio::local::blob::DownloadBuilder builder ;

        	builder.Path(path);

        	auto dl = builder.Build();
        	auto rc = dl->Prepare();
        	char * p = NULL ;
        	if (rc.Ok()) {
        		std::vector<uint8_t> buf;
        		while (!dl->IsEof()) {
        			dl->Read(buf);
           		}
    			int size = buf.size() ;
    			encoded_fragment_len = size ;
    			p = (char *) malloc(sizeof(char) * size);
    			memcpy (p, &buf[0], buf.size());
        	}

        	if (to.chunk_number < kVal)
        		encoded_data [to.chunk_number] = p ;
        	else
        		encoded_parity [to.chunk_number-kVal] = p ;

        	if (p == NULL)  // oups!!!  missing chunk!
        		mask = add_item_to_missing_mask(mask, kVal + mVal -1 - to.chunk_number );
        }


        /*
         * Run Decode
         */


        create_frags_array_set(&frags,encoded_data, args.k, encoded_parity,
        		                args.m, mask);
        rc = liberasurecode_decode(desc, frags.array, frags.num_fragments,
                    		       encoded_fragment_len, 1,
								   &out_data, &out_data_len);

        if (rc == 0) // decode ok we are done!
       	{
//            	bok = true ;
        	if (out_data_len < size_expected)
        	{
        		buffer.resize(out_data_len);
        		memcpy(&buffer[0], out_data, out_data_len) ;
        	}
        	else
        	{
        		buffer.resize(size_expected);
        		memcpy(&buffer[0], &out_data[offset],  size_expected ) ;
        	}
       	}

        free(frags.array);
        free(out_data);

        CleanUp() ;
        liberasurecode_instance_destroy(desc);

        if (rc == 0) // decode ok we are done!
        	return Status(Cause::OK);
        else
        	return Status(Cause::InternalError);
    }

    bool IsEof() override {
        return done;
    }

    int32_t Read(std::vector<uint8_t> &buf) override {
        if (buffer.size() <= 0) {
            return 0;
        }

        buffer.swap(buf);
        buffer.resize(0);
        done = true ;
        return buf.size();
    }

    void CleanUp() {

    	if (encoded_data) {
    		for (int j = 0; j < kVal; j++) {
    			if (encoded_data[j])
    				free(encoded_data[j]);
    		}
    		free(encoded_data);
    		encoded_data = NULL ;
    	}

    	if (encoded_parity) {
    		for (int j = 0; j < mVal; j++) {
    			if (encoded_parity[j])
    				free(encoded_parity[j]);
            }
            free(encoded_parity);
            encoded_parity = NULL ;
    	}
        free(fragments_needed);
        fragments_needed = NULL ;
    }

  private:
    FORBID_MOVE_CTOR(EcDownload);
    FORBID_COPY_CTOR(EcDownload);

    EcDownload() {}

  private:
    std::vector<uint8_t> buffer;
    std::string chunkid;
    std::set<oio::ec::blob::rawxSet> targets;
    std::map<std::string,std::string> xattr;

    std::vector<std::vector<int>> failure_combs;
    int num_failure_combs;

    int kVal, mVal , nbChunks ;
    int64_t chunkSize ;
    int *fragments_needed;
    char **encoded_data ;
    char **encoded_parity ;
    uint64_t encoded_fragment_len ;
    int desc = -1;
    bool done ;
    uint64_t offset, size_expected ;
};

DownloadBuilder::DownloadBuilder() {
}

DownloadBuilder::~DownloadBuilder() {
}

std::unique_ptr<blob::Download> DownloadBuilder::Build() {
	EcDownload *ul = new EcDownload();

    ul->kVal          = kVal;
    ul->mVal          = mVal;
    ul->nbChunks      = nbChunks ;
    ul->chunkSize     = chunkSize ;
    ul->offset        = offset ;
	ul->size_expected = size_expected ;

    for (const auto &to: targets)
        ul->Target(to);

    for (const auto &e: xattrs)
        ul->SetXattr(e.first, e.second);

    return std::unique_ptr<EcDownload>(ul);
}



class EcRemoval : public oio::api::blob::Removal {
    friend class RemovalBuilder;

  public:
    EcRemoval() {
    }

    ~EcRemoval() {
    }

    Status Prepare() override {
    	return Status(Cause::OK);
    }

    Status Commit() override {
    	return Status();
    }


    Status Abort() override {
    	return Status();
    }
  private:
};

RemovalBuilder::RemovalBuilder() {
}

RemovalBuilder::~RemovalBuilder() {
}

class EcUpload : public oio::api::blob::Upload {
    friend class UploadBuilder;

  public:
    bool Target(const oio::ec::blob::rawxSet &to)   { targets.insert(to);  return true; } ;
    void SetXattr (const std::string &k, const std::string &v) override	{ xattr[k] = v; DLOG(INFO) << "Received xattr [" << k << "]"; } ;

    Status Prepare() {
        encoded_data         = NULL;
        encoded_parity       = NULL;
        encoded_fragment_len = 0 ;
        data                 = NULL ;
		parity               = NULL ;
        return Status(Cause::OK);
    }


    Status Commit() override {
        uint ChunkSize = buffer.size() ;
        struct ec_args args;
        args.k = kVal ;
        args.m = mVal ;
        args.hd = 3 ;

        int err = posix_memalign((void **) &data, 16, ChunkSize);
        if (err != 0 || !data) {
        	LOG(ERROR) << "LIBERASURECODE: Could not allocate memory for data";
        	return Status(Cause::InternalError);
        }
        memcpy ( data, &buffer[0], ChunkSize);

        /*
        * Get handle
        */

        int desc = liberasurecode_instance_create(EC_BACKEND_LIBERASURECODE_RS_VAND, &args);
        if (desc <= 0) {
        	LOG(ERROR) << "LIBERASURECODE: Could not create libec descriptor";
        	return Status(Cause::InternalError);
        }

        /*
        * Get encode
        */

        int rc = liberasurecode_encode(desc, data, ChunkSize,
           	    	                   &encoded_data, &encoded_parity,
            						   &encoded_fragment_len);
        if (rc != 0) {
        	LOG(ERROR) << "LIBERASURECODE: encode error";
        	return Status(Cause::InternalError);
        }

        for (const auto &to: targets)
        {
        	const char *homedir;

        	if ((homedir = getenv("HOME")) == NULL) {
        	    homedir = getpwuid(getuid())->pw_dir;
        	}

        	std::string path =  std::string (homedir) + "/oio/rawx-" + std::to_string(to.chunk_number+1) + "/" + to.target ;
        	oio::local::blob::UploadBuilder builder ;

        	builder.Path(path);
        	builder.FileMode(0644);
        	builder.DirMode(0755);

        	auto ul = builder.Build();
        	auto rc = ul->Prepare();

        	if (rc.Ok()) {

        		std::string str (to.chunk_number < kVal ? encoded_data [to.chunk_number] : encoded_parity [to.chunk_number-kVal], encoded_fragment_len) ;
        		ul->Write (str) ;
        		ul->Commit () ;
        	}
        	else
        		ul->Abort() ;
        }

        CleanUp() ;

        liberasurecode_instance_destroy(desc);

    	return Status(Cause::OK) ;
    }

    void CleanUp() {
    	if (encoded_data) {
    		for (int j = 0; j < kVal; j++) {
    			if (encoded_data[j])
    				free(encoded_data[j]);
    		}
    		free(encoded_data);
    		encoded_data = NULL ;
    	}

    	if (encoded_parity) {
    		for (int j = 0; j < mVal; j++) {
    			if (encoded_parity[j])
    				free(encoded_parity[j]);
            }
            free(encoded_parity);
            encoded_parity = NULL ;
    	}

        free(data);
        data = NULL ;
    }

    Status Abort() override {
    	CleanUp () ;
    	return Status(Cause::OK) ;
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

  private:
    std::vector<uint8_t> buffer;
    uint32_t buffer_limit;
    std::string chunkid;
    std::set<oio::ec::blob::rawxSet> targets;
    std::map<std::string,std::string> xattr;

    int kVal, mVal , nbChunks ;
    int64_t offset_pos ;
    char **encoded_data = NULL;
    char **encoded_parity = NULL;
    uint64_t encoded_fragment_len;
    char *data, **parity;
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



