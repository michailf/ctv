struct movie_entry {
	int id;
	char *name;
	char *description;
	char *on_air;
	int children_count;
	int sel;             /* current child */
};

struct movie_list {
	int count;
	int sel;             /* selected index */
	struct movie_entry **items;
};

void api_init(const char *cache_dir);
struct movie_list *load_favorites();
