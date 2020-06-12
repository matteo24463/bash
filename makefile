CC=gcc
CFLAGS=-ggdb -Og -Wall -pedantic -Werror -lreadline -std=c11

all: 
	$(CC) main.c -o main $(CFLAGS)
	

