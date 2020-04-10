#
# Copyright (c) 2019, Advanced Micro Devices, Inc. All rights reserved.
#
# Adding offload regression testcases
# Date of Creation: 1st July 2019
#
# Removing dependency on -Mx,232,0x10
# Date of modification 1st October 2019
#
#

#!/bin/bash
CC=clang
FC=flang
TARGET_FLAGS="-target x86_64-pc-linux-gnu"
DEVICE_FLAGS="-fopenmp -fopenmp-targets=amdgcn-amd-amdhsa -Xopenmp-target=amdgcn-amd-amdhsa"
MARCH="-march=gfx900"
#XFLAGS="-Mx,232,0x40"
XFLAGS=""
FFLAGS="$TARGET_FLAGS $DEVICE_FLAGS $MARCH $XFLAGS -fuse-ld=ld"
total=0
passed=0
failed=0
$CC check.c -c -o check.o
for file in *.F90
do
  let "total++"
  basename=`basename $file .F90`
  $FC $FFLAGS $file check.o >& /dev/null
  if [ $? -ne 0 ]; then
    let "failed++"
    echo " $file : Compilation failure"
    continue
  fi
  ./a.out
  if [ $? -ne 0 ]; then
    let "failed++"
    echo " $file : Runtime failure"
  else
    let "passed++"
    echo " $file : Test case passed"
  fi
  rm ./a.out
done
rm check.o
rm *.mod
echo ""
echo "########################################################################"
echo ""
echo "Total test cases $total"
echo "Passes test cases $passed"
echo "Failed  test cases $failed"
echo ""
echo "########################################################################"
exit $failed
