#include <stdio.h>
#include <stdlib.h>
#include <conio2.h>
#include <string.h>

struct text_info info; //filled in main

struct list{
	wchar_t * line;
	struct list * next;
};

void roguelike() {
	enum { KEY_ESC = 27, KEY_ENTER = '\r', KEY_BACKSPACE = 8, 
		CTRL_S = 19, CTRL_P = 16, CTRL_B = 2,
		KEY_UP = 72, KEY_DOWN = 80, KEY_LEFT = 75, KEY_RIGHT = 77};
	clrscr();
	char * title = "Roguelike";
	gotoxy((info.screenwidth/2)-sizeof(title)/2, info.screenheight);
	printf(title);
	while((val=getch())!=KEY_ESC) {
	
	}
}
