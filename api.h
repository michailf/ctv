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

char *etvnet_get_activation_code();
void etvnet_authorize(const char *activation_code);

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

struct movie_list *etvnet_load_favorites();
char *etvnet_get_stream_url(int object_id, int bitrate);
struct movie_entry *etvnet_get_movie(int container_id, int idx);
