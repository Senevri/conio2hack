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
	fgets(filename, MAX-1, stdin);

	clrscr();
	/* test  */
	gotoxy(1,1);
	clreol();
	printf("%s\n", filename);
	if (data==NULL || data->line==NULL) {
		printf("null error");
		return;
	}
	while (data->next != NULL) {
		clreol();
		wprintf(L"%s", data->line);	
		data = data->next;
	}
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

void edit_mode(){
	clrscr();
	_setcursortype(20);
	//ctrl or modal?
	enum { KEY_ESC = 27, KEY_ENTER = '\r', KEY_BACKSPACE = 8, 
		CTRL_S = 19, CTRL_P = 16};
	int i=0;
	wchar_t buf[80];
	int val;
	int tmpx, tmpy;

	struct list *l = init_node();
	gotoxy(1,1);
	printf("Editor\n");
	textcolor(LIGHTGRAY);	

	//gotoxy(1,2);
	struct list *head = l;
	//struct list * tmp = head;
	while((val = getch())!=KEY_ESC){
		buf[i]=(wchar_t)val;		
		printf("%c", buf[i]);
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
				// append_unsaved_buffer(l, buf)
					i=0;
					store_and_next_node(l, buf);
					l=l->next;
				}

				//fix dbg line
				gotoxy(10,1); clreol();
				gotoxy(1, tmpy);
				break;
			case KEY_BACKSPACE:
				//this isn't nice if arrow keys work...
				clreol();
				break;
			case CTRL_S:
				//rewind save key
				if (i!=0){ 
					buf[i-1] = '\0'; 
				} else {
					l->line[78] = '\0';
				}
				//store current pos
				tmpx = wherex();
				tmpy = wherey();
				gotoxy(1, info.screenheight);
				printf("Save file:");
				clreol();
				if (i!=0){
					// append_unsaved_buffer(l, buf)
					i=0;
					store_and_next_node(l, buf);
					l=l->next;
				}
				save_file(head);
				gotoxy(1,1);
				printf("Editor\n");
				textcolor(LIGHTGRAY);	
				gotoxy(tmpx, tmpy);
				//val = KEY_ENTER;
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

