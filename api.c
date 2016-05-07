#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <json-c/json.h>
#include <sys/stat.h>
#include "common/net.h"
#include "common/fs.h"
#include "api.h"

static const char client_id[] = "a332b9d61df7254dffdc81a260373f25592c94c9";
static const char client_secret[] = "744a52aff20ec13f53bcfd705fc4b79195265497";
static const char token_url[] = "https://accounts.etvnet.com/auth/oauth/token";
static const char code_url[] = "https://accounts.etvnet.com/auth/oauth/device/code";
static const char api_root[] = "https://secure.etvnet.com/api/v3.0/";
static const char scope_encoded[] =
	"com.etvnet.media.browse%20"
	"com.etvnet.media.watch%20"
	"com.etvnet.media.bookmarks%20"
	"com.etvnet.media.history%20"
	"com.etvnet.media.live%20"
	"com.etvnet.media.fivestar%20"
	"com.etvnet.media.comments%20"
	"com.etvnet.persons%20"
	"com.etvnet.notifications";

static char *cache_path;
static char *access_token;
static char *refresh_token;
static char last_error[4096];

const char *
etvnet_error()
{
	return (etvnet_errno > 0) ? last_error : NULL;
}

int etvnet_errno = 0;

static int
fetch(const char *url, const char *fname)
{
	struct httpreq_opts opts = {
		.resp_fname = fname,
		.error = last_error,
		.error_size = 4095
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
		return -1;

	return json_object_get_int(child_obj);
}

void
etvnet_authorize(const char *activation_code)
{
	char url[1000];
	char fname[PATH_MAX];
	int rc;
	json_object *root;

	/* create .local dir */

	snprintf(fname, PATH_MAX-1, "%s/.local", getenv("HOME"));
	if (!exists(fname)) {
		rc = mkdir(fname, 0700);
		sprintf(last_error, "cannot create dir %s. Error: %d", fname, rc);
		etvnet_errno = 1;
		return;
	}

	snprintf(fname, PATH_MAX-1, "%s/.local/etvcc", getenv("HOME"));
	if (!exists(fname)) {
		rc = mkdir(fname, 0700);
		sprintf(last_error, "cannot create dir %s. Error: %d", fname, rc);
		etvnet_errno = 1;
		return;
	}

	/* fetch tokens */

	strcpy(url, token_url);
	strcat(url, "?client_id=");
	strcat(url, client_id);
	strcat(url, "&client_secret=");
	strcat(url, client_secret);
	strcat(url, "&scope=");
	strcat(url, scope_encoded);

	if (activation_code != NULL) {
		strcat(url, "&grant_type=http://oauth.net/grant_type/device/1.0");
		strcat(url, "&code=");
		strcat(url, activation_code);

	} else {
		strcat(url, "&grant_type=refresh_token");
		strcat(url, "&refresh_token=");
		strcat(url, refresh_token);
	}

	snprintf(fname, PATH_MAX-1, "%s/.local/etvcc/token.json", getenv("HOME"));

	rc = fetch(url, fname);
	if (rc != 0) {
		sprintf(last_error, "cannot authorize. Error: %d", rc);
		etvnet_errno = 1;
		return;
	}

	/* parse json */

	root = json_object_from_file(fname);
	if (root == NULL) {
		sprintf(last_error, "cannot load %s", fname);
		etvnet_errno = 1;
		return;
	}

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

	if (rc != 0) {
		etvnet_errno = 1;
		return NULL;
	}

	root = json_object_from_file(fname);
	error = get_str(root, "error");

	if (rc == 0 && root != NULL && error == NULL)
		return root;

	sleep(2);
	etvnet_authorize(NULL);
	get_full_url(url, full_url);
	rc = fetch(url, fname);

	root = json_object_from_file(fname);
	error = get_str(root, "error");

	if (rc != 0 || root == NULL) {
		sprintf(last_error, "cannot load %s. rc: %d", fname, rc);
		etvnet_errno = 1;
		return NULL;
	}

	if (error != NULL) {
		remove(fname);
		sprintf(last_error, "api error: %s", error);
		etvnet_errno = 1;
		return NULL;
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
	if (jres == FALSE) {
		sprintf(last_error, "Cannot get data for %s", name);
		etvnet_errno = 1;
		return  NULL;
	}

	jres = json_object_object_get_ex(data, name, &obj);
	if (jres == FALSE) {
		sprintf(last_error, "Cannot get data/%s", name);
		etvnet_errno = 1;
		return NULL;
	}

	return obj;
}

static struct movie_list *
parse_favorites(json_object *root)
{
	int i;

	json_object *bookmarks, *bookmark;
	int bookmarks_count;

	int status = get_int(root, "status_code");
	if (status != 200) {
		sprintf(last_error, "invalid status %d", status);
		etvnet_errno = 1;
		return NULL;
	}

	struct movie_list *list = calloc(1, sizeof(struct movie_list));
	bookmarks = get_data(root, "bookmarks");
	bookmarks_count = json_object_array_length(bookmarks);

	for (i = 0; i < bookmarks_count; i++) {
		bookmark = json_object_array_get_idx(bookmarks, i);
		if (bookmark == NULL) {
			sprintf(last_error, "Cannot get bookmark[%d]", i);
			etvnet_errno = 1;
			return NULL;
		}

		struct movie_entry *e = create_movie(bookmark);
		append_movie(list, e);
	}

	return list;
}

struct movie_list *
etvnet_load_favorites()
{
	char url[500];
	json_object *root, *folders, *folder;
	int i;

	etvnet_errno = 0;
	snprintf(url, 499, "%s/video/bookmarks/folders.json?per_page=20", api_root);

	root = get_cached(url, "favorites");
	if (etvnet_errno != 0)
		return NULL;

	folders = get_data(root, "folders");
	int folders_count = json_object_array_length(folders);
	int folder_id = 0;

	for (i = 0; i < folders_count; i++) {
		folder = json_object_array_get_idx(folders, i);
		if (folder == NULL) {
			sprintf(last_error, "Cannot get folder[%d]", i);
			etvnet_errno = 1;
			return  NULL;
		}

		const char *title = get_str(folder, "title");
		if (strcmp(title, "serge") == 0) {
			folder_id = get_int(folder, "id");
			break;
		}
	}

	if (folder_id == 0) {
		sprintf(last_error, "cannot get my favorite folder");
		etvnet_errno = 1;
		return NULL;
	}

	root = fetch_favorite(folder_id);
	if (etvnet_errno != 0)
		return NULL;

	struct movie_list *list = parse_favorites(root);

	return list;
}

void
etvnet_init()
{
	char fname[PATH_MAX];

	asprintf(&cache_path, "%s/.cache/etvcc/", getenv("HOME"));

	snprintf(fname, PATH_MAX-1, "%s/.local/etvcc/token.json", getenv("HOME"));

	if (exists(fname)) {
		json_object *root = json_object_from_file(fname);
		access_token = strdup(get_str(root, "access_token"));
		refresh_token = strdup(get_str(root, "refresh_token"));
	} else {
		sprintf(last_error, "Player is not activated on etvnet.com.");
		etvnet_errno = 1;
	}
}


char *
etvnet_get_stream_url(int object_id, int bitrate)
{
	char url[1000];
	char name[100];
	json_object *root;
	json_object *obj;
	json_bool jres;

	etvnet_errno = 0;
	snprintf(url, 499, "%svideo/media/%d/watch.json?format=mp4&protocol=hls&bitrate=%d",
		 api_root, object_id, bitrate);
	snprintf(name, 99, "stream-%d", object_id);

	root = get_cached(url, name);
	if (etvnet_errno != 0)
		return NULL;

	int status = get_int(root, "status_code");
	if (status != 200) {
		sprintf(last_error, "get stream status: %d", status);
		etvnet_errno = 1;
		return NULL;
	}

	jres = json_object_object_get_ex(root, "data", &obj);
	if (jres == FALSE) {
		sprintf(last_error, "Cannot get stream data");
		etvnet_errno = 1;
		return  NULL;
	}

	char *stream_url = strdup(get_str(obj, "url"));
	json_object_put(root);
	return stream_url;
}

struct movie_entry *
etvnet_get_movie(int container_id, int idx)
{
	char url[500];
	char name[100];
	json_object *root, *children, *child;

	etvnet_errno = 0;
	int page = (idx / 20) + 1;
	int pos_on_page = idx - (page - 1) * 20;

	snprintf(url, 499, "%s/video/media/%d/children.json?page=%d&per_page=20&order_by=on_air",
		 api_root, container_id, page);

	snprintf(name, 99, "child-%d-%d", container_id, idx);

	root = get_cached(url, name);
	if (etvnet_errno != 0)
		return NULL;

	children = get_data(root, "children");
	int children_count = json_object_array_length(children);

	if (pos_on_page >= children_count) {
		sprintf(last_error, "cannot get child by idx %d", idx);
		etvnet_errno = 1;
		return NULL;
	}

	child = json_object_array_get_idx(children, pos_on_page);
	struct movie_entry *e = create_movie(child);

	return e;
}

char *
etvnet_get_activation_code()
{
	char url[1000];
	char fname[PATH_MAX];
	int rc;
	json_object *root;

	strcpy(url, code_url);
	strcat(url, "?client_id=");
	strcat(url, client_id);
	strcat(url, "&client_secret=");
	strcat(url, client_secret);
	strcat(url, "&scope=");
	strcat(url, scope_encoded);

	snprintf(fname, PATH_MAX-1, "%sactivation.json", cache_path);
	rc = fetch(url, fname);
	if (rc != 0) {
		sprintf(last_error, "cannot get activation code. Error: %d", rc);
		etvnet_errno = 1;
		return NULL;
	}

	root = json_object_from_file(fname);
	if (root == NULL) {
		sprintf(last_error, "cannot load %s", fname);
		etvnet_errno = 1;
		return NULL;
	}

	const char *user_code = get_str(root, "user_code");
	if (user_code == NULL ) {
		sprintf(last_error, "no activation.user_code.");
		etvnet_errno = 1;
		return NULL;
	}

	return strdup(user_code);
}
