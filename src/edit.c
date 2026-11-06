#include <stdio.h>
#include <stdlib.h>
#include <conio2.h>
#include <string.h>
#include <wchar.h>
#include <errno.h>

#include "edit.h"

struct text_info info;

//end header part
int save_file(struct list *data){
    enum {MAX = 256};
    char filename[MAX];
    FILE * file;
    char mbstr[160];
    int len;
    
    if (data == NULL || data->line == NULL) {
        printf("Error: Invalid data structure\n");
        return -1;
    }
    
    printf("Enter filename to save: ");
    if (fgets(filename, MAX, stdin) == NULL) {
        printf("Error reading filename: %s\n", strerror(errno));
        return -1;
    }
    
    // Remove newline character if present
    filename[strcspn(filename, "\n")] = '\0';
    
    if (strcmp(filename, "") == 0) return -1;
    
    clrscr();
    gotoxy(1,1);
    clreol();
    printf("%s\n", filename);
    
    file = fopen(filename, "w");
    if (file == NULL) {
        printf("Error opening file: %s\n", strerror(errno));
        return -1;
    }
    
    struct list *current = data;
    while (current != NULL) {
        clreol();
        wprintf(L"%s", current->line);
        len = wcslen(current->line);
        wcstombs(mbstr, current->line, len + 1);
        fwrite(mbstr, sizeof(char), len, file);
        current = current->next;
    }
    
    fputc((wchar_t)0, file);
    fclose(file);
    return 1;
}

struct list* load_file(struct list *data){
    enum {MAX = 256};
    char filename[MAX];
    
    if (data == NULL) {
        printf("Error: Invalid data structure\n");
        return NULL;
    }
    
    printf("Enter filename to load: ");
    if (fgets(filename, MAX, stdin) == NULL) {
        printf("Error reading filename: %s\n", strerror(errno));
        return NULL;
    }
    
    // Remove newline character if present
    filename[strcspn(filename, "\n")] = '\0';
    
    if (strcmp(filename, "") == 0) return NULL;
    return load_file_by_filename(data, filename);
}

struct list* load_file_by_filename(struct list* data, const char* filename) {
    FILE * file;
    wchar_t buf[80];
    
    if (data == NULL) {
        printf("Error: Invalid data structure\n");
        return NULL;
    }
    
    // Free existing data
    struct list *current = data;
    while (current->next != NULL) {
        struct list *tmp = current;
        free(current->line);
        current = current->next;
        free(tmp);
    }
    
    data = init_node();
    
    clrscr();
    gotoxy(1,1);
    clreol();
    printf("%s\n", filename);
    
    file = fopen(filename, "r");
    if (file == NULL) {
        printf("Error opening file: %s\n", strerror(errno));
        return NULL;
    }
    
    while (fgetws(buf, 80, file) != NULL) {
        store_and_next_node(data, buf);
        fprintf(stdout, "%ls", buf);
    }
    
    fclose(file);
    return data;
}

struct list * init_node() {
    struct list * l = malloc(sizeof(struct list));
    if (l == NULL) {
        printf("Error: Failed to allocate memory for list node\n");
        return NULL;
    }
    
    l->line = malloc(80 * sizeof(wchar_t));
    if (l->line == NULL) {
        printf("Error: Failed to allocate memory for line\n");
        free(l);
        return NULL;
    }
    
    l->line[0] = 0;
    l->next = NULL;
    return l;
}

void store_and_next_node(struct list *l, const wchar_t * buf) {
    if (l == NULL) return;
    
    wcscpy(l->line, buf);
    l->next = init_node();
}

