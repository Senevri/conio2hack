#include <stdio.h>
#include <stdlib.h>
#include <conio2.h>
#include <string.h>
#include <wchar.h>
#include <errno.h>
#include <locale.h> // Add this for setlocale

#include "edit.h"

struct text_info info;

#define MAX_FILENAME_LENGTH 256
#define MAX_LINE_LENGTH 80

// Forward declarations
wchar_t *bitstr_to_str(wchar_t *buf);
void gotoxy(int x, int y);
void clear_buffer(wchar_t *buf);
void scroll_screen(struct list *l, struct cursor c, int amount);
int add_extra_buffer(struct list *l, wchar_t *buf, int i);
void dump_list(struct list *l);
void reset_header();
void shift_buffer(wchar_t *buf, int i, int direction);

void shift_buffer(wchar_t *buf, int i, int direction) {
    wchar_t *tmp = malloc(MAX_LINE_LENGTH * sizeof(wchar_t));
    wcscpy(tmp, buf + i);
    wcsncpy(buf + i + direction, tmp, MAX_LINE_LENGTH - i);
    free(tmp);
}

void reset_header() {
    gotoxy(1,1);
    textcolor(YELLOW);
    printf("Editor");
    clreol();
    textcolor(LIGHTGRAY);
}

void dump_list(struct list *l) {
    struct list *tmp = l;
    while (tmp != NULL) {
        printf("%ls\n", tmp->line);
        tmp = tmp->next;
    }
}

void clear_buffer(wchar_t *buf) {
    if (buf == NULL) {
        return;
    }

    for (int i = 0; i < MAX_LINE_LENGTH; i++) {
        buf[i] = L'\0';
    }

}


void restore_screen(struct list *l, struct cursor c)
{
    clrscr();
    gotoxy(1,2);
    struct list *tmp = l;
    while (tmp != NULL) {
        printf("%ls\n", tmp->line);
        tmp = tmp->next;
    }
    gotoxy(c.x, c.y);
}



wchar_t * erase_last_character(wchar_t *buf) {
    int i = wcslen(buf) - 1;
    buf[i] = L'\0';
    // buf[i - 1] = L'\0';
    return buf;
}

void scroll_screen(struct list *l, struct cursor c, int amount) {
    clrscr();
    gotoxy(1,2);
    struct list *tmp = l;
    int cur_line = 1; // screen line
    int max_lines = info.screenheight -3; // header, footer
    //rewind

    // should handle paging?
    while (amount != 0) {
        if (amount > 0) tmp = tmp->next;
        if (amount < 0) tmp = tmp->prev;
        amount = amount + (amount > 0 ? -1 : 1);
    }

    while (tmp != NULL) {
        printf("%ls\n", tmp->line);
        tmp = tmp->next;
        cur_line++;
        if (cur_line > max_lines) {
            break;
        }
    }
    gotoxy(c.x, c.y);

}


int save_file(struct list *data){
    char filename[MAX_FILENAME_LENGTH];
    FILE *file;
    char *mbstr;
    size_t len;

    if (data == NULL || data->line == NULL) {
        printf("Error: Invalid data structure\n");
        return -1;
    }

    printf("Enter filename to save: ");
    if (fgets(filename, MAX_FILENAME_LENGTH, stdin) == NULL) {
        printf("Error reading filename: %s\n", strerror(errno));
        return -1;
    }
    gotoxy(1,1);

    filename[strcspn(filename, "\n")] = '\0';

    if (strcmp(filename, "") == 0) {
        printf("Error: Filename cannot be empty\n");
        return -1;
    }

    // clrscr(); // No. This is a bug
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
        if (current->line == NULL) {
            current->line = malloc(MAX_LINE_LENGTH * sizeof(wchar_t));
        }
        len = wcslen(current->line);

        if (len == 0) {
            current->line[0] = L'\0';
            len = 1;
        }

        size_t capacity = 4 * (len + 1);
        mbstr = malloc(capacity);

        if (mbstr == NULL) {
            printf("Error: Failed to allocate memory for mbstr\n");
            fclose(file);
            return -1;
        }

        size_t bytes = wcstombs(mbstr, current->line, capacity);

        if (bytes == (size_t)-1) {
            printf("Error converting wide char to multibyte: %s\n", strerror(errno));
            free(mbstr);
            fclose(file);
            return -1;
        }

        fwrite(mbstr, sizeof(char), bytes, file);
        fputc('\n', file);
        free(mbstr);
        current = current->next;
    }


    fclose(file);
    return 1;
}

