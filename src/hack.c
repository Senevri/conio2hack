#include <stdio.h>
#include <stdlib.h>
#include <conio2.h>
#include <string.h>
#include <stdbool.h>


enum { KEY_UP = 72, KEY_DOWN = 80, KEY_LEFT = 75, KEY_RIGHT = 77};	

struct text_info info; //filled in main

struct list{
	wchar_t * line;
	struct list * next;
};

// Any symbol displayed on screen
struct symbol{
	const char * name;
	char character;
	int color;
};

//gameplay related data
struct statistics {
	int atk, def, hp, max_hp;
};

//any creature in game
struct creature{
	char name[255];
	struct symbol sym;
	struct statistics stats;
	int x;
	int y;
	struct creature * next;
};


enum TERRAIN { GRASS, WATER, WALL};	

struct symbol terrain[256] = {
    {"grass", '.', GREEN},
    {"water", '~', BLUE},
    {"wall",  '#', DARKGRAY}
};

void full_refresh(char *screenmap, struct symbol symbols[256]){
	for (int y=0; y < info.screenheight-1; y++){	
		for (int x=0; x < info.screenwidth; x++){
			unsigned char out = screenmap[y*(info.screenwidth+1)+x];
			if(15 < x && x < info.screenwidth-1 && 
					0 < y && y < info.screenheight-2){
				textcolor(symbols[out].color);
				putchxy(x+1, y+1, symbols[out].character);
			} else {
				textcolor(LIGHTGRAY);
				putchxy(x+1, y+1, out);
			}
			//printf("%s", screenmap[y]);
		}
	}
	gotoxy(0,0);
}

struct creature * update_creature_position(struct creature * cr, char* name, int x, int y) {
	struct creature * head = cr;
	while (cr != NULL) {
		if (strcmp(cr->name, name) == 0) {
			cr->x = x; cr->y = y;

		}
		cr = cr->next;
	}
	return head;
}

struct creature* create_creature(const char * name, const char symbol, int color, int x, int y){
	struct creature * c = malloc(sizeof(struct creature));
	strcpy(c->name, name);
	c->sym.character = symbol;
	c->sym.color = color;
	c->x = x;
	c->y = y;
	c->next = NULL;
	return c;
}

void draw_creatures(struct creature* critters){
	struct creature * cr = critters;
	while(NULL!=cr){
		textcolor(cr->sym.color);
		putchxy(cr->x, cr->y, cr->sym.character);
		cr = cr->next;
	}

}

void erase_creatures(struct creature * critters, char * screenmap, struct symbol symbols[256]) {
	struct creature * cr = critters;
	while(NULL!=cr){
		unsigned char out = screenmap[(cr->y-1)*(info.screenwidth+1)+cr->x-1];
		textcolor(symbols[out].color);
		putchxy(cr->x, cr->y, symbols[out].character);
		cr = cr->next;
	}

}


bool movement_blocked(struct creature *player, char * screenmap) {
    char id = screenmap[(player->y-1)*(info.screenwidth+1) + (player->x-1)];
    if (id==WALL) {
        return true;
    }
    return false;
    
}

struct creature * handle_input(int val, struct creature * player, char * screenmap) {
    struct creature tmp = *player;
	switch(val) {
		case KEY_UP:
			if (player->y > 2) {
				player->y -= 1;
			}
			break;
		case KEY_DOWN:
			if (player->y < info.screenheight -2) {
				player->y += 1;
			}
			break;
		case KEY_LEFT:
			if (player->x>17) {
				player->x -= 1;
			}
			break;
		case KEY_RIGHT:
			if (player->x<info.screenwidth -1) {
				player->x += 1;
			}
			break;
		default: 
			break;
	}
    if (movement_blocked(player, screenmap)) {
        memcpy(player, &tmp, sizeof(struct creature));
    }
	return player;
}

void roguelike() {
	enum { KEY_ESC = 27, KEY_ENTER = '\r', KEY_BACKSPACE = 8, 
		CTRL_S = 19, CTRL_P = 16, CTRL_Q = 17, CTRL_B = 2,
		KEY_UP = 72, KEY_DOWN = 80, KEY_LEFT = 75, KEY_RIGHT = 77};	
	

	char screenmap[info.screenheight][info.screenwidth+1]; //y then x
	memset(screenmap, ' ', (info.screenwidth+1)*info.screenheight);

	struct creature * critters = create_creature("playa", '@', LIGHTGRAY, 16+((info.screenwidth-16)/2), info.screenheight/2);
	struct creature * head = critters;

	/* store player for convenience*/
	struct creature * player = critters;

	
	//construct terrain
//	for (int i = 0; i <2; i++){
//		terrain[i].character = symbols[i][0];
//		terrain[i].color = symbols[i][1];
//	}
	for (int i = 3; i <256; i++){
		// zero unused values
		//
		terrain[i].name="undefined";
		terrain[i].character = 0;
		terrain[i].color = 0;

	}
	


	int val; // getch value 
	clrscr();
	char * title[] = {"Roguelike", "Press Any Key"};
	for (int i=0; i!=2; i++){
		gotoxy((info.screenwidth-(strchr(title[i], '\0')-title[i]))/2, -3+i+info.screenheight/2);
		printf("%s", title[i]);
	}
	getch();
	clrscr();
	
	for (int i = 0; i < info.screenheight-1; i++){
		screenmap[i][15] = '|';
		screenmap[i][0] = '*';
		screenmap[i][info.screenwidth-1] = '*';
		screenmap[i][info.screenwidth]='\0';
	}
	for (int i = 0; i < info.screenwidth; i++){
		screenmap[0][i] = '*';
		screenmap[info.screenheight-2][i] = '*';		
	}
	//let's make a lawn
	
	for (int y=1; y < info.screenheight-2; y++) {
		for (int x=16; x < info.screenwidth-1; x++) {
			screenmap[y][x] = (90 < rand() % 100) ? WATER : GRASS;
		}
	}
	// a wall.... 
	//
	
	for (int x=20; x<30;x++){
		screenmap[5][x] = WALL;
		screenmap[15][x] = WALL;
	} 
	for (int y=5; y<15;y++){
		screenmap[y][20] = WALL;
		screenmap[y][30] = WALL;
	} 

	textcolor(WHITE);
	full_refresh(&screenmap[0][0], terrain);
	draw_creatures(head);
	gotoxy(0,0);
	_setcursortype(_NOCURSOR);
	_setcursortype(0);
	while((val=getch())!=CTRL_Q) {
		erase_creatures(head, &screenmap[0][0], terrain);
		player = handle_input(val, player, (char *)screenmap);
		head = update_creature_position(head, player->name, player->x, player->y);
		draw_creatures(head);		
		gotoxy(0,0);
	}
	_setcursortype(_NORMALCURSOR);
	textcolor(YELLOW);
}
