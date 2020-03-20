CFLAGS=-Wall -std=c11 -g
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

SverigeCC: $(OBJS)
	cc -o SverigeCC $(OBJS) $(LDFLAGS)

$(OBJS): SverigeCC.h

test: SverigeCC
	./test.sh

clean:
	rm -f SverigeCC *.o *~ tmp*