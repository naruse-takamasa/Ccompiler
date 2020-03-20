#!/bin/bash
docker run --rm -v ~/Documents/GitHub/Ccompiler/src:/src -w /src compilerbook chmod a+x test.sh
docker run --rm -v ~/Documents/GitHub/Ccompiler/src:/src -w /src compilerbook make test
docker run --rm -v ~/Documents/GitHub/Ccompiler/src:/src -w /src compilerbook make clean