#!/bin/bash
export PATH=/home/deck/simpsons-recomp/tools/clang20/bin:$PATH
export LD_LIBRARY_PATH=/home/.steamos/offload/nix/store/ah4525ca553drv47jhvgpl9sl87i7a1d-libxml2-2.13.8/lib:$LD_LIBRARY_PATH
cd /home/deck/simpsons-recomp/simpsons/out/build/linux-amd64-relwithdebinfo
ninja -j3 -k 0
