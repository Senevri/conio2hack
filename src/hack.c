#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <conio2.h>
#include <errno.h>

struct text_info info; //filled in main

// macros
#define IDX(x, y) ((y) * (info.screenwidth + 1) + (x))

/* ------- command dispatcher  -------- */

enum {
    KEY_EXT   = 256,                 /* offset for prefixed keys */
    KEY_UP    = KEY_EXT + 72,
    KEY_DOWN  = KEY_EXT + 80,
    KEY_LEFT  = KEY_EXT + 75,
    KEY_RIGHT = KEY_EXT + 77,
    KEY_ESC   = 27,
    CTRL_Q    = 17,
    CTRL_C    = 3
};

enum action { ACT_NONE, ACT_UP, ACT_DOWN, ACT_LEFT, ACT_RIGHT, ACT_QUIT, ACT_WAIT };

struct binding { int key; enum action act; };

static struct binding bindings[] = {
    { KEY_UP,    ACT_UP    }, { 'k',    ACT_UP    },
    { KEY_DOWN,  ACT_DOWN  }, { 'j',    ACT_DOWN  },
    { KEY_LEFT,  ACT_LEFT  }, { 'h',    ACT_LEFT  },
    { KEY_RIGHT, ACT_RIGHT }, { 'l',    ACT_RIGHT },
    { CTRL_Q,    ACT_QUIT  }, { CTRL_C, ACT_QUIT  },
    { '.',       ACT_WAIT  },
};

enum action action_for(int key) {
    for (size_t i = 0; i < sizeof bindings / sizeof *bindings; i++)
        if (bindings[i].key == key) return bindings[i].act;
    return ACT_NONE;
}

int read_key(void) {
    int c = getch();
    if (c == 0 || c == 224)          /* prefix: real code follows */
        c = KEY_EXT + getch();
    return c;
}


struct list {
    wchar_t * line;
    struct list * next;
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

