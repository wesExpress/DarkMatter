#!/bin/bash

SRC_DIR=$PWD

cd build

c_files=$(find $SRC_DIR/application/src -type f -name "*.c")

#echo "C Files: " $c_files

output="DarkMatterApp"

compiler_flags="-g -MD -fdeclspec -fPIC -fdiagnostics-absolute-paths -Wall -Wno-missing-braces"
include_flags="-I$SRC_DIR/engine/include -I$SRC_DIR/engine/src "
linker_flags="-g -L./engine/ -lDarkMatter -Wl,-rpath,@rpath/engine"
defines="-DDM_DEBUG -DDM_IMPORT"

echo "Building $output..."

clang $c_files $compiler_flags -o $output $defines $include_flags $linker_flags

install_name_tool -add_rpath @executable_path/engine $output