#!/bin/bash
set -e

swig -DSINGLE_FRAME -I../inc -tcl segment.i
gcc -fPIC -I /usr/include/tcl -I ../inc -DSINGLE_FRAME -c segment_wrap.c 
gcc -shared segment_wrap.o ../build/lib/segmentlib.so -o segment.so -lpthread 
tclsh make-package.tcl > pkgIndex.tcl
stc segmenter.swift
