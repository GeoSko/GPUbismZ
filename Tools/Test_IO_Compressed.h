/*
 * Test_IO_Compressed.h
 * CubismZ
 *
 * Copyright 2017 ETH Zurich. All rights reserved.
 */
#pragma once

#include "Types_single.h"
#include "SerializerIO_WaveletCompression_MPI_Simple.h"
#include "Reader_WaveletCompression.h"
#include <ArgumentParser.h>
#include <GridMPI.h>
#define VERBOSE 0

#ifdef _USE_HDF_
#include <hdf5.h>
#endif

#ifdef _FLOAT_PRECISION_
#define HDF_REAL H5T_NATIVE_FLOAT
#else
#define HDF_REAL H5T_NATIVE_DOUBLE
#endif

typedef GridMPI < FluidGrid > G;

class Test_IO_Compressed : public Simulation
{
protected:

	int BPDX, BPDY, BPDZ;
	int XPESIZE, YPESIZE, ZPESIZE;
	int channel;
	int step_id;
	bool VERBOSITY;
	G * grid;
	ArgumentParser parser;
	string inputfile_name, inputfile_dataset, outputfile_name;
	int myrank;

	SerializerIO_WaveletCompression_MPI_SimpleBlocking<G, StreamerGridPointIterative> mywaveletdumper;

