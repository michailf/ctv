#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <err.h>
#include <json-c/json.h>
#include "common/net.h"
#include "common/fs.h"
#include "api.h"

static const char client_id[] = "a332b9d61df7254dffdc81a260373f25592c94c9";
static const char client_secret[] = "744a52aff20ec13f53bcfd705fc4b79195265497";
static const char token_url[] = "https://accounts.etvnet.com/auth/oauth/token";
static const char api_root[] = "https://secure.etvnet.com/api/v3.0/";
static const char scope[] =
	"com.etvnet.media.browse com.etvnet.media.watch "
	"com.etvnet.media.bookmarks com.etvnet.media.history "
	"com.etvnet.media.live com.etvnet.media.fivestar com.etvnet.media.comments "
	"com.etvnet.persons com.etvnet.notifications";

static char *cache_path;
static char *access_token;
static char *refresh_token;

static int
fetch(const char *url, const char *fname)
{
	struct httpreq_opts opts = {
		.resp_fname = fname
	};

	int rc = httpreq(url, NULL, &opts);
	return rc;
}

static void
get_full_url(const char *url, char *full_url)
{
	strcpy(full_url, url);
	strcat(full_url, "&access_token=");
	strcat(full_url, access_token);
}

static const char *
get_str(json_object *obj, const char *name)
{
	json_object *child_obj;
	json_bool jres;

	jres = json_object_object_get_ex(obj, name, &child_obj);
	if (jres == FALSE)
		return NULL;

	const char *str = json_object_get_string(child_obj);
	return str;
}

static int
get_int(json_object *obj, const char *name)
{
	json_object *child_obj;
	json_bool jres;

	jres = json_object_object_get_ex(obj, name, &child_obj);
	if (jres == FALSE)
		err(1, "Cannot get %s", name);

	return json_object_get_int(child_obj);
}

static void
authorize()
{
	char url[1000];
	char fname[PATH_MAX];
	int rc;
	json_object *root;

	strcpy(url, token_url);
	strcat(url, "?");
	strcat(url, "client_id=");
	strcat(url, client_id);
	strcat(url, "&client_secret=");
	strcat(url, client_secret);
	strcat(url, "&grant_type=refresh_token");
	strcat(url, "&refresh_token=");
	strcat(url, refresh_token);
	strcat(url, "&scope=");
	strcat(url, scope);

	snprintf(fname, PATH_MAX-1, "%s/token.json", cache_path);
	rc = fetch(url, fname);
	if (rc != 0)
		err(1, "cannot authorize. Error: %d", rc);

	root = json_object_from_file(fname);
	if (root == NULL)
		err(1, "cannot load %s", fname);

	if (access_token != NULL)
		free(access_token);

	if (refresh_token != NULL)
		free(refresh_token);

	access_token = strdup(get_str(root, "access_token"));
	refresh_token = strdup(get_str(root, "refresh_token"));
}

static json_object *
get_cached(const char *url, const char *name)
{
	char fname[PATH_MAX];
	char full_url[500];
	int rc = 0;
	const char *error;
	json_object *root;

	snprintf(fname, PATH_MAX-1, "%s%s.json", cache_path, name);

	if (expired(fname, 3600*24)) {
		get_full_url(url, full_url);
		rc = fetch(full_url, fname);
	}

	root = json_object_from_file(fname);
	error = get_str(root, "error");

	if (rc == 0 && root != NULL && error == NULL)
		return root;

	sleep(2);
	authorize();
	get_full_url(url, full_url);
	rc = fetch(url, fname);

	root = json_object_from_file(fname);
	error = get_str(root, "error");

	if (rc != 0 || root == NULL)
		err(1, "cannot load %s. rc: %d", fname, rc);

	if (error != NULL) {
		remove(fname);
		err(1, "api error: %s", error);
	}

	return root;
}

json_object *
fetch_favorite(int folder_id)
{
	char url[500];
	char name[100];

	snprintf(url, 499, "%s/video/bookmarks/folders/%d/items.json?per_page=20", api_root, folder_id);
	snprintf(name, 99, "fav-%d", folder_id);
	json_object *root = get_cached(url, name);

	return root;
}

static void
append_movie(struct movie_list *list, struct movie_entry *e)
{
	list->items = realloc(list->items, sizeof (struct movie_entry) * (list->count + 1));
	list->items[list->count] = e;
	list->count++;
}

