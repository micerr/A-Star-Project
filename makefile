CC=gcc
CFLAGS=-I. -Wall -lpthread -lm
SDIR = src
SRCS = $(SDIR)/*.c $(SDIR)/utility/*.c

bin/aStar.exe: $(SRCS)
	$(CC) -o $@ $^ $(CFLAGS)

debug: $(SRCS)
	$(CC) -o bin/aStar.exe $^ $(CFLAGS) -DDEBUG

time: $(SRCS)
	$(CC) -o bin/aStar.exe $^ $(CFLAGS) -DTIME

psearch: $(SRCS)
	$(CC) -o bin/aStar.exe $^ $(CFLAGS) -DPARALLEL_SEARCH -DTIME