	void _ic(G& grid)
	{
		vector<BlockInfo> vInfo = grid.getResidentBlocksInfo();
#if VERBOSE
		Real min_u = 1e8, max_u = -1e8;
#endif

		int myrank, mypeindex[3], pesize[3];
		int periodic[3];
		MPI_Comm cartcomm;

		periodic[0] = 1;
		periodic[1] = 1;
		periodic[2] = 1;

		pesize[0] = XPESIZE;
		pesize[1] = YPESIZE;
		pesize[2] = ZPESIZE;

		assert(XPESIZE*YPESIZE*ZPESIZE == MPI::COMM_WORLD.Get_size());

		MPI_Cart_create(MPI_COMM_WORLD, 3, pesize, periodic, true, &cartcomm);
		MPI_Comm_rank(cartcomm, &myrank);
		MPI_Cart_coords(cartcomm, myrank, 3, mypeindex);

		// open HDF5
		// read data block
		// store in grid block

		int rank;
		char filename[256];
		herr_t status;
		hid_t file_id, dataset_id, fspace_id, fapl_id, mspace_id;

		MPI_Comm_rank(MPI_COMM_WORLD, &rank);

		int coords[3];
		coords[0] = mypeindex[0]; coords[1] = mypeindex[1]; coords[2] = mypeindex[2];

		const int NX = BPDX*_BLOCKSIZE_;
		const int NY = BPDY*_BLOCKSIZE_;
		const int NZ = BPDZ*_BLOCKSIZE_;

#if VERBOSE
		printf("coords = %d,%d,%d\n", coords[0], coords[1], coords[2]);
		printf("NX,NY,NZ = %d,%d,%d\n", NX, NY, NZ);
#endif

		static const int NCHANNELS = TOTAL_CHANNELS;

		Real * array_all = new Real[NX * NY * NZ * NCHANNELS];

		vector<BlockInfo> vInfo_local = grid.getResidentBlocksInfo();

		static const int sX = 0;
		static const int sY = 0;
		static const int sZ = 0;

		const int eX = _BLOCKSIZE_;
		const int eY = _BLOCKSIZE_;
		const int eZ = _BLOCKSIZE_;

		hsize_t count[4] = {
			NX,
			NY,
			NZ, NCHANNELS};

		// global domain size as specified by the process grid, the block layout and the block size
		hsize_t dims[4] = {
			grid.getBlocksPerDimension(0)*_BLOCKSIZE_,
			grid.getBlocksPerDimension(1)*_BLOCKSIZE_,
			grid.getBlocksPerDimension(2)*_BLOCKSIZE_, NCHANNELS};

		hsize_t offset[4] = {
			coords[0]*NX,
			coords[1]*NY,
			coords[2]*NZ, 0};

#if VERBOSE
		printf("count = [%d, %d, %d, %d]\n", count[0], count[1], count[2], count[3]);
		printf("dims = [%d, %d, %d, %d]\n", dims[0], dims[1], dims[2], dims[3]);
		printf("offset = [%d, %d, %d, %d]\n", offset[0], offset[1], offset[2], offset[3]);
#endif
		sprintf(filename, "%s", inputfile_name.c_str());

		H5open();
		fapl_id = H5Pcreate(H5P_FILE_ACCESS);
		/*status = H5Pset_fapl_mpio(fapl_id, MPI_COMM_WORLD, MPI_INFO_NULL);*/
		file_id = H5Fopen(filename, H5F_ACC_RDONLY, fapl_id);
		status = H5Pclose(fapl_id);

		dataset_id = H5Dopen2(file_id, inputfile_dataset.c_str(), H5P_DEFAULT);

		fapl_id = H5Pcreate(H5P_DATASET_XFER);
		/*H5Pset_dxpl_mpio(fapl_id, H5FD_MPIO_COLLECTIVE);*/

		fspace_id = H5Dget_space(dataset_id);

#if 1
		if (myrank == 0)
		{
			// Before reading any data, we perform a check that we have specified the correct and exact domain size

			//https://stackoverflow.com/questions/15786626/get-the-dimensions-of-a-hdf5-dataset
			//If your data space is simple (i.e. not null or scalar), then you can get the number of dimensions using H5Sget_simple_extent_ndims:
			const int ndims = H5Sget_simple_extent_ndims(fspace_id);

			//and the size of each dimension using H5Sget_simple_extent_dims:
			hsize_t hdims[ndims];
			H5Sget_simple_extent_dims(fspace_id, hdims, NULL);

			//The dimensions are now stored in hdims.
			for (int i = 0; i < ndims; i++)
				printf("hdims[%d] = %ld\n", i, hdims[i]);

			for (int i = 0; i < ndims; i++)
				if (hdims[i] != dims[i])
				{
					printf("The dataset size does not match the one specified by the user. Aborting... \n");
					MPI_Abort(MPI_COMM_WORLD, 1);
				}
		}

		MPI_Barrier(MPI_COMM_WORLD);
#endif


		H5Sselect_hyperslab(fspace_id, H5S_SELECT_SET, offset, NULL, count, NULL);

		mspace_id = H5Screate_simple(4, count, NULL);
		status = H5Dread(dataset_id, HDF_REAL, mspace_id, fspace_id, fapl_id, array_all);

		MPI_Barrier(MPI_COMM_WORLD);

		// fill in the blocks
		//#pragma omp parallel for
		for(int i=0; i<(int)vInfo.size(); i++)
		{
			BlockInfo info = vInfo[i];
			const int idx[3] = {info.index[0], info.index[1], info.index[2]};
			FluidBlock& b = *(FluidBlock*)info.ptrBlock;
#if VERBOSE
			printf("idx=[%d,%d,%d]\n", idx[0], idx[1], idx[2]);
#endif
			for(int ix=sX; ix<eX; ix++)
				for(int iy=sY; iy<eY; iy++)
					for(int iz=sZ; iz<eZ; iz++)
					{
						const int gx = idx[0]*_BLOCKSIZE_ + ix;
						const int gy = idx[1]*_BLOCKSIZE_ + iy;
						const int gz = idx[2]*_BLOCKSIZE_ + iz;

						Real * const ptr_input = array_all + NCHANNELS*(gz + NZ * (gy + NY * gx));
						Real val = ptr_input[channel];

						b(ix, iy, iz).u = val;
#if VERBOSE
						if (val < min_u) min_u = val;
						if (val > max_u) max_u = val;
#endif
					}
		}

#if VERBOSE
		printf("min_u = %lf max_u = %lf\n", min_u, max_u);
#endif

		status = H5Pclose(fapl_id);
		status = H5Dclose(dataset_id);
		status = H5Sclose(fspace_id);
		status = H5Sclose(mspace_id);
		status = H5Fclose(file_id);

		H5close();
		MPI_Comm_free(&cartcomm);

		delete [] array_all;
	}


