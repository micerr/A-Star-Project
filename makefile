CC=gcc
CFLAGS=-I. -Wall -lpthread -lm
SDIR = src
SRCS := $(wildcard $(SDIR)/*.c $(SDIR)/utility/*.c)

SRCSBASE := $(filter-out $(SDIR)/Test.c, $(SRCS))
SRCSTEST := $(filter-out $(SDIR)/main.c, $(SRCS))

bin/aStar.exe: $(SRCSBASE)
	$(CC) -o $@ $^ $(CFLAGS)

debug: $(SRCSBASE)
	$(CC) -o bin/aStar.exe $^ $(CFLAGS) -DDEBUG

time: $(SRCSBASE)
	$(CC) -o bin/aStar.exe $^ $(CFLAGS) -DTIME

test: $(SRCSTEST)
	$(CC) -o bin/test.exe $^ $(CFLAGS) -DANALYTICS