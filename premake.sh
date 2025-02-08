#!/bin/sh
cd build
# Generate makefiles
premake5 gmake2
# generate compile_commands.json
premake5 ecc
cd ..
