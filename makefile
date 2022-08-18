CC=gcc
CFLAGS=-I. -Wall -lpthread -lm
SDIR = src
SRCS = $(SDIR)/main.c $(SDIR)/Graph.c $(SDIR)/PQ.c $(SDIR)/ST.c $(SDIR)/utility/Item.c $(SDIR)/utility/BitArray.c $(SDIR)/Heuristic.c $(SDIR)/AStar.c
DEF = -DDEBUG

bin/aStar.exe: $(SRCS)
	$(CC) -o $@ $^ $(CFLAGS)

debug: $(SRCS)
	$(CC) -o bin/aStar.exe $^ $(CFLAGS) -DDEBUG

time: $(SRCS)
	$(CC) -o bin/aStar.exe $^ $(CFLAGS) -DTIME