#!/bin/sh
cd $1
lcov --base-directory . --directory . --capture --output-file coverage.info
lcov --remove coverage.info "/usr*" --remove coverage.info "/thirdparty*" --remove coverage.info "/test*" --output-file coverage.info # remove output for external libraries
rm -rf ./coverage
genhtml -o ./coverage -t "notf Test Coverage Report" coverage.info


