CC=gcc
BUILD=./build
INCLUDE=./include
SOURCE=./src
CFLAGS=-Wall -Wno-unused-function

ifdef D
	CFLAGS += -ggdb
endif

.PHONY: lexer parser plotter default all clean

default: plotter

all: dirs lexer parser plotter

$(BUILD)/lexer.o: $(SOURCE)/lexer.c $(SOURCE)/utils/basic_utils.c $(INCLUDE)/lexer.h
	$(CC) $(CFLAGS) -I./$(INCLUDE) -c $(SOURCE)/lexer.c -o $(BUILD)/lexer.o

$(BUILD)/parser.o: $(SOURCE)/parser.c $(SOURCE)/utils/basic_utils.c $(INCLUDE)/parser.h $(BUILD)/lexer.o
	$(CC) $(CFLAGS) -I./$(INCLUDE) -c $(SOURCE)/parser.c -o $(BUILD)/parser.o

$(BUILD)/slider.o: $(SOURCE)/ui/slider.c $(INCLUDE)/slider.h
	$(CC) $(CFLAGS) -I./$(INCLUDE) -c $(SOURCE)/ui/slider.c -o $(BUILD)/slider.o

# Build lexer.c as main program
lexer: $(BUILD)/lexer
$(BUILD)/lexer: $(SOURCE)/lexer.c $(SOURCE)/utils/basic_utils.c $(INCLUDE)/lexer.h
	$(CC) $(CFLAGS) -I./$(INCLUDE) $(SOURCE)/lexer.c -o $(BUILD)/lexer -DLEXER_MAIN -lm

# Build parser.c as main program
parser: $(BUILD)/parser
$(BUILD)/parser: $(SOURCE)/parser.c $(SOURCE)/utils/basic_utils.c $(INCLUDE)/parser.h $(BUILD)/lexer.o
	$(CC) $(CFLAGS) -I./$(INCLUDE) $(SOURCE)/parser.c $(BUILD)/lexer.o -o $(BUILD)/parser -DPARSER_MAIN -lm

plotter: $(BUILD)/plotter
$(BUILD)/plotter: $(SOURCE)/plotter.c $(SOURCE)/utils/vector_utils.c $(INCLUDE)/parser.h $(BUILD)/lexer.o $(BUILD)/parser.o $(BUILD)/slider.o
	$(CC) $(CFLAGS) -I./$(INCLUDE) $(SOURCE)/plotter.c $(BUILD)/lexer.o $(BUILD)/parser.o $(BUILD)/slider.o -o $(BUILD)/plotter -lm -lraylib

dirs:
	mkdir -p $(BUILD)

clean:
	rm -rf $(BUILD)/*
