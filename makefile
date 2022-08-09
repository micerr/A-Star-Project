CC=gcc
CFLAGS=-I. -Wall
SDIR = src
SRCS = $(SDIR)/main.c $(SDIR)/Graph.c $(SDIR)/PQ.c $(SDIR)/ST.c

bin/aStar.exe: $(SRCS)
	$(CC) -o $@ $^ $(CFLAGS)