struct list* load_file(struct list *data){
    char filename[MAX_FILENAME_LENGTH];

    if (data == NULL) {
        printf("Error: Invalid data structure\n");
        return NULL;
    }

    printf("Enter filename to load: ");
    if (fgets(filename, MAX_FILENAME_LENGTH, stdin) == NULL) {
        printf("Error reading filename: %s\n", strerror(errno));
        return NULL;
    }

    filename[strcspn(filename, "\n")] = '\0';

    if (strcmp(filename, "") == 0) {
        printf("Error: Filename cannot be empty\n");
        return NULL;
    }

    return load_file_by_filename(data, filename);
}



struct list* load_file_by_filename(struct list* data, const char* filename) {
    FILE *file;
    wchar_t buf[MAX_LINE_LENGTH]; // line buffer

    if (data == NULL) {
        printf("Error: Invalid data structure\n");
        return NULL;
    }

    // 1. Free existing data COMPLETELY
    struct list *current = data;
    while (current != NULL) {
        struct list *tmp = current;
        free(current->line);
        current = current->next;
        free(tmp);
    }

    // 2. Create a fresh head node
    struct list *new_head = init_node();
    if (new_head == NULL) {
        printf("Error: Failed to initialize editor\n");
        return NULL;
    }

    clrscr();
    gotoxy(1,1);
    clreol();
    printf("%s\n", filename);

    file = fopen(filename, "r");
    if (file == NULL) {
        printf("Error opening file: %s\n", strerror(errno));
        free(new_head->line);
        free(new_head);
        return NULL;
    }

    // 3. Properly advance the pointer while reading
    struct list *curr = new_head;
    while (fgetws(buf, MAX_LINE_LENGTH, file) != NULL) {
        wcscpy(curr->line, buf);
        curr->next = init_node();
        if (curr->next == NULL) {
            break;
        }
        curr = curr->next;
        wprintf(L"%ls", buf);
    }

    fclose(file);
    return new_head;
}

struct list * init_node() {
    struct list *l = malloc(sizeof(struct list));
    if (l == NULL) {
        printf("Error: Failed to allocate memory for list node\n");
        return NULL;
    }

    l->line = malloc(MAX_LINE_LENGTH * sizeof(wchar_t));
    if (l->line == NULL) {
        printf("Error: Failed to allocate memory for line\n");
        free(l);
        return NULL;
    }

    l->line[0] = 0;
    l->next = NULL;
    l->prev = NULL;
    l->line_number = 1;
    return l;
}

struct list * store_and_next_node(struct list *l, const wchar_t *buf) {
    if (l == NULL) return NULL;
    wcscpy(l->line, buf);
    if (l->next == NULL) {
        l->next = init_node();
        l->next->line_number = l->line_number + 1;
        l->next->prev = l;
    }
    // l = l->next; // allegedly does nothing
    return l->next;
}

wchar_t *bitstr_to_str(wchar_t *buf) {
    int i=0, j=0;
    int bitpos=0;
    int bits[8] = { 128, 64, 32, 16, 8, 4, 2, 1 };
    wchar_t *out = calloc(MAX_LINE_LENGTH, sizeof(wchar_t));
    if (out == NULL) {
        printf("Error: Failed to allocate memory for bitstr_to_str\n");
        return NULL;
    }

    while(buf[i]){
        switch (buf[i]){
            case L'1':
            case L'0':
                // FIXED: Replaced dangerous atoi cast with simple ternary
                {
                    int bit_val = (buf[i] == L'1') ? 1 : 0;
                    out[j] += bit_val * bits[bitpos];
                }
                bitpos++;
                if (8 == bitpos) {
                    bitpos = 0;
                    j++;
                }
                break;
            default:
                break;
        }
        i++;
    }
    out[j] = L'\0'; // FIXED: Correct null termination index
    return out;
}

