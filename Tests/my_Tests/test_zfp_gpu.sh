#!/usr/bin/env bash
# test_zfp.sh
# CubismZ
#
# Copyright 2018 ETH Zurich. All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree.
#
set -x #echo on

rm compressed.cz recon.h5 recon.xmf ref.cz

# [[ ! -f ../Data/demo.h5 ]] && tar -C ../Data -xJf ../Data/data.tar.xz
# h5file=../Data/demo.h5
h5file=../Data/data_005000-p.h5

if [ -z ${1+x} ]
then
	echo "setting err=0.005"
	err=0.005
else
	err=$1
	if [ "$1" -eq "-1" ]; then
		echo "setting err=0.005"
		err=0.005
	fi
	shift
fi

nproc=1
if [ ! -z ${1+x} ]
then
    nproc=$1; shift
fi

bs=256
ds=512
nb=$(echo "$ds/$bs" | bc)

# rm -f tmp.cz

# check if reference file exists, create it otherwise
# if [ ! -f ref.cz ]
# then
#     ./genref.sh
# fi

export OMP_NUM_THREADS=$nproc
# mpirun -n 1 ../../Tools/bin/zfp/hdf2cz -bpdx $nb -bpdy $nb -bpdz $nb -sim io -h5file $h5file -czfile tmp.cz -threshold $err
# mpirun -n 1 ../../Tools/bin/zfp_gpu/hdf2cz -bpdx $nb -bpdy $nb -bpdz $nb -sim io -h5file $h5file -czfile tmp.cz -threshold $err

mpi_procs=1
bpdx=$(( nb / mpi_procs ))


########## GPU ##########

# compress
mpirun -n $mpi_procs ../../Tools/bin/zfp_gpu/hdf2cz -nprocx $mpi_procs -nprocy 1 -nprocz 1 -bpdx $bpdx -bpdy $nb -bpdz $nb -sim io -h5file $h5file -czfile compressed.cz -threshold 5


# reference file
mpirun -n 1 ../../Tools/bin/default_gpu/hdf2cz -bpdx $bpdx -bpdy $nb -bpdz $nb -h5file $h5file -czfile ref.cz
mpirun -n 1 ../../Tools/bin/zfp_gpu/cz2diff -czfile1 compressed.cz  -czfile2 ref.cz


# # file reconstruction
# mpirun -n 1 ../../Tools/bin/zfp_gpu/cz2hdf -czfile compressed.cz -h5file recon
# h5diff -r -p 0.005 $h5file recon.h5




########## CPU ##########

# bs=32
# nb=$(echo "$ds/$bs" | bc)

# # compress
# mpirun -n 1 ../../Tools/bin/zfp/hdf2cz -bpdx $nb -bpdy $nb -bpdz $nb -sim io -h5file $h5file -czfile compressed.cz -threshold $err


# # reference file
# mpirun -n 1 ../../Tools/bin/default/hdf2cz -bpdx $nb -bpdy $nb -bpdz $nb -h5file $h5file -czfile ref.cz
# mpirun -n 1 ../../Tools/bin/zfp/cz2diff -bpdx $nb -bpdy $nb -bpdz $nb -czfile1 compressed.cz  -czfile2 ref.cz


# # file reconstruction
# mpirun -n 1 ../../Tools/bin/zfp/cz2hdf -czfile compressed.cz -h5file recon
# h5diff -r -p 0.005 $h5file recon.h5




