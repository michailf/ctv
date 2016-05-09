/*
 * etvnet.com minimal API client for console TV box.
 * Serge Voilokov, 2016
 *
 * Methods:
 *   - activate
 *   - authorize
 *   - load my favorites
 *   - list movie in my favorites folder
 *   - load stream url for a movie
 */

void etvnet_init();
const char *etvnet_error();
extern int etvnet_errno;

void etvnet_get_activation_code(char **user_code, char **device_code);
void etvnet_authorize(const char *activation_code);

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
};

struct movie_list {
	int count;
	int sel;             /* selected index */
	struct movie_entry **items;
};

struct movie_list *etvnet_load_favorites();
char *etvnet_get_stream_url(int object_id, enum stream_format, int bitrate);
struct movie_entry *etvnet_get_movie(int container_id, int idx);
