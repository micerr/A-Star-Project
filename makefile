CC=gcc
CFLAGS=-I. -Wall
SDIR = src
SRCS = $(SDIR)/main.c $(SDIR)/Graph.c $(SDIR)/PQ.c $(SDIR)/ST.c
DEF = -DDEBUG

bin/aStar.exe: $(SRCS)
	$(CC) -o $@ $^ $(CFLAGS)

debug: $(SRCS)
	$(CC) -o bin/aStar.exe $^ $(CFLAGS) $(DEF)