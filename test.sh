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

try 3 1+2
try 42 42
try 45 " 12 + 13 + 20 "
try 47 '5+6*7'
try 15 '5 * (9 - 6)'
try 4 '(3+5)/2'
try 5 "-3 + 2 * 4"

echo OK