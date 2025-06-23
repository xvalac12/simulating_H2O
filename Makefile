CC=gcc
NAME=proj2
CFLAGS=-std=gnu99 -Wall -Wextra -Werror -pedantic
SEMFLAGS=-lrt -lpthread -lm 

all:
	$(CC) $(CFLAGS) $(NAME).c -o $(NAME) $(SEMFLAGS)

zip:
	zip proj2.zip proj2.c Makefile

clean:
	rm proj2.out proj2
