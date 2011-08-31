CONIO2 	:= ext/conio2
INCLUDE := -I$(CONIO2)
SRCS 	:= src/main.c src/edit.c

.PHONY: all


all:
	gcc -std=c99 $(SRCS) $(CONIO2)/conio.c $(INCLUDE) -o bin/test.exe -Wall
