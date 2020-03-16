#!/bin/bash
docker run --rm -v ~/Documents/GitHub/Ccompiler:/Ccompiler -w /Ccompiler compilerbook chmod a+x test.sh
docker run --rm -v ~/Documents/GitHub/Ccompiler:/Ccompiler -w /Ccompiler compilerbook make test
docker run --rm -v ~/Documents/GitHub/Ccompiler:/Ccompiler -w /Ccompiler compilerbook make clean