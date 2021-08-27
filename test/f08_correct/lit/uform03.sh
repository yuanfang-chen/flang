#
# Copyright (c) 2019, Advanced Micro Devices, Inc. All rights reserved.
#
# F2008 Compliance Tests: Error stop code - Execution control
#
# Date of Modification: 1st Sep 2019
#
# Tests the F2008 :Unlimited format item - Input/Output feature
# for an integer array
# Shared lit script for each tests. Run bash commands that run tests with make.
# RUN: KEEP_FILES=%keep FLAGS=%flags TEST_SRC=%s MAKE_FILE_DIR=%S/.. bash %S/runmake | tee %t
# RUN: cat %t | FileCheck %S/runmake