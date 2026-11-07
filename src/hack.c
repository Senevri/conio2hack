#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>
#include <conio2.h>
#include <errno.h>

enum { KEY_UP = 72, KEY_DOWN = 80, KEY_LEFT = 75, KEY_RIGHT = 77 };

struct text_info info; //filled in main

const int SIDE_PANE_WIDTH = 20;

struct list {
    wchar_t * line;
    struct list * next;
};

struct point {
    int x, y;
};

struct area {
    int x, y;
    int width, height;
};


struct screen {
    int width, height;
    char * screenmap;
    bool * dirty;
};

struct screen * _screen = NULL;

// Any symbol displayed on screen
struct symbol {
    const char * name;
    char character;
    int color;
};

//gameplay related data
struct statistics {
    int atk, def, hp, max_hp, kills;
};

const char * stat_titles[] = {
    "ATK", "DEF", "HP", "MAX", "KILLS", NULL
};


const int NUM_STATS = 5;

//any creature in game
struct creature {
    char name[255];
    int id;
    struct symbol sym;
    struct statistics stats;
    int x;
    int y;
    struct creature * next;
    struct creature * prev;
};

enum TERRAIN { GRASS, WATER, WALL };

// struct symbol terrain[256] = {
//     {"grass", '.', GREEN},
//     {"water", '~', BLUE},
//     {"wall",  '#', DARKGRAY}
// };

#define MAX_TERRAIN_CHARS 256

static struct symbol terrain[MAX_TERRAIN_CHARS];

static void init_terrain_defaults(void);



// BLACK,          /**< black color */
// BLUE,           /**< blue color */
// GREEN,          /**< green color */
// CYAN,           /**< cyan color */
// RED,            /**< red color */
// MAGENTA,        /**< magenta color */
// BROWN,          /**< brown color */
// LIGHTGRAY,      /**< light gray color */
// DARKGRAY,       /**< dark gray color */
// LIGHTBLUE,      /**< light blue color */
// LIGHTGREEN,     /**< light green color */
// LIGHTCYAN,      /**< light cyan color */
// LIGHTRED,       /**< light red color */
// LIGHTMAGENTA,   /**< light magenta color */
// YELLOW,         /**< yellow color */
// WHITE           /**< white color */

//struct screen screen = {0, 0, NULL, NULL, 0};

/* Define specific terrain tiles */
static const struct symbol known_terrain[] = {
        {"wall",       '#',  LIGHTGRAY},
        {"floor",      '.',  WHITE},
        {"rock",       '(',  CYAN},
        {"water",      '~',  BLUE},
        {"tree",       '*',  GREEN},
        {"dirt",      ',',  YELLOW},
        {"grass",     '+',  GREEN},
        {"stairs",    '^',  YELLOW},
        {"door",      '|',  LIGHTCYAN},
        {"stair_up",  '>',  LIGHTRED},
        {"stair_down",'<',  RED},
        {"bridge",    '@',  WHITE},
        {"puddle",    '/',  BLUE},
        {NULL, '\0', BLACK}
};

/* Default terrain entry - safe fallback for unknown characters */
static void init_terrain_defaults(void) {
    int i;
    for (i = 0; i < MAX_TERRAIN_CHARS; i++) {
        terrain[i].name   = "space";
        terrain[i].character = ' ';
        terrain[i].color = BLACK;
    }
}

const char * terrain_map[MAX_TERRAIN_CHARS];


void init_terrain(void) {
    int i;
    for (i = 0; known_terrain[i].name != NULL; i++) {
        terrain[(int)known_terrain[i].character] = known_terrain[i];
        terrain_map[i] = known_terrain[i].name;
    }
}



struct symbol get_terrain_symbol(const char * name) {
    int i = 0;
    for (i = 0; terrain[i].name != NULL; i++) {
        if (0 == strcmp(name, terrain[i].name)) {
            break;
        }
    }
    return terrain[i];
}

int get_symbol_idx(const char *name) {
    for (int i = 0; i < MAX_TERRAIN_CHARS; i++) {
        if (terrain[i].name && 0 == strcmp(name, terrain[i].name)) return i;
    }
    return 0; // or get_symbol_idx("space") safely
}