 struct point {
    int x, y;
 };

//gameplay related data
struct statistics {
    int atk, def, hp, max_hp;
};

//any creature in game
struct creature {
    char name[255];
    struct symbol sym;
    struct statistics stats;
    int x;
    int y;
    struct creature * next;
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

int get_symbol_idx(const char * name) {
    int i = 0;
    for (i = 0; terrain[i].name != NULL; i++) {
        if (0 == strcmp(name, terrain[i].name)) {
            break;
        }
    }
    return i;
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



bool check_is_dirty(struct screen * scr, struct area area) {
    for (int y = area.y; y < area.y + area.height; y++) {
        for (int x = area.x; x < area.x + area.width; x++) {
            if (scr->dirty[y * (info.screenwidth + 1) + x]) return true;
        }
    }
    return false;
}

void mark_dirty(struct screen * scr, struct area area) {
    for (int y = area.y; y < area.y + area.height; y++) {
        for (int x = area.x; x < area.x + area.width; x++) {
            scr->dirty[y * (info.screenwidth + 1) + x] = true;
        }
    }
}

void mark_clean(struct screen * scr, struct area area) {
    for (int y = area.y; y < area.y + area.height; y++) {
        for (int x = area.x; x < area.x + area.width; x++) {
            scr->dirty[y * (info.screenwidth + 1) + x] = false;
        }
    }
}


// --- Creature functions ---

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
    c->next = NULL;

    return c;
}

static inline void draw_creature(struct creature * creature) {
    if (creature == NULL) return;
    textcolor(creature->sym.color);
    putchxy(creature->x, creature->y, creature->sym.character);
}

void draw_creatures(struct creature* critters) {
    struct creature * cr = critters;
    while (cr != NULL) {
        draw_creature(cr);
        cr = cr->next;
    }
}

void erase_creatures(struct creature * critters, char * screenmap, struct symbol symbols[256]) {
    struct creature * cr = critters;
    while (cr != NULL) {
        unsigned char out = screenmap[(cr->y - 1) * (info.screenwidth + 1) + (cr->x - 1)];
        textcolor(symbols[out].color);
        putchxy(cr->x, cr->y, symbols[out].character);
        cr = cr->next;
    }
}

void print_creature_stats(struct creature * cr, char offset_y) {
    if (cr == NULL) return;

    char * stat_str[] = {
        "ATK", "DEF", "HP", "MAX", NULL
    };

    int * stats[] = {
        &cr->stats.atk,
        &cr->stats.def,
        &cr->stats.hp,
        &cr->stats.max_hp,
        NULL
    };

    gotoxy(2, 2 + offset_y);
    printf("%s", cr->name);


    cputsxy(3, 2+offset_y, cr->name);

    for (int i = 0; stats[i] != NULL; i++) {
        gotoxy(3, 3 + offset_y + i);
        printf("%s:%d", stat_str[i], *stats[i]);
    }
    mark_dirty(_screen, (struct area) { 0, 0, info.screenwidth, info.screenheight });
}

void update_creature_stat_display(struct creature * cr, int offset_y) {
    if (cr == NULL) return;

    int * stats[] = {
        &cr->stats.atk,
        &cr->stats.def,
        &cr->stats.hp,
        &cr->stats.max_hp,
        NULL
    };

    for (int i = 0; stats[i] != NULL; i++) {
        gotoxy(5, 3 + offset_y + i);
        printf(":%d", *stats[i]);
    }
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

void clear_creature_stats(int offset_y) {
    for (int i = 0; i <= 6; i++) {
        int sum_dirty = 0;
        for (int j = 0; j < 12; j++) {
            sum_dirty += _screen->dirty[
                (i + offset_y) * (info.screenwidth + 1) + j
            ] ? 1 : 0;
        };
        if (sum_dirty == 0) continue;
        cputsxy(3, i + offset_y, "       ");
        for (int j = 0; j < 6; j++) {
            _screen->dirty[(i + offset_y) * (info.screenwidth + 1 + j)] = false;
        };

    }
}

bool movement_blocked(struct creature *player, char * screenmap) {
    if (player == NULL || screenmap == NULL) return true;

    char id = screenmap[(player->y - 1) * (info.screenwidth + 1) + (player->x - 1)];
    if (id == get_symbol_idx("wall")) {
        return true;
    }

    if (creature_in_position(player, player->x, player->y)) {
        struct creature * cr = creature_in_position(player, player->x, player->y);
        if (cr != NULL && strcmp(cr->name, player->name) != 0) {
            print_creature_stats(cr, 6);
            return true;
        }
        clear_creature_stats(7); // off by one bug maybe?
        return false;
    }

    return false;
}

struct creature * handle_input(enum action act, struct creature * player, char * screenmap) {
    if (player == NULL || screenmap == NULL) return NULL;

    struct point max_edge = {
        .x=info.screenwidth - 1,
        .y=info.screenheight - 2
    };

    // testing suggestion
    struct point p_pos = { .x=player->x, .y=player->y };


    // enum action act = action_for(val);

    switch (act) {
        case ACT_UP:    player->y += (p_pos.y > 2)          ? -1 : p_pos.y;
            break;
        case ACT_LEFT:  player->x += (p_pos.x > 2)          ? -1 : p_pos.x;
            break;
        case ACT_RIGHT: player->x += (p_pos.x < max_edge.x) ?  1 : max_edge.x;
            break;
        case ACT_DOWN:  player->y += (p_pos.y < max_edge.y) ?  1 : max_edge.y;
            break;
        case ACT_WAIT:
            break;
        case ACT_NONE:
            break;
        case ACT_QUIT:
            // Save and quit?
            exit(0);
            break;
        default:
            break;
    }


    /* handle movement */

    if (movement_blocked(player, screenmap)) {
        player->x = p_pos.x;
        player->y = p_pos.y;
    }

    return player;
}

void clear_sidebar(char start, char end) {
    for (int i = start; i < end; i++) {
        for (int j = 0; j < 10; j++) {
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

///void screen_rect(struct screen * scr, struct symbol symbol, struct area area, enum filltype fill, struct symbol * inner_symbol) {
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




void roguelike() {
    // enum { KEY_ESC = 27, KEY_ENTER = '\r', KEY_BACKSPACE = 8,
    //     CTRL_S = 19, CTRL_P = 16, CTRL_Q = 17, CTRL_B = 2,
    //     KEY_UP = 72, KEY_DOWN = 80, KEY_LEFT = 75, KEY_RIGHT = 77 };

    char screenmap[info.screenheight][info.screenwidth + 1]; //y then x
    memset(screenmap, ' ', (info.screenwidth + 1) * info.screenheight);

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


    // struct screen screenbuffer = {
    //     .width = info.screenwidth, .height = info.screenheight, .screenmap = malloc(info.screenheight * (info.screenwidth + 1))
    // };


    clrscr();

    //char screenmap[info.screenheight][info.screenwidth + 1]; //y then x
    memset(screen.screenmap, get_symbol_idx("space"), (info.screenwidth + 1) * info.screenheight);
    memset(screen.dirty, 0, (info.screenwidth + 1) * info.screenheight);
    //memset(screenbuffer.screenmap, get_symbol_idx("space"), (info.screenwidth + 1) * info.screenheight);

    struct creature * critters = create_creature("playa", '@', LIGHTGRAY, 16 + ((info.screenwidth - 16) / 2), info.screenheight / 2);
    if (critters == NULL) {
        printf("Error: Failed to create player\n");
        return;
    }

    struct creature * head = critters;
    head->next = create_creature("monsu", 'M', RED, 16 + 20, 20);
    if (head->next == NULL) {
        printf("Error: Failed to create monster\n");
        free_creatures(critters);
        return;
    }

    /* store player for convenience */
    struct creature * player = critters;
    struct statistics s_pl = { .atk = 11, .def = 8, .hp = 5, .max_hp = 5 };
    player->stats = s_pl;


    //construct terrain
    init_terrain_defaults();
    init_terrain();

    clrscr();

    char * title[] = {"Roguelike", "Press Any Key"};
    for (int i = 0; i != 2; i++) {
        gotoxy((info.screenwidth - (strchr(title[i], '\0') - title[i])) / 2, -3 + i + info.screenheight / 2);
        printf("%s", title[i]);
    }

    read_key();
    clrscr();
    // screen_rect(get_terrain_symbol("space"), (struct area){0, 0, info.screenwidth, info.screenheight}, FILL, 0);

    // struct symbol terrain_symbol = get_terrain_symbol("wall");
    struct symbol border = { .name = "border", .character = '*', .color = LIGHTGRAY };
    struct symbol vline = { .name = "line", .character = '|', .color = LIGHTGRAY };

    //let's make a lawn
    for (int y = 1; y < info.screenheight - 2; y++) {
        for (int x = 15; x < info.screenwidth - 2; x++) {
            screen.screenmap[y * (info.screenwidth + 1) + x] = (90 < rand() % 100)
            ? get_symbol_idx("water") : get_symbol_idx("grass");
        }
    }


    // // a wall....
    struct area area = { .x = 20, .y = 5, .width = 10, .height = 10 };
    map_rect(
        screen.screenmap, get_symbol_idx("wall"),
        area, BORDER, get_symbol_idx("floor")
    );


    textcolor(WHITE);

    draw_screen(&screen, terrain, get_symbol_idx("space"));
    // draw_creatures(head);
    // --- UI ---

    int max_width = info.screenwidth-2;
    int max_height = info.screenheight-2;

    screen_rect(
        border,
        (struct area){ .x = 1, .y = 1, .width = max_width, .height = max_height },
        NO_FILL, (struct symbol *) NULL
    );
    screen_rect(
        vline,
        (struct area){ .x = 15, .y = 2, .width = 1, .height = max_height - 2 },
        FILL, NULL
    );

    gotoxy(1, 1);
    _setcursortype(_NOCURSOR);
    _setcursortype(0);
    print_creature_stats(player, 0);
    enum action act = action_for(read_key());
    while (act != ACT_QUIT) {
        //clear_sidebar(6, 13);
        erase_creatures(head, screen.screenmap, terrain);
        player = handle_input(act, player, (char *)screen.screenmap);
        // head = update_creature_position(head, player->name, player->x, player->y);
        draw_creatures(head);
        textcolor(WHITE);
        // putchxy(2, 2, ':');
        // // printf("test\n");
        update_creature_stat_display(player, 0);
        _setcursortype(_NOCURSOR);
        _setcursortype(0);
        act =  action_for(read_key());
    }

    // Free all allocated memory
    free_creatures(critters);

    _setcursortype(_NORMALCURSOR);
    textcolor(YELLOW);
    clrscr();
}