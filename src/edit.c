#include <stdio.h>
#include <stdlib.h>
#include <conio2.h>
#include <string.h>

struct text_info info;

struct list{
	wchar_t * line;
	struct list * next;
};

void save_file(struct list *data){
	enum {MAX = 256};
	char filename[MAX];
	FILE * file;
	char mbstr[160];
	int len;
	//fgets(filename, MAX-1, stdin);
	scanf("%256s", filename);

	if (0==strcmp(filename, "")) return;
	clrscr();
	/* test  */
	gotoxy(1,1);
	clreol();
	printf("%s\n", filename);
	if (data==NULL || data->line==NULL) {
		printf("null error");
		return;
	}
	file = fopen(filename, "w");
	while (data->next != NULL) {
		clreol();
		//debug
		wprintf(L"%s", data->line);
		//write
		len = wcstombs(mbstr, data->line, 160);
		fwrite(mbstr, sizeof(char), len,file);
		data = data->next;
	}
	fputc((wchar_t)0, file);
	fclose(file);
	//print last line
	//printf("%s", data->line);		
}
struct list * init_node() {
	struct list * l;
	l = malloc(sizeof(struct list));
	l->line = malloc(80*sizeof(wchar_t));
	l->line[0] = 0;
	//l->next = malloc(sizeof(struct list));
	l->next = NULL;
	return l;
}
void store_and_next_node(struct list *l, const wchar_t * buf) {
	memcpy(l->line, buf, 80*sizeof(wchar_t)); 
	l->next = init_node();
} 

wchar_t * bitstr_to_str(wchar_t *buf){
	int i=0, j=0;
	int bitpos=0;
	int bits[8] = { 128, 64, 32, 16, 8, 4, 2, 1 };
	wchar_t * out = calloc(80, sizeof(wchar_t));
	while(buf[i]){
		switch (buf[i]){
			case '1':
			case '0':
				out[j] += atoi((const char *)&buf[i])*bits[bitpos];
				bitpos++;
				if (8==bitpos) {
					//out[j]++;
				//	wprintf(L"%d|", out[j]);
					bitpos = 0;
					j++;
				}
				break;
			default:
				break;
		}
		i++;
	}
	out[j+1]='\0';
	return out;
}
wchar_t * str_to_bitstr(wchar_t * buf){
	int i=0;
	wchar_t * out = malloc(9*80*sizeof(wchar_t));
	int bits[8] = { 128, 64, 32, 16, 8, 4, 2, 1 };
	
	//char out[9*80];
	while(buf[i]){
		//we have a number...
		//bits: 1, 2, 4, 8, 16, 32, 64, 128
		int tmp = (int)buf[i];
		for (int j=0; j!=8; j++) {
			if (tmp-bits[j]>=0) {
				out[(j)+i*9] = '1';
				tmp-=bits[j];
			} else {
				out[j+i*9] = '0';
			}
		}
		out[8+i*9] = ' ';
		i++;
	}
	out[i*9] = '\0';
	return out;
}

void rewind_cmd_key(int i, wchar_t *buf, wchar_t *line){
	if (i!=0){ 
		buf[i-1] = '\0'; 
	} else { //last char in line is 'reserved'. 
		line[78] = '\0';
	}
}

