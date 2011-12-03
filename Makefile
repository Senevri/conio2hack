CONIO2 	:= ext/conio2
INCLUDE := -I$(CONIO2)
SRCS 	:= src/main.c src/edit.c src/hack.c
GCC		:= g:\mingw\bin\gcc.exe

.PHONY: all

all:
	$(GCC) -std=c99 $(SRCS) $(CONIO2)/conio.c $(INCLUDE) -o bin/test.exe -Wall
