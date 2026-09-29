#!/bin/sh -e
cd "${0%/*}"

code=$PWD/code
mkdir -p build
cd build

echo building
clang -O0 -g -Werror -Wall -Wextra -Wshadow -Wconversion "$code"/renderer.c -o renderer -lm
