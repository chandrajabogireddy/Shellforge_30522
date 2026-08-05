CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -Iinclude

all:
	$(CC) $(CFLAGS) src/history.c src/main.c -lreadline -o shellforge
