#!/bin/sh


# This is how you generate a report with llvm-cov
# Copy this file into the build directory, containing the tests executable and a file "default.profraw".

./tests
llvm-profdata-10 merge ./default.profraw -sparse --output default.profdata
llvm-cov-10 show -instr-profile=default.profdata -format=html -ignore-filename-regex="/notf/(thirdparty|tests)/" -Xdemangler=c++filt -line-coverage-lt=1 ./tests > coverage.html
cp ./coverage.html ../../../../../INSTALL
