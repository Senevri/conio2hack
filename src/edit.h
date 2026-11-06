// header file
struct list{
	wchar_t * line;
	int line_number;
	struct list * next;
	struct list * prev;
};

struct cursor{
	int x, y;
};

struct list* init_node();
struct list* store_and_next_node(struct list *l, const wchar_t * buf);
struct list* load_file_by_filename(struct list*, const char*);

struct line {
    wchar_t *text;
    size_t length;
    size_t capacity;
    struct line *next;
};

struct document {
    struct line *first;
    struct line *current;
    size_t cursor_x;
    size_t cursor_y;
};