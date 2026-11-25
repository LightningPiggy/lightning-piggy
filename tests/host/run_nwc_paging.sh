#!/bin/sh
# Builds and runs the NWC paging host test with sanitizers.
set -e
cd "$(dirname "$0")"
OUT="$(mktemp -d)"
trap 'rm -rf "$OUT"' EXIT
c++ -std=c++17 -Wall -Wextra -Werror -g -fsanitize=address,undefined -o "$OUT/test_nwc_paging" test_nwc_paging.cpp
"$OUT/test_nwc_paging"