wchar_t *str_to_bitstr(wchar_t *buf){
    int i=0;
    // FIXED: Added +1 for null terminator to prevent buffer overflow
    wchar_t *out = malloc((9 * MAX_LINE_LENGTH + 1) * sizeof(wchar_t));
    if (out == NULL) {
        printf("Error: Failed to allocate memory for str_to_bitstr\n");
        return NULL;
    }

    int bits[8] = { 128, 64, 32, 16, 8, 4, 2, 1 };

    while(buf[i]){
        int tmp = (int)buf[i];
        if (tmp > 255) tmp = 255; // Clamp to 8 bits for this simple converter

        for (int j=0; j<8; j++) {
            if (tmp - bits[j] >= 0) {
                out[j + i*9] = L'1';
                tmp -= bits[j];
            } else {
                out[j + i*9] = L'0';
            }
        }
        out[8 + i*9] = L' ';
        i++;
    }
    out[i*9] = L'\0';
    return out;
}

void print_help(){
    clrscr();
    _setcursortype(20);
    reset_header();
    gotoxy(1,2);
    textcolor(LIGHTGRAY);
    const char *help = "\
    \n\
    Commands:\n\
    Ctrl-S: Save\n\
    Ctrl-P: Print\n\
    Ctrl-B: Current line to binary?\n\
    Ctrl-D: Current line to decimal?\n\
    Ctrl-O: Open\n\
    Ctrl-?: Help\n";
    printf("%s", help);
    textcolor(WHITE);
}

