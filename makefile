CC=gcc
CFLAGS=-I. -Wall -lpthread -lm
CRYPTO_FLAGS= -Wall -lpthread -lm
SDIR = src
SRCS := $(wildcard $(SDIR)/*.c $(SDIR)/utility/*.c)

SRCSBASE := $(filter-out $(SDIR)/Test.c, $(SRCS))
SRCSTEST := $(filter-out $(SDIR)/main.c, $(SRCS))

bin/AStar: $(SRCSBASE)
	$(CC) -o $@ $^ $(CFLAGS)

debug: $(SRCSBASE)
	$(CC) -o bin/AStar_debug $^ $(CFLAGS) -DDEBUG

time: $(SRCSBASE)
	$(CC) -o bin/AStar_time $^ $(CFLAGS) -DTIME

test: $(SRCSTEST)
	$(CC) -o bin/Test $^ $(CFLAGS) -DANALYTICS
