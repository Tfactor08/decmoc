CC=gcc
BUILD=./build
INCLUDE=./include
SOURCE=./src
CFLAGS=-Wall

.PHONY: default parser plotter clean

default: parser

$(BUILD)/parser.o: $(SOURCE)/parser.c $(SOURCE)/lexer.c $(SOURCE)/utils.c $(INCLUDE)/parser.h
	$(CC) $(CFLAGS) -I./$(INCLUDE) -c $(SOURCE)/parser.c -o $(BUILD)/parser.o $(FLAGS)

parser: $(BUILD)/parser
$(BUILD)/parser: FLAGS = -DPARSER_MAIN
$(BUILD)/parser: $(BUILD)/parser.o
	$(CC) $(CFLAGS) $(BUILD)/parser.o -o $(BUILD)/parser -lm

plotter: $(BUILD)/plotter
$(BUILD)/plotter: $(SOURCE)/plotter.c $(INCLUDE)/parser.h $(BUILD)/parser.o
	$(CC) $(CFLAGS) -I./$(INCLUDE) $(SOURCE)/plotter.c $(BUILD)/parser.o -o $(BUILD)/plotter -lm -lraylib

clean:
	rm -f $(BUILD)/*
