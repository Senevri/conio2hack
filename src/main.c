//hey macarena!
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio2.h>

#include "edit.h"

enum {INPUT, COMMAND, QUIT};
int mode = INPUT;
struct text_info info;

/* defined in edit.h */
/*struct list{
	int * line;
	struct list * next;
}; */ 

void movecursym(int x, int y){
		gotoxy(x, y);
		textcolor(YELLOW);
		putchxy(x, y, '@');
		textcolor(LIGHTGRAY);
		_setcursortype(_NOCURSOR);
		gotoxy(x, y); // putting a character increases x
}

void edit_mode();
void roguelike();
void eval_command(int cmd, char* params){
	//enums must match order in char array
	enum {EXIT, EDITOR, HACK, HELP};
	struct list *help;
	switch (cmd) {
		case EXIT : //namespace overlap
			printf("=%s", params);
			mode = QUIT;
			//printf("suor komento foo");
			break;
		case EDITOR:
			//printf("suor komento komento ");
			edit_mode();
			break;
		case HACK: 
			roguelike();
			break;
		case HELP:
			// load readme
			help = init_node();
			//edit_mode();
			load_file_by_filename(help, "README.asciidoc");
			break;
		default: 
			printf("ERRORERROR");
	}
	//parse_cmd(params);
}


//accept \0 - terminated strings, scan for command words
void parse_cmd(char* cmd) {
	char *commands[] = {
		"quit",
		"editor",
		"hack",
		"help",
		NULL
	};
	int i = 0;
	gotoxy(1,1);
	clreol();
	char cword[80];
	//printf("end-or-space-seek");
	while ((cmd[i] != '\0') && (cmd[i] != ' ')) {
		cword[i] = cmd[i];
		i++;
	}
	cword[i] = '\0'; //null-terminate
	
		//printf("+");
	printf("=");
	for(int k=0; commands[k] != NULL; k++){
		if (0 == strcmp(cword, commands[k])){
			printf("cmd found");
			eval_command(k, (('\0' == cmd[i]) ? NULL : cmd+(i+1)));
			//cword[j+1] = '\0'; // let's leave this loop
		}	
	}
	
	
}
void debug_show_keycode(int val){
	int x, y;
	x=wherex();
	y=wherey();
	gotoxy(1,1);
	clreol();
	printf("%d", val);

	gotoxy(x,y);
}

void command_mode(struct text_info info) {
	textcolor(YELLOW);
	char cmd[80];
	int pos=0;
	int val;
	int backspace = 0;
	while(COMMAND == mode) {
		val = getche();
		switch (val) {
			case '\t': //TAB: leave command mode
				mode = INPUT;
				break;
			case '\r': //ENTER - exec command;
				cmd[pos] = '\0';
                // TODO: check for aliases
				parse_cmd(cmd);
				gotoxy(1, info.screenheight);
				printf("Command: ");
				clreol();
				pos = 0;
				break;
			case 8: //backspace
				if (pos>0) {
					pos = pos - 2; //backspace and the del'd chara 

					//cmd[pos+1] = '\0';
					clreol();
					backspace = 1;
				}
			default: 
				cmd[pos+backspace] = (char)val;
				backspace=0;
				debug_show_keycode(val);
				pos++;
		}
	}
}


int main(int argc, char *argv[])
{
	enum { MENUHEIGHT = 2, KEY_ESC = 27 };
	int val = 0;
	//struct text_info info;
	struct char_info * screen_buffer;
	int cursor_pos_x, cursor_pos_y;

	/* we want to restore current text attributes on the end of application */
	inittextinfo();

	/* get the info about screen */
	gettextinfo( &info );

	/* save the cursor position */
	cursor_pos_x = info.curx;
	cursor_pos_y = info.cury;

	/* set normal cursor */
	_setcursortype( _SOLIDCURSOR );

	/* first we save the existing screen so we can restore it on end */
	screen_buffer = (struct char_info *) malloc(
			info.screenwidth * info.screenheight * sizeof(struct char_info) );
	gettext( 1, 1, info.screenwidth, info.screenheight, screen_buffer );

	/* clear the screen */
	clrscr();
	printf("hello, world!\n");
	movecursym(info.screenwidth/2, info.screenheight/2);
	int cursorx = wherex();
	int cursory = wherey();
	gotoxy(1, 1);
	while(val != KEY_ESC && mode != QUIT){
		textcolor(YELLOW);
		val = getch();
		// we want to park the cursor to lower left corner while not in use....
		gotoxy(cursorx, cursory);
		if (INPUT==mode) {
			switch (val) {
				case 'a': 
					if(cursorx>1) { 
						putchxy(cursorx, cursory, ' ');
						movecursym(cursorx-1, cursory);
					}
					break;
			
				case 'd': //go right to the edge but no further
					if (cursorx<info.screenwidth) { 
						putchxy(cursorx, cursory, ' ');
						movecursym(cursorx+1, cursory);
					}
					break;
				case 's':
					if (cursory<info.screenheight-MENUHEIGHT) {
						putchxy(cursorx, cursory, ' ');
						movecursym(cursorx, cursory+1);
					}
					break;
				case 'w':
					if(cursory>2) {
						putchxy(cursorx, cursory, ' ');
						movecursym(cursorx, cursory-1);
					}
					break;
			}
		}
		switch (val) {
			case '\t': //should catch tab.
				cursorx = wherex(); cursory = wherey();
				gotoxy(1, info.screenheight); //lowest row
				switch(mode){
					case INPUT:
						mode = COMMAND;
						printf("Command: ");
			
						break;
					case COMMAND:
						printf("neverhere");
						mode = INPUT;
						break;

				
				}

		}
		if (COMMAND==mode) {
			command_mode(info);
			//if(COMMAND == mode) mode = INPUT;
			gotoxy(1, info.screenheight); //lowest row
			printf("Input"); clreol();  
			gotoxy(cursorx, cursory);
		}
		textcolor(LIGHTGRAY);
		cursorx = wherex();
		cursory = wherey();
		gotoxy(1, info.screenheight-1);
		//printf("this should go to bottom of the screen");
		gotoxy(1, 1);
        if (argc ==2 && 0 == strcmp("hack", argv[1])) { // fixme check argument is hack.
            parse_cmd("hack"); // run hack, on return enter command mode
            argc = 1;
            val = 0;
            mode = COMMAND;
        }
	}
	/* restore the original screen */
	puttext( 1, 1, info.screenwidth, info.screenheight, screen_buffer );
	normvideo();
	gotoxy( cursor_pos_x, cursor_pos_y );
	textcolor(LIGHTGRAY);
	free( screen_buffer );
	return 0;
}
