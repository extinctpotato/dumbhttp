CC = gcc
CFLAGS = -Wall -pthread
BIN_DIR = ./bin/
SRC_DIR = ./src/

PROGS = dumbhttpd
LIST = $(addprefix $(BIN_DIR), $(PROGS))

.PHONY: clean

all: $(LIST)

clean:
	rm -rf $(BIN_DIR)

$(BIN_DIR)%: $(SRC_DIR)%.c
	mkdir -p $(BIN_DIR)
	$(CC) $(INC) $< $(CFLAGS) -o $@
