#!/bin/bash

set echo on

SRC_DIR=$PWD

cd build

# shaders
cp -r $SRC_DIR/engine/shaders .

# assets
cp -r $SRC_DIR/application/assets .