int add_extra_buffer(struct list *l, wchar_t *buf, int i){
    if (i == MAX_LINE_LENGTH - 1) {
        l = store_and_next_node(l, buf);
        if (l == NULL) {
            printf("Error: Failed to store line\n");
            return 0;
        }
        // multiple buffer, same line number
        l->line_number=l->prev->line_number;
        memset(buf, 0, MAX_LINE_LENGTH * sizeof(wchar_t));
    } else {
        return 0;
    }
    return 1;
}
void edit_mode(){
    clrscr();
    _setcursortype(20);
    enum { KEY_ESC = 27, KEY_ENTER = '\r', KEY_BACKSPACE = 8,
        CTRL_S = 19, CTRL_P = 16, CTRL_B = 2, CTRL_D = 4,
        CTRL_O = 15, CTRL_Q = 17, CTRL_HELP = -1,
        KEY_UP = 72, KEY_DOWN = 80, KEY_LEFT = 75, KEY_RIGHT = 77};

    int i = 0;
    wchar_t buf[MAX_LINE_LENGTH];
    int val;
    wchar_t *bits;

    struct cursor c = {1,2};
    printf("%c", 0x2588);

    struct list *l = init_node();
    if (l == NULL) {
        printf("Error: Failed to initialize editor\n");
        return;
    }

    reset_header();

    struct list *head = l;
    memset(buf, 0, MAX_LINE_LENGTH * sizeof(wchar_t));
    gotoxy(c.x, c.y);

    while((val = getch()) != KEY_ESC){
        // 1. FIXED: Handle extended keys (arrow keys, function keys)
        if (val == 0 || val == 224) {
            val = getch(); // Consume the actual scan code

            // Prevent arrow keys from corrupting the buffer.
            // (Full cursor navigation requires tracking X/Y separately)
            switch(val) {
                // utilize fallthrough
                case KEY_UP:
                    if (l->prev == NULL) {
                        // cursor may be in wrong position, allow scrolling in that case.
                        if (c.y > 2){
                            c.y -= 1;
                        }
                        break;
                    };
                    if (c.y <= 2) {
                        scroll_screen(l, c, -1);
                        c.y = 2;
                    }
                    c.y -= 1;
                    wcscpy(l->line, buf); // store current line
                    l = l->prev;
                    wcscpy(buf, l->line); // load previous line
                    i = wcslen(l->line);
                    if (c.x > i+1) c.x = i+1;
                    break;
                case KEY_DOWN:
                    c.y += 1;
                    l = store_and_next_node(l, buf);
                    if (wcslen(l->line) > 0) {
                        i = wcslen(l->line);
                        wcscpy(buf, l->line);
                    } else {
                        clear_buffer(buf);
                    }
                    if (c.y > info.screenheight - 2) {
                        c.y = info.screenheight - 2;
                        scroll_screen(l, c, 1);
                    }
                    break;
                case KEY_LEFT:
                    if (c.x <= 1) break;
                    c.x -= 1;
                    i--;
                    break;
                case KEY_RIGHT:
                    c.x += 1;
                    if (buf[i] == L'\0') {
                        buf[i] = L' ';
                    }
                    i++;

                    if (i > MAX_LINE_LENGTH - 1) {
                        i = 0;
                        l = store_and_next_node(l, buf);
                        clear_buffer(buf);
                    }
                    break;
            }
            //debug
            switch (val) {
                case KEY_UP: case KEY_DOWN: case KEY_LEFT: case KEY_RIGHT:
                    gotoxy(1, info.screenheight - 1);
                    clreol();
                    printf("key %d, x%d, y%d line %d", val, c.x, c.y, l->line_number);
                    gotoxy(c.x, c.y);
                    continue; // Skip normal character processing
            }
        }

        int extra = 0;

        switch (val){
            case KEY_ENTER:
                c.y++; c.x = 1;
                l = store_and_next_node(l, buf);
                if (l == NULL) {
                    printf("Error: Failed to store line\n");
                    break;
                }
                i = 0;
                clear_buffer(buf);
                break;

            case KEY_BACKSPACE:
                if (i > 0) {
                    shift_buffer(buf, i, -1);
                    i--;
                    c.x = i;
                    if (c.x < 1) {
                        c.x = 1;
                    }
                    gotoxy(c.x, c.y);

                    printf("%ls", buf+i-1);
                    c.x++;
                }
                break;

            case CTRL_S:
                // Ensure current line is saved before writing to disk
                l = store_and_next_node(l, buf);
                clrscr();
                gotoxy(1,2);


                gotoxy(1, info.screenheight);
                printf("Save file: ");
                clreol();
                i = 0;
                clear_buffer(buf);

                if (save_file(head) == -1) {
                    printf("Failed to save file\n");
                }
                reset_header();
                gotoxy(1,2);
                dump_list(head);
                break;

            case CTRL_O:
                c.x = wherex();
                c.y = wherey();
                gotoxy(1, info.screenheight);
                printf("Load file: ");
                clreol();

                store_and_next_node(l, buf);
                if (l->next != NULL) l = l->next;
                i = 0;
                memset(buf, 0, MAX_LINE_LENGTH * sizeof(wchar_t));

                // 4. FIXED: Actually assign the returned head pointer
                struct list *new_head = load_file(head);
                if (new_head == NULL) {
                    printf("Failed to load file\n");
                } else {
                    head = new_head;
                    l = head;
                    while (l->next != NULL) {
                        l = l->next;
                    }
                    wcscpy(buf, l->line);
                    i = wcslen(buf);
                }

                reset_header();
                break;

            case CTRL_B:
                gotoxy(1, c.y++);
                bits = str_to_bitstr(buf);
                if (bits != NULL) {
                    wprintf(L"%ls", bits);
                    free(bits);
                }
                gotoxy(1, c.y++);
                break;

            case CTRL_D:
                c.y++;
                gotoxy(1, c.y);
                bits = bitstr_to_str(buf);
                if (bits != NULL) {
                    wprintf(L"%ls", bits);
                    free(bits);
                }
                gotoxy(1, c.y++);
                break;

            case CTRL_P:
                break;

            case CTRL_HELP:
                c.x = wherex();
                c.y = wherey();
                gotoxy(1, 2);
                print_help();
                restore_screen(l, c);
                break;

            default:
                extra = add_extra_buffer(l, buf, i);
                if (extra == 1){
                    l = l->next;
                    i = 0;
                }
                if (val != 0) {
                    wprintf(L"%lc", (wchar_t)val);
                }
                for (int j=0; j<i+1; j++){
                    if (buf[j] == L'\0') {
                        buf[j] = L' ';
                    }
                }

                shift_buffer(buf, i, 1);
                buf[i++] = (wchar_t)val;
                wprintf(L"%ls", buf+i );
                c.x = c.x + 1;
                c.y = wherey();
                break;
        }
        wcscpy(l->line, buf); // FIXED: Use wcscpy to save line
        // update "UI"
        gotoxy(1,info.screenheight-1);
        wprintf(
            L"%d %2d %ls",
            l->line_number, i, buf); // FIXED: %ls for wchar_t string
        clreol();
        gotoxy(c.x, c.y);
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