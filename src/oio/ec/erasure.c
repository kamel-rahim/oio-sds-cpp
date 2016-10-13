#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include "erasurecode.h"
#include "erasurecode_helpers.h"
//#include "erasurecode_helpers_ext.h"


int encode(int k, int m, int hd, size_t ChunkSize, unsigned char *buffer, size_t bufferSize, char **encoded_parity, char **encoded_data, uint64_t *encoded_fragment_len)
{
    int ret = -1;
    struct ec_args args = {
        .k = k,
        .m = m,
        .hd = hd,
    };

//        std::vector<int>> fragments_needed;
//        fragments_needed.resize(args->k*args->m*sizeof(int), 0);

    char *data, **parity;
    int *fragments_needed;

    int err = posix_memalign((void **) &data, 16, ChunkSize);
    if (err != 0 || !data) {
        fprintf(stderr, "Could not allocate memory for data\n");
        exit(1);
    }
    memcpy ( data, buffer, bufferSize);

    /*
    * Get handle
    */
    int desc = liberasurecode_instance_create(EC_BACKEND_FLAT_XOR_HD, &args);
    if (desc <= 0) {
        fprintf(stderr, "Could not create libec descriptor\n");
            exit(1);
    }

    int rc = liberasurecode_encode(desc, data, bufferSize,
       	    	                   &encoded_data, &encoded_parity,
        						   encoded_fragment_len);

}
