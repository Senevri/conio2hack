// header file
struct list{
	wchar_t * line;
	struct list * next;
};

struct list * init_node();
void store_and_next_node(struct list *l, const wchar_t * buf);
struct list* load_file_by_filename(struct list*, const char*);
