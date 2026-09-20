
CC := gcc
CVERSION := -std=c23
CFLAGS := -Wall -pedantic -Wunused-variable -Wunused-function -g
EXEC := test
###########################################################################
all: $(EXEC)

$(EXEC): test.c md4.o
	$(CC) $(CVERSION) $(CFLAGS) -o $(EXEC) $^

md4.o: md4.c md4.h
	$(CC) $(CVERSION) $(CFLAGS) -c -o $@ $<

.PHONY: clean
clean:
	rm -rf *.o $(EXEC)
