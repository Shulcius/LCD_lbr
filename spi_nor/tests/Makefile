CC ?= gcc
CFLAGS ?= -std=c99 -Wall -Wextra -Werror -O0 -g
INCLUDES = -I../include -I.

SRCS = ../src/spi_nor.c mock_hal.c test_spi_nor.c
BIN = test_spi_nor

.PHONY: all test clean

all: $(BIN)

$(BIN): $(SRCS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $(SRCS)

test: $(BIN)
	./$(BIN)

clean:
	rm -f $(BIN)
