#!/bin/sh
set +e

mkdir -p build
cd ./build

cmake ../
make

MAKE_EXIT_CODE=$?

cd ../

exit $?
