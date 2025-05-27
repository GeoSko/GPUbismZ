// File created by Georgios Skondras
/* minimal code example showing how to call the zfp (de)compressor */


#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "zfp.h"
#include <omp.h>
#include <cuda_runtime_api.h>


#define MAX_N 1600
#define MIN_N 100  
#define STEP_N 100


void validate(double* array_raw, double* array_recon, size_t nx, size_t ny, size_t nz){
    double max_error=0;
    double sum_error=0;
    double mean_absolute_error=0;
    double diff;
    

    size_t i, j, k;
    for (k = 0; k < nz; k++){
      for (j = 0; j < ny; j++){
        for (i = 0; i < nx; i++) {
            diff = fabs(array_raw[i + nx * (j + ny * k)] - array_recon[i + nx * (j + ny * k)]);
            // printf("%lf\n", diff);
            sum_error += diff;
            if(diff>max_error)
                max_error=diff;
        }
      }
    }

    mean_absolute_error = sum_error/(nx*ny*nz);

    if(mean_absolute_error < 0.01 && sum_error > 0.01){
      fprintf(stderr, "\n * SEEMS CORRECT* \n");
    }else{
        fprintf(stderr, "\n * SEEMS WRONG* \n");
    }

    fprintf(stderr, "\nsum_error:%lf\n", sum_error);
    fprintf(stderr, "max_error:%lf\n", max_error);
    fprintf(stderr, "mean_absolute_error:%lf\n", mean_absolute_error);



}

void init_array(double* array, size_t nx, size_t ny, size_t nz, FILE * fptr){
      fprintf(stderr, "\nInitializing %ldx%ldx%ld array of doubles(%ld cells)\n",nx,ny,nz,nx*ny*nz);
      // Calculate the total number of bytes
      size_t total_bytes = nx * ny * nz * sizeof(double);

      // Calculate the total in gigabytes
      double total_gb = (double)total_bytes / (1024 * 1024 * 1024);
      printf("Total bytes: %zu\n", total_bytes);
      printf("Total gigabytes: %.6f GB\n", total_gb);
      fprintf(fptr, "%f", total_gb);

    /* initialize array to be compressed */
    size_t i, j, k;
    // #pragma omp parallel for private(i,j,k)
    for (k = 0; k < nz; k++){
      for (j = 0; j < ny; j++){
        for (i = 0; i < nx; i++) {
          double x = 2.0 * i / nx;
          double y = 2.0 * j / ny;
          double z = 2.0 * k / nz;
          double res = exp(-(x * x + y * y + z * z));
          array[i + nx * (j + ny * k)] = res;
        }
      }
    }
}

void print_array(double* array, size_t nx, size_t ny, size_t nz){
    size_t i, j, k;
    for (k = 0; k < nz; k++) {
            for (j = 0; j < ny; j++) {
            for (i = 0; i < nx; i++) {
                /* Print the decompressed value at each index */
                printf("%lf ", array[i + nx * (j + ny * k)]);
            }
            printf("\n");  /* new row for clarity */
            }
            printf("\n");  /* new layer for clarity */
        }
}

/* compress  array */
size_t compress(zfp_stream* zfp, zfp_field* field, FILE *fptr)
{
  double start,end, diff;
  size_t zfpsize;    /* byte size of compressed stream */
  if (zfp_stream_set_execution(zfp, zfp_exec_cuda)){
    fprintf(stderr, "\nCUDA ok");
  }else{
    fprintf(stderr, "\nCUDA not ok");
  }
  
  /* compress array and output compressed stream */
  printf("\nCompressing array\n");
  start = omp_get_wtime(); 
  zfpsize = zfp_compress(zfp, field);
  end = omp_get_wtime(); 
  diff = end - start;
  fprintf(fptr,";%f", diff);
  if (!zfpsize) {
    printf("Compression failed\n");
  }
  else{
    printf("Compression ok\n");
    printf ("GPU compression time: %.5lf seconds to run.\n", diff );
  }
  return zfpsize;
}

