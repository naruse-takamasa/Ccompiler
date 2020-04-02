docker run --rm -v ~/Documents/GitHub/Ccompiler/src:/src -w /src compilerbook gcc -static -o tmp ./tmp.s
docker run --rm -v ~/Documents/GitHub/Ccompiler/src:/src -w /src compilerbook ./tmp
echo "$?"