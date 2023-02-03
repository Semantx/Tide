CC = gcc -o $@ $^
BIN = nav/nav

.PHONY: all
all: $(BIN)

nav/nav: nav/nav.c
	$(CC) -l ncurses

.PHONY: clean
clean:
	rm -f $(BIN)