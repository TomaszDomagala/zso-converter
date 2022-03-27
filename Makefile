all: converter

ROOT_DIR = $(shell pwd)

converter: src/main.c
	@echo "Building..."
	@gcc -Wall -O2 -o $@ $<
	@cp $(ROOT_DIR)/$@ $(ROOT_DIR)/example
	@cp $(ROOT_DIR)/$@ $(ROOT_DIR)/z1_test

test-64.o:
	gcc -mcmodel=small -fno-pic -c example/test-64.c -o test-64.o

test-32.o:
	gcc -m32 -fno-pic -c example/test-64.c -o test-32.o

.PHONY: help
help:
	@awk -F ':|##' '/^[^\t].+?:.*?##/ {printf "\033[36m%-25s\033[0m %s\n", $$1, $$NF}' $(MAKEFILE_LIST)