char get_terrain(struct screen screen, int x, int y) {
        if (x < 0 || x >= screen.width || y < 0 || y >= screen.height) {
            return ' ';  /* default to floor at boundaries */
        }
        return screen.screenmap[y * (screen.width + 1) + x];
}

void full_refresh(char *screenmap, struct symbol symbols[256]) {
    for (int y = 0; y < info.screenheight - 1; y++) {
        for (int x = 0; x < info.screenwidth; x++) {
            unsigned char out = screenmap[y * (info.screenwidth + 1) + x];
            if (15 < x && x < info.screenwidth - 1 && 0 < y && y < info.screenheight - 2) {
                textcolor(symbols[out].color);
                putchxy(x + 1, y + 1, symbols[out].character);
            } else {
                textcolor(LIGHTGRAY);
                putchxy(x + 1, y + 1, out);
            }
        }
    }
    gotoxy(0, 0);
}


bool check_is_dirty_point(struct screen * scr, int x, int y) {
    return scr->dirty[y * (info.screenwidth + 1) + x];
}

bool check_is_dirty(struct screen * scr, struct area area) {
    for (int y = area.y; y < area.y + area.height; y++) {
        for (int x = area.x; x < area.x + area.width; x++) {
            if (scr->dirty[y * (info.screenwidth + 1) + x]) return true;
        }
    }
    return false;
}

void mark_dirty(struct screen * scr, int x, int y) {
    scr->dirty[y * (info.screenwidth + 1) + x] = true;
}

void mark_clean(struct screen * scr, int x, int y) {
    scr->dirty[y * (info.screenwidth + 1) + x] = false;
}

void mark_dirty_area(struct screen * scr, struct area area) {
    for (int y = area.y; y < area.y + area.height; y++) {
        for (int x = area.x; x < area.x + area.width; x++) {
            scr->dirty[y * (info.screenwidth + 1) + x] = true;
        }
    }
}

void mark_clean_area(struct screen * scr, struct area area) {
    for (int y = area.y; y < area.y + area.height; y++) {
        for (int x = area.x; x < area.x + area.width; x++) {
            scr->dirty[y * (info.screenwidth + 1) + x] = false;
        }
    }
}


// --- Creature functions ---


int * get_creature_stats(int * stats, struct creature cr) {
    memcpy(stats, &cr.stats, sizeof(cr.stats));
    return stats;
}


struct creature * get_last_creature(struct creature * critters) {
    struct creature * cr = critters;
    while (cr != NULL) {
        critters = cr;
        cr = cr->next;
    }
    return critters;
}

struct creature * update_creature_position(struct creature * cr, char* name, int x, int y) {
    if (cr == NULL) return NULL;

    while (cr != NULL) {
        if (strcmp(cr->name, name) == 0) {
            cr->x = x;
            cr->y = y;
            return cr;
        }
        cr = cr->next;
    }
    return NULL;
}

struct creature* create_creature(const char * name, const char symbol, int color, int x, int y) {
    struct creature * c = malloc(sizeof(struct creature));
    if (c == NULL) {
        printf("Error: Failed to allocate memory for creature\n");
        return NULL;
    }

    strcpy(c->name, name);
    c->sym.character = symbol;
    c->sym.color = color;
    c->x = x;
    c->y = y;
    c->stats.atk = 10;
    c->stats.def = 10;
    c->stats.hp = 5;
    c->stats.max_hp = 5;
    c->stats.kills = 0;
    c->next = NULL;
    c->prev = NULL;

    if (c == NULL) {
        printf("Error: Failed to create monster\n");
        return NULL;
    }


    return c;
}

static inline void draw_creature(struct creature * creature, struct screen scr) {
    if (creature == NULL) return;
    textcolor(creature->sym.color);
    putchxy(creature->x, creature->y, creature->sym.character);
}

void draw_creatures(struct creature* critters, struct screen scr) {
    struct creature * cr = critters;
    while (cr != NULL) {
        draw_creature(cr, scr);
        cr = cr->next;
    }
}

