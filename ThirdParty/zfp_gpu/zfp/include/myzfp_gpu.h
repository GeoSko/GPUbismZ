// File created my Georgios Skondras

#pragma once
#include<stdio.h>
#include "zfp.h"
#include <cuda_runtime_api.h>

#define CUDA_DEVICE 0



// mabe need to change nx,ny,nz to long unsigned int for large arrays
static int zfp_gpu_compress_buffer(void* array, int nx, int ny, int nz, double tolerance, int is_float, unsigned char *output, size_t *zbytes){

    int status = 0;    /* return value: 0 = success */
    zfp_type type;     /* array scalar type */
    zfp_field* field;  /* array meta data */
    zfp_stream* zfp;   /* compressed stream */
    void* buffer;      /* storage for compressed stream */
    size_t bufsize;    /* byte size of compressed buffer */
    bitstream* stream; /* bit stream to write to or read from */
    size_t zfpsize;    /* byte size of compressed stream */

    // cudaSetDevice(CUDA_DEVICE); //for single gpu
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    cudaSetDevice(rank); //gpu device selection according to mpi rank

    /* allocate meta data for the 3D array a[nz][ny][nx] */
    if (!is_float) {
        type = zfp_type_double;
    }
    else {
        type = zfp_type_float;
    }
    field = zfp_field_3d(array, type, nx, ny, nz);

    /* allocate meta data for a compressed stream */
    zfp = zfp_stream_open(NULL);

    /* set compression mode and parameters via one of four functions */
    // zfp_stream_set_reversible(zfp);
    // zfp_stream_set_rate(zfp, rate, type, zfp_field_dimensionality(field), zfp_false);
    zfp_stream_set_rate(zfp, tolerance, type, zfp_field_dimensionality(field), zfp_false); //here I use the value of tolerance as rate
    // zfp_stream_set_precision(zfp, precision); 
    // zfp_stream_set_accuracy(zfp, tolerance);

    /* allocate buffer for compressed data */
    bufsize = zfp_stream_maximum_size(zfp, field);
    buffer = malloc(bufsize);

    /* associate bit stream with allocated buffer */
    stream = stream_open(buffer, bufsize);
    zfp_stream_set_bit_stream(zfp, stream);
    zfp_stream_rewind(zfp);

    if (!zfp_stream_set_execution(zfp, zfp_exec_cuda )){
        fprintf(stderr, "Could not initialize CUDA parallel execution policy\n");
    }

    /* compress entire array */
    zfpsize = zfp_compress(zfp, field);
    if (!zfpsize) {
        fprintf(stderr, "compression failed\n");
        status = 1;
    }
    else {
        *zbytes = zfpsize;
        memcpy(output, buffer, zfpsize);
        /*fwrite(buffer, 1, zfpsize, stdout);*/
    }

    /* clean up */
    zfp_field_free(field);
    zfp_stream_close(zfp);
    stream_close(stream);
    free(buffer);
    /*  free(array);*/

    return status;
}



static int zfp_gpu_decompress_buffer(void* array, int nx, int ny, int nz, double tolerance, int is_float, unsigned char *input, size_t zbytes, size_t *bytes){

    int status = 0;    /* return value: 0 = success */
    zfp_type type;     /* array scalar type */
    zfp_field* field;  /* array meta data */
    zfp_stream* zfp;   /* compressed stream */
    void* buffer;      /* storage for compressed stream */
    size_t bufsize;    /* byte size of compressed buffer */
    bitstream* stream; /* bit stream to write to or read from */
    size_t zfpsize;    /* byte size of compressed stream */

    // cudaSetDevice(CUDA_DEVICE); //for single gpu
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    cudaSetDevice(rank); //gpu device selection according to mpi rank

    /* allocate meta data for the 3D array a[nz][ny][nx] */
    if (!is_float) {
        type = zfp_type_double;
    }
    else {
        type = zfp_type_float;
    }
    field = zfp_field_3d(array, type, nx, ny, nz);

    /* allocate meta data for a compressed stream */
    zfp = zfp_stream_open(NULL);

    /* set compression mode and parameters via one of four functions */
    // zfp_stream_set_reversible(zfp);
    // zfp_stream_set_rate(zfp, rate, type, zfp_field_dimensionality(field), zfp_false);
    zfp_stream_set_rate(zfp, tolerance, type, zfp_field_dimensionality(field), zfp_false); //here I use the value of tolerance as rate
    // zfp_stream_set_precision(zfp, precision); 
    // zfp_stream_set_accuracy(zfp, tolerance);

    /* allocate buffer for compressed data */
    bufsize = zfp_stream_maximum_size(zfp, field);
    buffer = malloc(bufsize);

    /* associate bit stream with allocated buffer */
    stream = stream_open(buffer, bufsize);
    zfp_stream_set_bit_stream(zfp, stream);
    zfp_stream_rewind(zfp);

    /* compress or decompress entire array */
    /* read compressed stream and decompress array */
    /*zfpsize = fread(buffer, 1, bufsize, stdin);*/
    zfpsize = zbytes;
    memcpy(buffer, input, zfpsize);

    if (!zfp_stream_set_execution(zfp, zfp_exec_cuda )){
        fprintf(stderr, "Could not initialize CUDA parallel execution policy\n");
    }

    if (!zfp_decompress(zfp, field)) {
        fprintf(stderr, "decompression failed\n");
        status = 1;
        *bytes = 0;
    }
    else {
        *bytes = nx*ny*nz*(is_float?sizeof(float):sizeof(double));
    }

    /* clean up */
    zfp_field_free(field);
    zfp_stream_close(zfp);
    stream_close(stream);
    free(buffer);
    /*  free(array);*/

    return status;
    
    
    return 0;
}