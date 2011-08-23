CONIO2 := ext/conio2
INCLUDE := -I$(CONIO2)

.PHONY: all


all:
	gcc src/test.c $(CONIO2)/conio.c $(INCLUDE) -o bin/test.exe -Wall