void erase_creatures(struct creature *critters, struct screen scr, struct symbol symbols[256]) {
    char *screenmap = scr.screenmap;
    for (struct creature *cr = critters; cr != NULL; cr = cr->next) {
        if (!check_is_dirty_point(&scr, cr->x, cr->y)) continue;
        int mx = cr->x - 1;   // map coords
        int my = cr->y - 1;
        if (mx < 0 || my < 0 || mx >= scr.width || my >= scr.height) continue;
        unsigned char out = screenmap[my * (scr.width + 1) + mx];
        textcolor(symbols[out].color);
        putchxy(cr->x, cr->y, symbols[out].character);
    }
}

int split_string(const char *string, char **lines, int max_lines, int max_length) {
    int line_count = 0;
    char *current_line = NULL;
    int current_pos = 0;

    for (int i = 0; i < strlen(string); i++) {
        if (string[i] == '\n' || i - current_pos >= max_length) {
            if (line_count >= max_lines) {
                break;
            }
            current_line = malloc((i - current_pos + 1) * sizeof(char));
            memcpy(current_line, string + current_pos, i - current_pos);
            current_line[i - current_pos] = '\0';
            lines[line_count++] = current_line;
            current_pos = i + 1;
        }
    }
    if (current_pos < strlen(string)) {
        if (line_count < max_lines) {
            current_line = malloc((strlen(string) - current_pos + 1) * sizeof(char));
            memcpy(current_line, string + current_pos, strlen(string) - current_pos);
            current_line[strlen(string) - current_pos] = '\0';
            lines[line_count++] = current_line;
        }
    }
    return line_count;
}

// --- UI functions ---

const int MAX_LINES = 10;

void write_side_pane(const char * string, int palette_color, int x_offset, int y_offset) {

    // split to multiple lines
    if (strlen(string) > SIDE_PANE_WIDTH-(2+x_offset)) {
        char *lines[10];
        int line_count = split_string(string, lines, MAX_LINES, SIDE_PANE_WIDTH-(2+x_offset));
        for (int i = 0; i < line_count; i++) {
            textcolor(palette_color);
            gotoxy(1+x_offset, 1+i+y_offset);
            printf("%s", lines[i]);
            free(lines[i]);
        }
    } else {
        textcolor(palette_color);
        gotoxy(1+x_offset, 1+y_offset);
        printf("%s", string);
    }
}


void print_creature_stats(struct creature * cr, char offset_y) {
    if (cr == NULL) return;

    //int num_stats = sizeof(cr->stats)/sizeof(int);

    int * stats = (int *)&cr->stats;

    textcolor(YELLOW);
    cputsxy(3, 2+offset_y, cr->name);
    textcolor(WHITE);

    for (int i = 0; stat_titles[i] != NULL; i++) {
        gotoxy(3, 3 + offset_y + i);
        printf("%-5s:%d", stat_titles[i], stats[i]);
    }
    mark_dirty_area(_screen, (struct area) { 0, 0, info.screenwidth, info.screenheight });
}

// const char * format(const char * fmt, ...) {
//     va_list args;
//     va_start(args, fmt);
//     int buf_size = 1024*sizeof(char);
//     const char * buffer = malloc(buf_size);
//     vsnprintf(buffer, buf_size, fmt, args);
//     va_end(args);
//     return buffer; //memory leak?
// }

void update_creature_stat_display(struct creature * cr, int offset_y) {
    getch();
    if (cr == NULL) return;

    int * stats = (int *)&cr->stats;

    // char * stat_str = malloc(10*sizeof(char));
    // memset(stat_str, 0, 10*sizeof(char));
    int xpos = 4+5;
    char stat_str[10];
    for (int i = 0; stat_titles[i] != NULL; i++) {
        snprintf(stat_str, 10, "%d  ", stats[i]);
        // write_side_pane(stat_str, YELLOW, 6, 1+i + 1+offset_y);
        cputsxy(xpos, 2+ i + 1 +offset_y, stat_str);
    }
    //free(stat_str);
}


struct creature* creature_in_position(struct creature * head, int x, int y) {
    if (head == NULL) return NULL;

    struct creature * cr = head->next;
    while (cr != NULL) {
        if (cr->x == x && cr->y == y) {
            return cr;
        }
        cr = cr->next;
    }
    return NULL;
}

void get_padded_string(const char *string, int length, char *out) {
    memset(out, ' ', length);
    size_t n = strlen(string);
    if (n > (size_t)length) n = length;
    memcpy(out, string, n);
    out[length] = '\0';
}

