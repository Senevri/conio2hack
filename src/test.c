//hey macarena!
#include <stdio.h>
#include <stdlib.h>
#include <conio2.h>

void movecursym(int x, int y){
		gotoxy(x, y);
		textcolor(YELLOW);
		putchxy(x, y, '@');
		textcolor(LIGHTGRAY);
		_setcursortype(_NOCURSOR);
		gotoxy(x, y); // putting a character increases x
}

int main(int argc, char *argv[])
{
	int val = 0;
	struct text_info info;
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
	while(val != 27){
		textcolor(YELLOW);
		val = getch();
		int cursorx = wherex();
		int cursory = wherey();
		switch (val) {
			case 'a':
				if(cursorx>0) { 
					putchxy(cursorx, cursory, ' ');
					movecursym(cursorx-1, cursory);
				}
				break;
		
			case 's':
				if (cursorx<info.screenwidth) {
					putchxy(cursorx, cursory, ' ');
					movecursym(cursorx, cursory+1);
				}
				break;
			case 'd':
				if (cursory<info.screenheight) {
					putchxy(cursorx, cursory, ' ');
					movecursym(cursorx+1, cursory);
				}
				break;
			case 'w':
				if(cursory>0) {
					putchxy(cursorx, cursory, ' ');
					movecursym(cursorx, cursory-1);
				}
				break;
		}
	}
	/* restore the original screen */
	puttext( 1, 1, info.screenwidth, info.screenheight, screen_buffer );
	normvideo();
	gotoxy( cursor_pos_x, cursor_pos_y );

	free( screen_buffer );
	return 0;
}