void decompress(zfp_stream* zfp, zfp_field* field, size_t zfpsize, long unsigned int nx, long unsigned int ny, long unsigned int nz)
{
  double start,end, diff;

  if (zfp_stream_set_execution(zfp, zfp_exec_serial)){
    fprintf(stderr, "\nSerial ok\n");
  }else{
    fprintf(stderr, "\nSerial not ok\n");
  }

  fprintf(stderr, "Decompressing\n");
  /* read compressed stream and decompress and output array */
  // zfpsize = fread(buffer, 1, bufsize, comp_buffer);
  start = omp_get_wtime(); 
  if (!zfp_decompress(zfp, field)) {
      
    fprintf(stderr, "Decompression failed\n");
    // status = EXIT_FAILURE;
  }
  else{
      fprintf(stderr, "Decompression ok\n");
      // size_t i, j, k;
      // printf("\nDecompressed values:\n");
      // for (k = 0; k < nz; k++) {
      //     for (j = 0; j < ny; j++) {
      //     for (i = 0; i < nx; i++) {
      //         /* Print the decompressed value at each index */
      //         printf("%f ", array_recon[i + nx * (j + ny * k)]);
      //     }
      //     printf("\n");  /* new row for clarity */
      //     }
      //     printf("\n");  /* new layer for clarity */
      // }
      // // fwrite(array, sizeof(double), zfp_field_size(field, NULL), stdout);

  }
  end = omp_get_wtime(); 
  diff = end-start;
  printf ("Serial decompression time: %.5lf seconds to run.\n", diff );
  long unsigned int uncompressed_size=nx*ny*nz*sizeof(double);
  fprintf(stderr, "\nUncompressed size: %ld bytes\n",uncompressed_size);
  fprintf(stderr, "Compressed storage:%ld bytes\n", zfpsize);
  fprintf(stderr, "Compression ratio: %f\n", uncompressed_size/((double)zfpsize));

}
 
int main(int argc, char* argv[])
{

  // Initialization
  

//Itinialization
  void * compressed_buffer;
  double* array;
  double* array_recon;
  FILE *fptr;
  fptr = fopen("results_gpu.txt", "w");
  fprintf(fptr, "n;giga_bytes;time");
  
  long unsigned int n;
  
  cudaSetDevice(6); //put the device you want

// Zfp ititialization
  int status = 0;    /* return value: 0 = success */
  zfp_type type;     /* array scalar type */
  zfp_field* field;  /* array meta data */
  zfp_stream* zfp;   /* compressed stream */
  void* buffer;      /* storage for compressed stream */
  size_t bufsize;    /* byte size of compressed buffer */
  bitstream* stream; /* bit stream to write to or read from */
  size_t zfpsize;    /* byte size of compressed stream */

  double tolerance = 1e-3;
  double rate = 2;


  for(n=MIN_N; n<=MAX_N; n+=STEP_N){
    
    fprintf(fptr, "\n%ld;",n);
    array = malloc(n * n * n * sizeof(double));
    array_recon = malloc(n * n * n * sizeof(double));

    if(array==NULL || array_recon==NULL){
      printf("Could not allocate memory.\n");
      exit(1);
    }

    /* allocate meta data for the 3D array a[nz][ny][nx] */
    type = zfp_type_double;
    long unsigned int nx = n, ny = n, nz = n;
    field = zfp_field_3d(array, type, nx, ny, nz);

    /* allocate meta data for a compressed stream */
    zfp = zfp_stream_open(NULL);

    /* set compression mode and parameters via one of four functions */
    // zfp_stream_set_reversible(zfp);
    zfp_stream_set_rate(zfp, rate, type, zfp_field_dimensionality(field), zfp_false);
    // zfp_stream_set_precision(zfp, precision); 
    // zfp_stream_set_accuracy(zfp, tolerance);

    /* allocate buffer for compressed data */
    bufsize = zfp_stream_maximum_size(zfp, field);
    buffer = malloc(bufsize);

    /* associate bit stream with allocated buffer */
    stream = stream_open(buffer, bufsize);
    zfp_stream_set_bit_stream(zfp, stream);
    zfp_stream_rewind(zfp);


        
    init_array(array, n, n, n, fptr);
    // print_array(array, n, n, n);
    zfpsize = compress(zfp, field, fptr);
    field = zfp_field_3d(array_recon, type, nx, ny, nz);
    stream = stream_open(buffer, bufsize);
    zfp_stream_set_bit_stream(zfp, stream);
    zfp_stream_rewind(zfp);
    decompress(zfp, field, zfpsize, n, n ,n);
    validate(array, array_recon, n, n, n);
    free(array);
    free(array_recon);
  }
  
  return 0;
}