void clear_creature_stats(int offset_y) {

    int rows = NUM_STATS + 1;

    bool is_dirty = check_is_dirty(
        _screen, (struct area) { 1, offset_y, SIDE_PANE_WIDTH-2, rows }
    );
    if (!is_dirty) return;

    char clearstr[SIDE_PANE_WIDTH-1];
    memset(clearstr, ' ', SIDE_PANE_WIDTH-2);
    clearstr[SIDE_PANE_WIDTH-2] = '\0';
    for (int i = 0; i < NUM_STATS+3; i++) {
        //write_side_pane(clearstr, BLACK, 1, offset_y+i);
        cputsxy(2, offset_y+i, clearstr);
    }
    mark_clean_area(_screen, (struct area){ 1, offset_y, SIDE_PANE_WIDTH-2, rows });
}

bool movement_blocked(struct creature *player, char *screenmap, struct creature *monster) {
    if (player == NULL || screenmap == NULL) return true;

    // Check for wall collision
    char id = screenmap[(player->y - 1) * (info.screenwidth + 1) + (player->x - 1)];
    if (id == get_symbol_idx("wall")) {
        return true;
    }

    // Check for monster collision
    struct creature *cr = creature_in_position(player, player->x, player->y);
    if (cr != NULL && strcmp(cr->name, player->name) != 0) {
        // Simple damage calculation
        int damage = player->stats.atk - monster->stats.def;
        if (damage < 0) damage = 0;
        monster->stats.hp -= damage;

        // Display damage message
        print_creature_stats(monster, NUM_STATS +2 );
        cputsxy(3, 4+NUM_STATS +2, "Damage dealt!");

        // Check if monster is defeated
        if (monster->stats.hp <= 0) {
            cputsxy(11, 2+NUM_STATS +2, "defeated!");
            player->stats.kills += 1;
            // Remove monster from the game
            struct creature* prev = player;
            while (prev->next != monster) {
                prev = prev->next;
            }
            prev->next = monster->next;
            free(monster);
            return false;
        }
        return true;
    }
    clear_creature_stats(NUM_STATS + 3);
    return false;
}

struct creature * handle_input(int val, struct creature *player, struct screen scr, struct creature *monster) {
    if (player == NULL || scr.screenmap == NULL) return NULL;

    int old_x = player->x;
    int old_y = player->y;

    // Handle player movement
    switch (val) {
        case KEY_UP:
            if (player->y > 2) {
                player->y -= 1;
            }
            break;
        case KEY_DOWN:
            if (player->y < info.screenheight - 2) {
                player->y += 1;
            }
            break;
        case KEY_LEFT:
            if (player->x > 17) {
                player->x -= 1;
            }
            break;
        case KEY_RIGHT:
            if (player->x < info.screenwidth - 1) {
                player->x += 1;
            }
            break;
        case '.':
            break;
        default:
            val = 0;
            break;
    }

    if (val != '.') {
        mark_dirty(&scr, old_x, old_y);
    }

    // Handle monster movement (simple AI)
    if (monster != NULL && val > 0) {
        struct point old_pos = { monster->x, monster->y };
        // Simple AI: move toward player
        if (monster->x < player->x && monster->x < info.screenwidth - 1) {
            monster->x += 1;
        } else if (monster->x > player->x && monster->x > SIDE_PANE_WIDTH - 1) {
            monster->x -= 1;
        }
        if (monster->y < player->y && monster->y < info.screenheight - 2) {
            monster->y += 1;
        } else if (monster->y > player->y && monster->y > 2) {
            monster->y -= 1;
        }
        if (movement_blocked(monster, scr.screenmap, monster)) {
            monster->x = old_pos.x;
            monster->y = old_pos.y;
        }
        mark_dirty(&scr, old_pos.x, old_pos.y);
    }

    // Check for collisions
    if (movement_blocked(player, scr.screenmap, monster)) {
        player->x = old_x;
        player->y = old_y;
    }

    return player;
}

void clear_sidebar(char start, char end) {
    for (int i = start; i < end; i++) {
        for (int j = 1; j < SIDE_PANE_WIDTH-2; j++) {
            const int point = i * (info.screenwidth + 1) + j;
            if (_screen->dirty[point] == false) {
                continue;
            }
            _screen->dirty[point] = false;
            gotoxy(info.screenwidth + 1, i);
            printf(" ");
        }

    }
}

