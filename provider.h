enum stream_format {
	SF_NONE,
	SF_MP4,
	SF_WMV
};

struct movie_entry {
	int id;
	char *name;
	char *description;
	char *on_air;
	int children_count;
	int sel;             /* selected child */
	int bitrate;
	enum stream_format format;
	char *stream_url;
};

struct movie_list {
	int count;
	int sel;             /* selected index */
	struct movie_entry **items;
};

struct provider {
	char *name;

	int error_number;
	const char *(*error)();

	void (*get_activation_code)(char **user_code, char **device_code);
	void (*authorize)(const char *activation_code);

	struct movie_list *(*load)();
	char *(*get_stream_url)(struct movie_entry *e);
	struct movie_entry *(*get_movie)(int parent_id, int idx);
};
