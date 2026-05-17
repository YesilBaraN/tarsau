# tarsau derlemesi
CC = gcc
CFLAGS = -Wall -Wextra -std=c11
HEDEF = tarsau

all: $(HEDEF)

$(HEDEF): tarsau.c
	$(CC) $(CFLAGS) -o $(HEDEF) tarsau.c

clean:
	rm -f $(HEDEF)