void free_creatures(struct creature * critters) {
    struct creature * current = critters;
    while (current != NULL) {
        struct creature * next = current->next;
        free(current);
        current = next;
    }
}



enum filltype { FILL, NO_FILL, BORDER };

///void screen_rect(struct screen * scr, struct symbol symbol, struct area area, enum filltype fill, struct symbol * inner_symbol)
void screen_rect(struct symbol symbol, struct area area, enum filltype fill, struct symbol * inner_symbol) {

    struct symbol fill_symbol = symbol;
    // area.x = area.x + 1;
    // area.y = area.y + 1;
    //area.width = area.width - 1;
    //area.height = area.height - 1;
    if (fill == FILL || fill == BORDER) {
        for (int y = area.y; y < area.y + area.height; y++) {
            for (int x = area.x; x < area.x + area.width; x++) {
                if (inner_symbol != NULL) {
                  fill_symbol = *inner_symbol;
                }
                textcolor(fill_symbol.color);
                putchxy(x, y, fill_symbol.character);
                //scr->dirty_coords[scr->dirty_count] = y * (info.screenwidth + 1) + x;
            }
        }
    }

    if (fill==NO_FILL || fill==BORDER) {
        int y = area.y;
        for (int x = area.x; x < area.x + area.width; x++) {
            textcolor(symbol.color);
            putchxy(x, y, symbol.character);
            putchxy(x, y+area.height, symbol.character);
        }
        int x = area.x;
        for (y = area.y; y < area.y + area.height; y++) {
            textcolor(symbol.color);
            putchxy(x, y, symbol.character);
            putchxy(x+area.width, y, symbol.character);
        }
    }
}


void map_rect(char * screenmap, int symbol_code, struct area area, enum filltype fill, int symbol_code2) {
    // area.x = area.x + 1;
    // area.y = area.y + 1;
    area.width = area.width - 1;
    area.height = area.height - 1;
    if (fill == FILL || fill == BORDER) {
        for (int y = area.y; y < area.y + area.height; y++) {
            for (int x = area.x; x < area.x + area.width; x++) {
                if (symbol_code2 > 0) {
                    screenmap[y * (info.screenwidth + 1) + x] = symbol_code2;
                } else {
                    screenmap[y * (info.screenwidth + 1) + x] = symbol_code;
                }
            }
        }

    }

    if (fill == NO_FILL || fill == BORDER) {
        int y = area.y;
        for (int x = area.x; x < area.x + area.width; x++) {
            screenmap[y * (info.screenwidth + 1) + x] = symbol_code;
            screenmap[(y+area.height) * (info.screenwidth + 1) + x] = symbol_code;
        }
        int x = area.x;
        for (y = area.y; y <= area.y + area.height; y++) {
            screenmap[y * (info.screenwidth + 1) + x] = symbol_code;
            screenmap[y * (info.screenwidth + 1) + x+area.width] = symbol_code;
        }
    }
}

void draw_screen(struct screen * screen, struct symbol terrain[256], unsigned char transparent_char) {
    for (int y = 0; y < screen->height; y++) {
        for (int x = 0; x < screen->width; x++) {
            char idx = screen->screenmap[y * (screen->width + 1) + x];
            if (idx == transparent_char) {
                continue;
            }
            if (idx < 0) {
                idx = get_symbol_idx("space");
            }
            if (idx > 255){
                idx = get_symbol_idx("space");
            }
            struct symbol sym = terrain[(int)idx];
            textcolor(sym.color);
            putchxy(x + 1, y + 1, sym.character);
        }
    }
}


void add_creature(struct creature * critters, struct creature * creature) {
    struct creature * last = get_last_creature(critters);
    last->next = creature;
}



struct point get_random_position() {
    struct point p;
    p.x = SIDE_PANE_WIDTH + 2 + rand() % (info.screenwidth - SIDE_PANE_WIDTH - 4);
    p.y = 2 + rand() % (info.screenheight - 5);
    return p;
}

