#!/bin/bash

set echo on

mkdir -p bin
cd engine

c_files=$(find src -type f -name "*.c")
objc_files=$(find src -type f -name "*.m")

# stb image
external_files="lib/stb_image/include/stb_image/stb_image.c"

echo "C Files: " $c_files
echo "ObjC Files: " $objc_files

output="DarkMatter"

compiler_flags="-g -shared -fdeclspec -fPIC -Wall -Wno-missing-braces"
include_flags="-Isrc -Ilib/stb_image/include -Ilib/mt19937/include"
linker_flags="-lobjc -framework QuartzCore -framework Cocoa -framework CoreFoundation -framework Metal"
defines="-DDM_DEBUG"

echo "Building $output..."

clang $c_files $objc_files $external_files $compiler_flags -o ../bin/$output.dylib $defines $include_flags $linker_flags