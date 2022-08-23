CC=gcc
CFLAGS=-I. -Wall -lpthread -lm
SDIR = src
CRYPTO_FLAGS=-I/usr/local/ssl/include -L/usr/local/ssl/lib64 -Wall -lpthread -lm -lcrypto
SRCS = $(SDIR)/*.c $(SDIR)/utility/*.c

bin/aStar.exe: $(SRCS)
	$(CC) -o $@ $^ $(CFLAGS)

debug: $(SRCS)
	$(CC) -o bin/aStar.exe $^ $(CFLAGS) -DDEBUG

time: $(SRCS)
	$(CC) -o bin/aStar.exe $^ $(CFLAGS) -DTIME

kdf: $(SRCS)
	$(CC) -o bin/aStar.exe $^ $(CRYPTO_FLAGS) 