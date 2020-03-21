#!/bin/bash
try() {
  expected="$1"
  input="$2"

  ./SverigeCC "$input" > tmp.s
  gcc -o tmp tmp.s
  ./tmp
  actual="$?"

  if [ "$actual" = "$expected" ]; then
    echo "$input => $actual"
  else
    echo "$input => $expected expected, but got $actual"
    exit 1
  fi
}

try 3 "a = 2; b = 1; a + b;"
try 3 "1 + 2;"
try 44 "44;"
try 24 "a = 3; b = 2; c = 5; a + (b + c) * a;"
try 10 "return 10;"
try 15 "return 15; return 10;"
try 15 "a = 3; _b = 4; now = 1; return a * (_b + now);"
echo OK