wchar_t * bitstr_to_str(wchar_t *buf){
    int i=0, j=0;
    int bitpos=0;
    int bits[8] = { 128, 64, 32, 16, 8, 4, 2, 1 };
    wchar_t * out = calloc(80, sizeof(wchar_t));
    if (out == NULL) {
        printf("Error: Failed to allocate memory for bitstr_to_str\n");
        return NULL;
    }
    
    while(buf[i]){
        switch (buf[i]){
            case '1':
            case '0':
                out[j] += atoi((const char *)&buf[i])*bits[bitpos];
                bitpos++;
                if (8==bitpos) {
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
    if (out == NULL) {
        printf("Error: Failed to allocate memory for str_to_bitstr\n");
        return NULL;
    }
    
    int bits[8] = { 128, 64, 32, 16, 8, 4, 2, 1 };
    
    while(buf[i]){
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
    } else {
        line[78] = '\0';
    }
}

void edit_mode(){
    clrscr();
    _setcursortype(20);
    enum { KEY_ESC = 27, KEY_ENTER = '\r', KEY_BACKSPACE = 8,
        CTRL_S = 19, CTRL_P = 16, CTRL_B = 2, CTRL_D = 4,
        CTRL_O = 15,
        KEY_UP = 72, KEY_DOWN = 80, KEY_LEFT = 75, KEY_RIGHT = 77};
    
    int i=0;
    wchar_t buf[80];
    int val;
    int tmpx, tmpy;
    wchar_t * bits;
    
    struct list *l = init_node();
    if (l == NULL) {
        printf("Error: Failed to initialize editor\n");
        return;
    }
    
    gotoxy(1,1);
    printf("Editor\n");
    textcolor(LIGHTGRAY);
    
    struct list *head = l;
    
    while((val = getch())!=KEY_ESC){
        if (i < 79) {
            buf[i]=(wchar_t)val;
            buf[i+1]='\0';
        }
        
        if (i==79) {
            i=0;
            store_and_next_node(l, buf);
            if (l->next == NULL) {
                printf("Error: Failed to store line\n");
                break;
            }
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
                    if (l->next == NULL) {
                        printf("Error: Failed to store line\n");
                        break;
                    }
                    l=l->next;
                }
                break;
            case KEY_BACKSPACE:
                if (i > 0) {
                    rewind_cmd_key(i-1, buf, l->line);
                    i-=2;
                    wcscpy(&buf[i], &buf[i+2]);
                    gotoxy(1, wherey());
                    clreol();
                    wprintf(L"%s", buf);
                }
                break;
            case CTRL_S:
                rewind_cmd_key(i, buf, l->line);
                tmpx = wherex();
                tmpy = wherey();
                gotoxy(1, info.screenheight);
                printf("Save file: ");
                clreol();
                if (i!=0){
                    i=0;
                    store_and_next_node(l, buf);
                    if (l->next == NULL) {
                        printf("Error: Failed to store line\n");
                        break;
                    }
                    l=l->next;
                }
                if (save_file(head) == -1) {
                    printf("Failed to save file\n");
                }
                gotoxy(1,1);
                textcolor(YELLOW);
                printf("Editor");
                clreol();
                textcolor(LIGHTGRAY);
                gotoxy(tmpx, tmpy);
                break;
            case CTRL_O:
                rewind_cmd_key(i, buf, l->line);
                tmpx = wherex();
                tmpy = wherey();
                gotoxy(1, info.screenheight);
                printf("Load file: ");
                clreol();
                if (i!=0){
                    i=0;
                    store_and_next_node(l, buf);
                    if (l->next == NULL) {
                        printf("Error: Failed to store line\n");
                        break;
                    }
                    l=l->next;
                }
                if (load_file(head) == NULL) {
                    printf("Failed to load file\n");
                }
                gotoxy(1,1);
                textcolor(YELLOW);
                printf("Editor");
                clreol();
                textcolor(LIGHTGRAY);
                gotoxy(tmpx, tmpy);
                break;
            case CTRL_B:
                rewind_cmd_key(i, buf, l->line);
                tmpy = wherey();
                gotoxy(1, tmpy+1);
                bits = str_to_bitstr(buf);
                if (bits != NULL) {
                    wprintf(L"%s", bits);
                    free(bits);
                }
                gotoxy(1, wherey()+1);
                if (i!=0){
                    i=0;
                    store_and_next_node(l, buf);
                    if (l->next == NULL) {
                        printf("Error: Failed to store line\n");
                        break;
                    }
                    l=l->next;
                }
                break;
            case CTRL_D:
                rewind_cmd_key(i, buf, l->line);
                tmpy = wherey();
                gotoxy(1, tmpy+1);
                bits = bitstr_to_str(buf);
                if (bits != NULL) {
                    wprintf(L"%s", bits);
                    free(bits);
                }
                gotoxy(1, wherey()+1);
                if (i!=0){
                    i=0;
                    store_and_next_node(l, buf);
                    if (l->next == NULL) {
                        printf("Error: Failed to store line\n");
                        break;
                    }
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
                break;
            case CTRL_P:
                break;
            default:
                if (0 != val) wprintf(L"%c", (wchar_t)val);
                tmpx = wherex();
                tmpy = wherey();
                gotoxy(10,1);
                wprintf(L"%2d %s", i, buf);
                gotoxy (tmpx, tmpy);
                break;
        };
    }
    
    // Free all allocated memory
    struct list *current = head;
    while (current != NULL) {
        struct list *next = current->next;
        free(current->line);
        free(current);
        current = next;
    }
    
    _setcursortype(_NOCURSOR);
    textcolor(YELLOW);
}