CC=gcc
CFLAGS=-Wall
BUILD=./build
DECMOC_INCLUDE=../include
DECMOC_SOURCE=../src
DECMOC_BUILD=../build

.PHONY: default all lexer_test parser_test clean 

default: lexer_test

all: dirs lexer_test parser_test

lexer_test: $(BUILD)/lexer_test
$(BUILD)/lexer_test: lexer_test.c test.h $(DECMOC_SOURCE)/lexer.c
	$(CC) $(CFLAGS) lexer_test.c -lm -o $(BUILD)/lexer_test

parser_test: $(BUILD)/parser_test
$(BUILD)/parser_test: parser_test.c test.h $(DECMOC_BUILD)/parser.o
	$(CC) $(CFLAGS) -I$(DECMOC_INCLUDE) parser_test.c $(DECMOC_BUILD)/parser.o -lm -o $(BUILD)/parser_test

dirs:
	mkdir -p $(BUILD)

clean:
	rm -rf $(BUILD)/*
