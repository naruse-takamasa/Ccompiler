#!/bin/bash
cat <<EOF | gcc -xc -c -o tmp2.o -
int ret3() { return 3; }
int ret5() { return 5; }
int add(int x, int y) { return x+y; }
int sub(int x, int y) { return x-y; }
int add6(int a, int b, int c, int d, int e, int f) {
  return a+b+c+d+e+f;
}
EOF

try() {
  expected="$1"
  input="$2"

  ./SverigeCC "$input" > tmp.s
  #gcc -o tmp tmp.s
  gcc -static -o tmp tmp.s tmp2.o
  ./tmp
  actual="$?"

  if [ "$actual" = "$expected" ]; then
    echo "$input => $actual"
  else
    echo "$input => $expected expected, but got $actual"
    exit 1
  fi
}
try 7 'main() { return add2(3,4); } add2(x,y) { return x+y; }'
try 1 'main() { return sub2(4,3); } sub2(x,y) { return x-y; }'
try 55 'main() { return fib(9); } fib(x) { if (x<=1) return 1; return fib(x-1) + fib(x-2); }'
try 0 'main() { return 0; }'
try 42 'main() { return 42; }'
try 21 'main() { return 5+20-4; }'
try 41 'main() { return  12 + 34 - 5 ; }'
try 47 'main() { return 5+6*7; }'
try 15 'main() { return 5*(9-6); }'
try 4 'main() { return (3+5)/2; }'
try 10 'main() { return -10+20; }'
try 10 'main() { return - -10; }'
try 10 'main() { return - - +10; }'

try 0 'main() { return 0==1; }'
try 1 'main() { return 42==42; }'
try 1 'main() { return 0!=1; }'
try 0 'main() { return 42!=42; }'

try 1 'main() { return 0<1; }'
try 0 'main() { return 1<1; }'
try 0 'main() { return 2<1; }'
try 1 'main() { return 0<=1; }'
try 1 'main() { return 1<=1; }'
try 0 'main() { return 2<=1; }'

try 1 'main() { return 1>0; }'
try 0 'main() { return 1>1; }'
try 0 'main() { return 1>2; }'
try 1 'main() { return 1>=0; }'
try 1 'main() { return 1>=1; }'
try 0 'main() { return 1>=2; }'

try 3 'main() { a=3; return a; }'
try 8 'main() { a=3; z=5; return a+z; }'

try 1 'main() { return 1; 2; 3; }'
try 2 'main() { 1; return 2; 3; }'
try 3 'main() { 1; 2; return 3; }'

try 3 'main() { foo=3; return foo; }'
try 8 'main() { foo123=3; bar=5; return foo123+bar; }'

try 3 'main() { if (0) return 2; return 3; }'
try 3 'main() { if (1-1) return 2; return 3; }'
try 2 'main() { if (1) return 2; return 3; }'
try 2 'main() { if (2-1) return 2; return 3; }'

try 3 'main() { {1; {2;} return 3;} }'

try 10 'main() { i=0; while(i<10) i=i+1; return i; }'
try 55 'main() { i=0; j=0; while(i<=10) {j=i+j; i=i+1;} return j; }'

try 55 'main() { i=0; j=0; for (i=0; i<=10; i=i+1) j=i+j; return j; }'
try 3 'main() { for (;;) return 3; return 5; }'

try 3 'main() { return ret3(); }'
try 5 'main() { return ret5(); }'
try 8 'main() { return add(3, 5); }'
try 2 'main() { return sub(5, 3); }'
try 21 'main() { return add6(1,2,3,4,5,6); }'

try 32 'main() { return ret32(); } ret32() { return 32; }'

echo OK