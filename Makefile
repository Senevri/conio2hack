CONIO2  := ext/conio21
INCLUDE := -I$(CONIO2) -Isrc
SRCS   := src/main.c src/edit.c src/hack.c
GCC    := gcc
MKDIR  := mkdir -p
OUT_DIR := bin

.PHONY: directories all clean

all: directories
	$(GCC) -std=c99 $(SRCS) $(CONIO2)/conio.c $(INCLUDE) -o $(OUT_DIR)/test.exe -Wall

directories:
	$(MKDIR) $(OUT_DIR)

clean:
	-@rm -rf bin *~