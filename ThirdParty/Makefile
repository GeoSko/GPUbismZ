# Makefile
# CubismZ
#
# Copyright 2018 ETH Zurich. All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree.
#

#File modified by Georgios Skondras

.PHONY: clean fpzip sz zfp zlib zfp_gpu

all: fpzip lzfour sz zfp zlib zfp_gpu

fpzip:
	$(MAKE) -C fpzip/ CC=$(MPICXX)

lzfour:
	$(MAKE) -C lz4/ CC=$(MPICC) cc=$(MPICC)

sz:
	$(MAKE) -C SZ/ CC=$(MPICC) cc=$(MPICC)

zfp:
	$(MAKE) -C zfp/ CC=$(MPICXX) cc=$(MPICC)

zlib:
	$(MAKE) -C zlib/

zfp_gpu:
	mkdir -p zfp_gpu/zfp/builds/build_gpu
	cd zfp_gpu/zfp/builds/build_gpu && \
	cmake -DZFP_LIBRARY_PREFIX=gpu -DBUILD_EXAMPLES=OFF -DZFP_WITH_CUDA=ON ../.. && \
	cmake --build . --config Release
	mkdir -p build/lib build/include
	cp -r zfp_gpu/zfp/builds/build_gpu/lib/* build/lib/
	cp -r zfp_gpu/zfp/include/* build/include/


clean:
	rm -rf build
	$(MAKE) -C fpzip/ clean
	$(MAKE) -C lz4/ clean
	$(MAKE) -C zfp/ clean
	$(MAKE) -C zlib/ clean
	$(MAKE) -C SZ/ clean
	rm -rf zfp_gpu/zfp/builds/build_gpu  
