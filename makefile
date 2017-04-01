# Michael Girbino -- mjg159
# EECS 338 HW7: Baboon Crossing - POSIX threads implementation

CC = gcc
CFLAGS = -std=c11 -ggdb
OUT = baboon_xing_threads.o
SRC = baboon_xing_threads.c

all:	processes

processes:
	$(CC) -o $(OUT) $(SRC) $(CFLAGS)
	chmod 755 $(OUT)

clean:
	rm -f $(OUT)
