#File created by Georgios Skondras

.PHONY: zfp my_tests

all: zfp my_tests

my_tests:
	$(MAKE) -C my_tests/

zfp:
	mkdir -p zfp/builds/build_gpu
	mkdir -p ../build/include/zfp_gpu
	cd zfp/builds/build_gpu && \
	cmake -DZFP_LIBRARY_PREFIX=gpu -DBUILD_EXAMPLES=OFF -DZFP_WITH_CUDA=ON ../.. && \
	cmake --build . --config Release
	mkdir -p ../build/lib ../build/include
	cp -r zfp/builds/build_gpu/lib/* ../build/lib/
	cp -r zfp/include/* ../build/include/zfp_gpu

clean:
	$(MAKE) -C zfp/ clean
	$(MAKE) -C my_tests/ clean
	rm -rf zfp/builds/build_gpu
