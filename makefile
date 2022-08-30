CC=gcc
CFLAGS=-I. -Wall -lpthread -lm
CRYPTO_FLAGS=-I/usr/local/ssl/include -L/usr/local/ssl/lib64 -Wall -lpthread -lm -lcrypto
SDIR = src
SRCS := $(wildcard $(SDIR)/*.c $(SDIR)/utility/*.c)

SRCSBASE := $(filter-out $(SDIR)/Test.c $(SDIR)/utility/Balancer.c, $(SRCS))
SRCSTEST := $(filter-out $(SDIR)/main.c $(SDIR)/utility/Balancer.c, $(SRCS))

bin/aStar.exe: $(SRCSBASE)
	$(CC) -o $@ $^ $(CFLAGS)

debug: $(SRCSBASE)
	$(CC) -o bin/aStar.exe $^ $(CFLAGS) -DDEBUG

time: $(SRCSBASE)
	$(CC) -o bin/aStar.exe $^ $(CFLAGS) -DTIME

kdf: $(SRCS)
	$(CC) -o bin/aStar.exe $^ $(CRYPTO_FLAGS)

test: $(SRCSTEST)
	$(CC) -o bin/test.exe $^ $(CFLAGS) -DANALYTICS
