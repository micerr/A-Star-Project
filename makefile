CC=gcc
CFLAGS=-I. -Wall -lpthread -lm
SDIR = src
CRYPTO_FLAGS=-I/usr/local/ssl/include -L/usr/local/ssl/lib64 -Wall -lpthread -lm -lcrypto
SRCS := $(wildcard $(SDIR)/*.c $(SDIR)/utility/*.c)
SRCNOSSL := $(filter-out $(SDIR)/utility/Balancer.c, $(SRCS))

bin/aStar.exe: $(SRCNOSSL)
	$(CC) -o $@ $^ $(CFLAGS)

debug: $(SRCNOSSL)
	$(CC) -o bin/aStar.exe $^ $(CFLAGS) -DDEBUG

time: $(SRCNOSSL)
	$(CC) -o bin/aStar.exe $^ $(CFLAGS) -DTIME

psearch: $(SRCNOSSL)
	$(CC) -o bin/aStar.exe $^ $(CFLAGS) -DPARALLEL_SEARCH -DTIME

kdf: $(SRCS)
	$(CC) -o bin/aStar.exe $^ $(CRYPTO_FLAGS)