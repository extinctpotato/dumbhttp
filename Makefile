CC = gcc
CFLAGS = -Wall -pthread
BIN_DIR = ./bin/
SRC_DIR = ./src/

PROGS = dumbhttpd.o hash.o
LIST = $(addprefix $(BIN_DIR), $(PROGS))

.PHONY: clean

all: $(BIN_DIR)dumbhttpd 

clean:
	rm -rf $(BIN_DIR)

$(BIN_DIR)%.o: $(SRC_DIR)%.c
	@echo "making $@"
	mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BIN_DIR)hash: $(BIN_DIR)hash.o
	$(CC) $(CFLAGS) -o $@ $(SRC_DIR)hash_cli.c $<

$(BIN_DIR)dumbhttpd: $(LIST)
	$(CC) $(CFLAGS) -o $@ $^
