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

static struct movie_list *
parse_favorites(json_object *root)
{
	struct movie_list *list = calloc(1, sizeof(struct movie_list));

	json_object *data, *bookmarks, *bookmark;
	json_bool jres;
	int bookmarks_count;

	int status = get_int(root, "status_code");
	if (status != 200)
		err(1, "invalid status %d", status);

	jres = json_object_object_get_ex(root, "data", &data);
	if (jres == FALSE)
		err(1, "Cannot get data");

	jres = json_object_object_get_ex(data, "bookmarks", &bookmarks);
	if (jres == FALSE)
		err(1, "Cannot get bookmarks");

	bookmarks_count = json_object_array_length(bookmarks);

	for (int i = 0; i < bookmarks_count; i++) {
		bookmark = json_object_array_get_idx(bookmarks, i);
		if (bookmark == NULL)
			err(1, "Cannot get bookmark[%d]", i);

		struct movie_entry *e = malloc(sizeof(struct movie_entry));
		e->sel = 0;
		e->id = get_int(bookmark, "id");
		e->children_count = get_int(bookmark, "children_count");
		e->name = strdup(get_str(bookmark, "name"));
		e->description = strdup(get_str(bookmark, "description"));
		e->on_air = strdup(get_str(bookmark, "on_air"));
		append_movie(list, e);
	}

	return list;
}

struct movie_list *
load_favorites()
{
	char url[500];
	json_bool jres;
	json_object *obj, *folder;

	snprintf(url, 499, "%s/video/bookmarks/folders.json?per_page=20", api_root);
	json_object *root = get_cached(url, "favorites");

	jres = json_object_object_get_ex(root, "data", &obj);
	if (jres == FALSE)
		err(1, "Cannot get data");

	jres = json_object_object_get_ex(obj, "folders", &obj);
	if (jres == FALSE)
		err(1, "Cannot get folders");

	int folders_count = json_object_array_length(obj);
	int folder_id = 0;

	for (int i = 0; i < folders_count; i++) {
		folder = json_object_array_get_idx(obj, i);
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

	FILE *f = fopen("/Users/svoilokov/.config/etvcc/access_token.txt", "rt");
	access_token = malloc(100);
	fgets(access_token, 100, f);
	access_token[strlen(access_token)-1] = 0;
	fclose(f);

	f = fopen("/Users/svoilokov/.config/etvcc/refresh_token.txt", "rt");
	refresh_token = malloc(100);
	fgets(refresh_token, 100, f);
	refresh_token[strlen(refresh_token)-1] = 0;
	fclose(f);
}


// def get_stream_url(object_id, max_avail_bitrate):
// 	url = api_root + 'video/media/%d/watch.json?format=mp4&protocol=hls&bitrate=%d' % \
// 		(object_id, max_avail_bitrate)
// 	try:
//         	a = get_cached(url, 'stream')
// 	except:
// 		print 'exception while getting url'
// 		time.sleep(4)
//         	return False, 'exception while getting url'
// 	if a['status_code'] != 200:
// 		print a
// 		return False, 'cannot get video url'
// 	return True, a['data']['url']
// 
// def get_children(child_id, page):
// 	url = api_root + '/video/media/%d/children.json?page=%d&per_page=20&order_by=on_air' % \
// 		(child_id, page)
// 	a = get_cached(url, 'children')
// 	return a['data']['children']
