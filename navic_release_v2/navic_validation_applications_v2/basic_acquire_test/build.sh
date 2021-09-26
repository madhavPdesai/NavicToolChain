#
# !/bin/bash
#
CWD=$(pwd)
# Build steps
# 1.  compile the application, will be mapped from 0x40000000 onwards
cd application
./compile_for_ajit_uclibc.sh
cd $CWD
#
# 2.  prepare the boot image.
#
cd bootstrap
#  arguments
#      application-name  application-mmap-size-in-bytes  app-start-addr  log-mem-size-of-bin-file
../../scripts/prepare_bootstrap_mmap.sh basic_acquire_test 64000 0x40000000 19
cd $CWD
#
# this produces a boot image from 0x0 onwards, which will
# be loaded into flash.
#