void add_creature_if_none(struct creature * critters) {
    if (critters->next != NULL) {
        return;
    }
    struct creature *c = create_creature("monsu", 'm', RED, 0, 0);
    // randomize position
    struct point p = get_random_position();
    // collision check
    while (
        creature_in_position(critters, p.x, p.y)) p = get_random_position();

    c->x = p.x;
    c->y = p.y;
    add_creature(critters, c);

}

void roguelike() {
    enum { KEY_ESC = 27, KEY_ENTER = '\r', KEY_BACKSPACE = 8,
        CTRL_S = 19, CTRL_P = 16, CTRL_Q = 17, CTRL_B = 2,
        KEY_UP = 72, KEY_DOWN = 80, KEY_LEFT = 75, KEY_RIGHT = 77 };

    enum { FILL, NO_FILL, BORDER };



    inittextinfo();
    gettextinfo( &info );
    struct screen screen = {
        .width = info.screenwidth,
        .height = info.screenheight,
        .screenmap = malloc(info.screenheight * (info.screenwidth + 1)),
        .dirty = malloc(info.screenheight * (info.screenwidth + 1)),
    };
    _screen = &screen;
    clrscr();

    memset(screen.screenmap, get_symbol_idx("space"), (info.screenwidth + 1) * info.screenheight);
    memset(screen.dirty, 0, (info.screenwidth + 1) * info.screenheight);

    struct creature * critters = create_creature("playa", '@', LIGHTGRAY, 16 + ((info.screenwidth - 16) / 2), info.screenheight / 2);
    if (critters == NULL) {
        printf("Error: Failed to create player\n");
        return;
    }

    struct creature * head = critters;

    /* store player for convenience */
    struct creature * player = critters;
    struct statistics s_pl = { .atk = 11, .def = 8, .hp = 5, .max_hp = 5, .kills=0 };
    player->stats = s_pl;

    //construct terrain
    init_terrain_defaults();
    init_terrain();

    int val; // getch value
    clrscr();
    char * title[] = {"Roguelike", "Press Any Key"};
    for (int i = 0; i != 2; i++) {
        gotoxy((info.screenwidth - (strchr(title[i], '\0') - title[i])) / 2, -3 + i + info.screenheight / 2);
        printf("%s", title[i]);
    }
    getch();
    clrscr();

    struct symbol border = { .name = "border", .character = '*', .color = LIGHTGRAY };
    struct symbol vline = { .name = "line", .character = '|', .color = LIGHTGRAY };

    //let's make a lawn - screenmap is 0-idexed?
    for (int y = 1; y < info.screenheight - 2; y++) {
        for (int x = SIDE_PANE_WIDTH; x < info.screenwidth - 2; x++) {
            screen.screenmap[y * (info.screenwidth + 1) + x] = (90 < rand() % 100) ? get_symbol_idx("water") : get_symbol_idx("grass");
        }
    }

    // // a wall....
    struct area area = { .x = SIDE_PANE_WIDTH+5, .y = 5, .width = 10, .height = 10 };
    map_rect(screen.screenmap, get_symbol_idx("wall"),
        area, BORDER, get_symbol_idx("floor"));

    textcolor(WHITE);
    draw_screen(&screen, terrain, get_symbol_idx("space"));
    draw_creatures(head, screen);
    // --- UI ---

    screen_rect(
        border,
        (struct area){ .x = 1, .y = 1, .width = info.screenwidth-2, .height = info.screenheight - 2 },
        NO_FILL, (struct symbol *) NULL);
    screen_rect(
        vline,
        (struct area){ .x = SIDE_PANE_WIDTH, .y = 2, .width = 1, .height = info.screenheight - 4 },
        FILL, NULL
    );

    gotoxy(1, 1);
    _setcursortype(_NOCURSOR);
    _setcursortype(0);
    print_creature_stats(player, 0);
    while ((val = getch()) != CTRL_Q) {
        if (val == KEY_ESC) break;
        add_creature_if_none(head);
        erase_creatures(head, screen, terrain);
        player = handle_input(val, player, screen, head->next);
        draw_creatures(head, screen);
        textcolor(WHITE);
        update_creature_stat_display(player, 0);
        _setcursortype(_NOCURSOR);
        _setcursortype(0);
    }

    // Free all allocated memory
    free_creatures(critters);

    _setcursortype(_NORMALCURSOR);
    textcolor(YELLOW);
    clrscr();
}