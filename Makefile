CC = gcc
CFLAGS = -Wall -Wextra -g

all: slush

slush: slush.c
	$(CC) $(CFLAGS) -o slush slush.c

clean:
	rm -f slush
