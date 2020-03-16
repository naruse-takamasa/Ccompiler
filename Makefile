CFLAGS=-std=c11 -g -static

SverigeCC: SverigeCC.c

test: SverigeCC
	./test.sh

clean:
	rm -f SverigeCC *.o *~ tmp*

.PHONY: test clean