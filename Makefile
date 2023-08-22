CFLAGS = -g -std=gnu11
LIBS = -pthread -lm
SOURCES = main.c scheduler.c interface.c
OUT = proj2

.DEFAULT_GOAL := all

all:
	gcc $(CFLAGS) $(SOURCES) $(LIBS) -o $(OUT)
clean:
	rm -f $(OUT)