	void _setup_mpi_constants(int& xpesize, int& ypesize, int& zpesize)
	{
		xpesize = parser("-xpesize").asInt(1);
		ypesize = parser("-ypesize").asInt(1);
		zpesize = parser("-zpesize").asInt(1);
	}

public:
	const bool isroot;

	Test_IO_Compressed(const bool isroot, const int argc, const char ** argv) : isroot(isroot) , grid(NULL), parser(argc,argv) { }


	void setup()
	{
		parser.unset_strict_mode();

		BPDX = parser("-bpdx").asInt(1);
		BPDY = parser("-bpdy").asInt(1);
		BPDZ = parser("-bpdz").asInt(1);

		step_id = parser("-stepid").asInt(0);
		channel = parser("-channel").asInt(0);

		MPI_Comm_rank(MPI_COMM_WORLD, &myrank);

		inputfile_name = parser("-simdata").asString("none");
		inputfile_dataset = parser("-dataset").asString("/data");

		outputfile_name = parser("-outdata").asString("none");

		if ((inputfile_name == "none")||(outputfile_name == "none"))
		{
			printf("usage: %s -simdata <filename> -outdata <filename> [-swap] [-wtype_read <type1>] [-wtype_write <type2>]\n", "tool");
			exit(1);
		}


		_setup_mpi_constants(XPESIZE, YPESIZE, ZPESIZE);

		VERBOSITY = 0;
		if (isroot)
			VERBOSITY = 1;

		if (VERBOSITY)
		{
			printf("////////////////////////////////////////////////////////////\n");
			printf("////////////      TEST IO Compressed MPI     ///////////////\n");
			printf("////////////////////////////////////////////////////////////\n");
		}

		grid = new G(XPESIZE, YPESIZE, ZPESIZE, BPDX, BPDY, BPDZ);

		assert(grid != NULL);

		_ic(*grid);
		vp(*grid, step_id);

	}


	void vp(G& grid, const int step_id)
	{
		if (isroot) std::cout << "Creating MPI CZ dump...\n" ;

		const string path = parser("-fpath").asString(".");
		const int wtype_write = parser("-wtype_write").asInt(1);

		std::stringstream streamer;
		streamer<<path;
		streamer<<"/";
		streamer<<outputfile_name;

#if defined(_USE_SZ_)
		SZ_Init((char *)"sz.config");
#ifdef _OPENMP
		omp_set_num_threads(1);
#endif
#endif

		double threshold = parser("-threshold").asDouble(0);
		mywaveletdumper.verbose();
		if (isroot) printf("setting threshold to %f\n", threshold);
		mywaveletdumper.set_threshold(threshold);
		mywaveletdumper.set_wtype_write(wtype_write);

		MPI_Barrier(MPI_COMM_WORLD);
		double t0 = MPI_Wtime();
		mywaveletdumper.Write<0>(grid, streamer.str());
		double t1 = MPI_Wtime();

		if (isroot) std::cout << "done" << endl;

		int nthreads;
#ifdef _OPENMP
		#pragma omp parallel
		#pragma omp master
		nthreads = omp_get_num_threads();
#else
		nthreads = 1;
#endif

		if (isroot) std::cout << "threads: " << nthreads << " elapsed time: " << t1-t0 << " seconds" << endl;
	}

	void dispose()
	{
#if defined(_USE_SZ_)
		SZ_Finalize();
#endif

		if (grid!=NULL) {
			delete grid;
			grid = NULL;
		}
	}
};
