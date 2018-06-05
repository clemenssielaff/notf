#!/bin/sh

# HOWTO:
# 0: Make sure you have gcov and lcov installed
# 1: Compile test build with gcc using "-fprofile-arcs -ftest-coverage" c++ flags
# 2: Run the test to generate coverage
# 3: Copy this file into the build directory (for example ${NOTF}/build/gcc/debug) and execute it

# generate coverage.info file from the CMake build
lcov --directory "${PWD}/CMakeFiles/notf-test.dir/" --capture --output-file coverage.info

# remove all output but the one generated from the src/ subdirecty
lcov --extract coverage.info "*/notf/src/*" --output-file coverage.info

# create HTML report in ./coverage directory
rm -rf ./coverage
genhtml -o ./coverage -t "notf Test Coverage Report" coverage.info 

