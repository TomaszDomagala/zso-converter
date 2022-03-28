all: converter

ROOT_DIR = $(shell pwd)
CC=gcc
CFLAGS= -Wall -O2 -I./src
DEPS = src/prints.h src/fatal.h
OBJ = main.o prints.o fatal.o

%.o: src/%.c $(DEPS)
	@$(CC) -c -o $@ $< $(CFLAGS)

converter: $(OBJ)
	@echo "Building..."
	@gcc -o $@ $^ $(CFLAGS)
	@cp $(ROOT_DIR)/$@ $(ROOT_DIR)/example
	@cp $(ROOT_DIR)/$@ $(ROOT_DIR)/z1_test

test-64.o:
	gcc -mcmodel=small -fno-pic -c example/test-64.c -o test-64.o

test-32.o:
	gcc -m32 -fno-pic -c example/test-64.c -o test-32.o

.PHONY: help
help:
	@awk -F ':|##' '/^[^\t].+?:.*?##/ {printf "\033[36m%-25s\033[0m %s\n", $$1, $$NF}' $(MAKEFILE_LIST)
