CC=gcc
CFLAGS=-I. -Wall -lpthread -lm
SDIR = src
SRCS = $(SDIR)/main.c $(SDIR)/Graph.c $(SDIR)/PQ.c $(SDIR)/ST.c $(SDIR)/Heuristic.c

bin/aStar.exe: $(SRCS)
	$(CC) -o $@ $^ $(CFLAGS)

debug: $(SRCS)
	$(CC) -o bin/aStar.exe $^ $(CFLAGS) -DDEBUG

time: $(SRCS)
	$(CC) -o bin/aStar.exe $^ $(CFLAGS) -DTIME