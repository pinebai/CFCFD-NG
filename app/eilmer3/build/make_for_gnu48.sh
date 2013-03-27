#!/bin/bash
# script to set the compiler environment variables and then build.
# Usage:
# $ ./make_for_gnu48.sh 
# $ ./make_for_gnu48.sh install

export OMPI_CC=gcc-4.8
export OMPI_CXX=g++-4.8
make TARGET=for_openmpi $@
