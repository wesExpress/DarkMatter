#!/bin/bash

cd bin

c_files=$(find ../application/src -type f -name "*.c")

echo "C Files: " $c_files

output="DarkMatterApp"

compiler_flags="-g -fdeclspec -fPIC -fdiagnostics-absolute-paths -Wall -Wno-missing-braces"
include_flags="-I../engine/include -I../engine/src "
linker_flags="-L. -lDarkMatter -Wl,-rpath,."
defines="-DDM_DEBUG -DDM_IMPORT"

echo "Building $output..."

clang $c_files $compiler_flags -o $output $defines $include_flags $linker_flags