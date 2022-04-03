#!/bin/bash

cd application

c_files=$(find src -type f -name "*.c")

echo "C Files: " $c_files

output="DarkMatterApp"

compiler_flags="-g -fdeclspec -fPIC -fdiagnostics-absolute-paths -Wall -Wno-missing-braces"
include_flags="-I../engine/include -I../engine/src "
linker_flags="-L../bin/ -lDarkMatter -Wl,-rpath,."
defines="-DDM_DEBUG"

echo "Building $output..."

clang $c_files $compiler_flags -o ../bin/$output $defines $include_flags $linker_flags