void edit_mode(){
	clrscr();
	_setcursortype(20);
	//ctrl or modal?
	enum { KEY_ESC = 27, KEY_ENTER = '\r', KEY_BACKSPACE = 8, 
		CTRL_S = 19, CTRL_P = 16, CTRL_B = 2, CTRL_D = 4,
		KEY_UP = 72, KEY_DOWN = 80, KEY_LEFT = 75, KEY_RIGHT = 77};
	int i=0;
	wchar_t buf[80];
	int val;
	int tmpx, tmpy;
	wchar_t * bits;

	struct list *l = init_node();
	gotoxy(1,1);
	printf("Editor\n");
	textcolor(LIGHTGRAY);	

	//gotoxy(1,2);
	struct list *head = l;
	//struct list * tmp = head;
	while((val = getch())!=KEY_ESC){
		buf[i]=(wchar_t)val;		
		//printf("%c", buf[i]);
		i++;
		buf[i]='\0';
		//if (i<79 && l==NULL) l=malloc(sizeof(struct list));
		if (i==79) {			
			i=0;
			store_and_next_node(l, buf);
			l=l->next;
			memset(buf, 0, 80*sizeof(wchar_t));
		}
		switch (val){
			case KEY_ENTER:
				gotoxy(1, (wherey()+1));
				tmpy = wherey();
				if (i<79) {
					buf[i]='\n';
					buf[i+1]='\0';
				}
				if (i!=0){
					i=0;
					store_and_next_node(l, buf);
					l=l->next;
				}
				//fix dbg line
				gotoxy(10,1); clreol();
				gotoxy(1, tmpy);
				break;
			case KEY_BACKSPACE:
				//correct way: delete entry by shifting rest of line backwards by 1.
				rewind_cmd_key(i-1, buf, l->line);
				//wprintf(L"%c", (wchar_t)val);
				i-=2;
				wcscpy(&buf[i], &buf[i+2]);
				gotoxy(1, wherey());
				clreol();
				wprintf(L"%s", buf);
				
				//TBD
				break;
			case CTRL_S:
				//rewind save key
				rewind_cmd_key(i, buf, l->line);
				//store current pos
				tmpx = wherex();
				tmpy = wherey();
				gotoxy(1, info.screenheight);
				printf("Save file:");
				clreol();
				if (i!=0){
					i=0;
					store_and_next_node(l, buf);
					l=l->next;
				}
				save_file(head);
				gotoxy(1,1);
				textcolor(YELLOW);
				printf("Editor");
				clreol();
				textcolor(LIGHTGRAY);	
				gotoxy(tmpx, tmpy);
				//val = KEY_ENTER;
				val=0;
				break;
			case CTRL_B: //print buffer in binary
				rewind_cmd_key(i, buf, l->line);
				//goto next line, print bitstring
				gotoxy(1, tmpy+1);
				bits = str_to_bitstr(buf);
				wprintf(L"%s", bits);
				free(bits);
				gotoxy(1, wherey()+1);
				if (i!=0){
					i=0;
					store_and_next_node(l, buf);
					l=l->next;
				}		
				break;
			case CTRL_D: //print characters of bitstream
				rewind_cmd_key(i, buf, l->line);
				//goto next line, print bitstring
				gotoxy(1, tmpy+1);
				bits = bitstr_to_str(buf);
				wprintf(L"%s", bits);
				free(bits);
				gotoxy(1, wherey()+1);
				if (i!=0){
					i=0;
					store_and_next_node(l, buf);
					l=l->next;
				}		
				break;
			case KEY_UP:
			case KEY_DOWN:
			case KEY_LEFT:
			case KEY_RIGHT:
				rewind_cmd_key(i-1, buf, l->line);
				gotoxy(1, wherey());
				clreol();
				wprintf(L"%s", buf);
				i--;
				val=0;
				break;
			case CTRL_P:
				/*tmpx = wherex(); tmpy = wherey();
				gotoxy(1, 10);
				while (tmp->next != NULL) {
					wprintf(L"%s\n",tmp->line);
					tmp=tmp->next;
				}
				tmp = head;
				if (i!=0){	
					memcpy(l->line, buf, i*sizeof(wchar_t));
					wprintf(L"%s",tmp->line);
				}
				gotoxy(tmpx, tmpy);*/
				break;				
			default:
				if (0 != val) wprintf(L"%c", (wchar_t)val);
				//dbg code
				tmpx = wherex();
				tmpy = wherey();
				gotoxy(10,1);
				wprintf(L"%2d %s", i, buf);
				gotoxy (tmpx, tmpy);
				break;
		};
	}
	while(head->next!=NULL){
		free(head->line);
		head=head->next;
	}	
	free(l);
	_setcursortype(_NOCURSOR);
	textcolor(YELLOW);
}