static struct movie_entry *
create_movie(json_object *obj)
{
	struct movie_entry *e = malloc(sizeof(struct movie_entry));

	e->sel = 0;
	e->id = get_int(obj, "id");
	e->children_count = get_int(obj, "children_count");
	e->name = strdup(get_str(obj, "name"));
	e->description = strdup(get_str(obj, "description"));
	e->on_air = strdup(get_str(obj, "on_air"));

	return e;
}

static json_object *
get_data(json_object *root, const char *name)
{
	json_object *data, *obj;
	json_bool jres;

	jres = json_object_object_get_ex(root, "data", &data);
	if (jres == FALSE)
		err(1, "Cannot get data");

	jres = json_object_object_get_ex(data, name, &obj);
	if (jres == FALSE)
		err(1, "Cannot get data/%s", name);

	return obj;
}

static struct movie_list *
parse_favorites(json_object *root)
{
	struct movie_list *list = calloc(1, sizeof(struct movie_list));

	json_object *bookmarks, *bookmark;
	int bookmarks_count;

	int status = get_int(root, "status_code");
	if (status != 200)
		err(1, "invalid status %d", status);

	bookmarks = get_data(root, "bookmarks");
	bookmarks_count = json_object_array_length(bookmarks);

	for (int i = 0; i < bookmarks_count; i++) {
		bookmark = json_object_array_get_idx(bookmarks, i);
		if (bookmark == NULL)
			err(1, "Cannot get bookmark[%d]", i);

		struct movie_entry *e = create_movie(bookmark);
		append_movie(list, e);
	}

	return list;
}

struct movie_list *
load_favorites()
{
	char url[500];
	json_bool jres;
	json_object *root, *folders, *folder;

	snprintf(url, 499, "%s/video/bookmarks/folders.json?per_page=20", api_root);

	root = get_cached(url, "favorites");
	folders = get_data(root, "folders");
	int folders_count = json_object_array_length(folders);
	int folder_id = 0;

	for (int i = 0; i < folders_count; i++) {
		folder = json_object_array_get_idx(folders, i);
		if (folder == NULL)
			err(1, "Cannot get folder[%d]", i);

		const char *title = get_str(folder, "title");
		if (strcmp(title, "serge") == 0) {
			folder_id = get_int(folder, "id");
			break;
		}
	}

	if (folder_id == 0)
		err(1, "cannot get my favorite folder");

	root = fetch_favorite(folder_id);
	struct movie_list *list = parse_favorites(root);
	return list;
}

void
api_init(const char *cache_dir)
{
	cache_path = strdup(cache_dir);
	char fname[PATH_MAX];

	snprintf(fname, PATH_MAX-1, "%s/.config/etvcc/access_token.txt", getenv("HOME"));
	FILE *f = fopen(fname, "rt");
	access_token = malloc(100);
	fgets(access_token, 100, f);
	access_token[strlen(access_token)-1] = 0;
	fclose(f);

	snprintf(fname, PATH_MAX-1, "%s/.config/etvcc/refresh_token.txt", getenv("HOME"));
	f = fopen(fname, "rt");
	refresh_token = malloc(100);
	fgets(refresh_token, 100, f);
	refresh_token[strlen(refresh_token)-1] = 0;
	fclose(f);
}


char *
get_stream_url(int object_id, int bitrate)
{
	char url[1000];
	char name[100];
	json_object *root;
	json_object *obj;
	json_bool jres;

	snprintf(url, 499, "%svideo/media/%d/watch.json?format=mp4&protocol=hls&bitrate=%d",
		 api_root, object_id, bitrate);
	snprintf(name, 99, "stream-%d", object_id);

	root = get_cached(url, name);

	int status = get_int(root, "status_code");
	if (status != 200)
		return NULL;

	jres = json_object_object_get_ex(root, "data", &obj);
	if (jres == FALSE)
		err(1, "Cannot get data");

	char *stream_url = strdup(get_str(obj, "url"));
	json_object_put(root);
	return stream_url;
}

struct movie_entry *
get_child(int container_id, int idx)
{
	char url[500];
	char name[100];
	json_object *root, *children, *child;

	int page = (idx / 20) + 1;
	int pos_on_page = idx - (page - 1) * 20;

	snprintf(url, 499, "%s/video/media/%d/children.json?page=%d&per_page=20&order_by=on_air",
		 api_root, container_id, page);

	snprintf(name, 99, "child-%d-%d", container_id, idx);

	root = get_cached(url, name);
	children = get_data(root, "children");
	int children_count = json_object_array_length(children);

	if (pos_on_page >= children_count)
		err(1, "cannot get child by idx %d", idx);

	child = json_object_array_get_idx(children, pos_on_page);
	struct movie_entry *e = create_movie(child);

	return e;
}
