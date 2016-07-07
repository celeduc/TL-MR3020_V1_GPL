#!/bin/sh
#PRODUCT_N is the name of product you will build

make PID=302001 tpclean
rm -rf gcc-4.3.3
make PID=302001 fakeroot_build
make PID=302001 toolchain_prep
make PID=302001 fs_prep
make PID=302001 302001
