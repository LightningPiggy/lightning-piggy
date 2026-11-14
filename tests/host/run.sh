#!/bin/sh
# Build and run the host tests for Main/HttpResponse.h. Needs only a C++ compiler.
set -e
cd "$(dirname "$0")"
mkdir -p build
${CXX:-c++} -std=c++11 -Wall -Wextra -Werror -fsanitize=address,undefined -g -I shim -o build/test_http_response test_http_response.cpp
./build/test_http_response "$@"
