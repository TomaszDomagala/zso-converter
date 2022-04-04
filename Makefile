all: converter

ROOT_DIR = $(shell pwd)
CC=gcc
CFLAGS= -Wall -Wextra -O2 -I./src
DEPS = src/prints.h src/fatal.h src/list.h src/funcfile.h src/conv.h src/elffile.h
OBJ = main.o prints.o fatal.o list.o funcfile.o conv.o elffile.o

%.o: src/%.c $(DEPS)
	@$(CC) -c -o $@ $< $(CFLAGS)

converter: $(OBJ)
converter: ## Build the converter
	@echo "Building..."
	@gcc -o $@ $^ $(CFLAGS)
	@cp $(ROOT_DIR)/$@ $(ROOT_DIR)/example
	@cp $(ROOT_DIR)/$@ $(ROOT_DIR)/z1_test

test-64.o: mvp/test-64.c
	gcc -mcmodel=small -fno-pic -c mvp/test-64.c -o test-64.o

test-32.o: mvp/test-64.c
	gcc -m32 -fno-pic -c mvp/test-64.c -o test-32.o

stub32to64.o: stub32to64.s
	as --32 -o stub32to64.o stub32to64.s

newelf.o: converter test-64.o
	./converter test-64.o

mvp.out: newelf.o mvp/test.c
	gcc -m32 -g -no-pie -o mvp.out $^

.PHONY: clean
clean:	## Clean up
	@rm -f *.o test
	@rm -f converter

.PHONY: help
help:
	@awk -F ':|##' '/^[^\t].+?:.*?##/ {printf "\033[36m%-25s\033[0m %s\n", $$1, $$NF}' $(MAKEFILE_LIST)
