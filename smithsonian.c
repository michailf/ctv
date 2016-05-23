#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <limits.h>
#include "provider.h"
#include "common/log.h"

static struct provider *provider;

static struct movie_list *
load(struct provider *p)
{
	char s[2000], *ptr, *title, *url, fname[PATH_MAX];
	int id = 1, rc;

	snprintf(fname, PATH_MAX-1, "%s/.cache/etvcc/smithsonian.txt", getenv("HOME"));
	snprintf(s, 1999, "python %s/src/ctv/smithsonianchannel.py > %s", getenv("HOME"), fname);

	rc = system(s);
	if (rc != 0)
		logfatal("cannot refresh smithsonian channel list");

	FILE *f = fopen(fname, "rt");
	struct movie_list *list = calloc(1, sizeof(struct movie_list));

	while (!feof(f)) {
		ptr = fgets(s, 1999, f);
		if (ptr == NULL)
			break;
		s[strlen(s)-1] = 0;

		if (*ptr != ' ') {
			title = strdup(strchr(s, ' ') + 1);
		} else if (strncmp(s, "    url: ", 9) == 0) {
			url = &s[9];
//			printf("title: [%s]\n", title);
//			printf("url:   [%s]\n", url);
//			printf("\n");

			list->items = realloc(list->items, (list->count + 1) * sizeof(struct movie_entry));
			struct movie_entry *e = calloc(1, sizeof(struct movie_entry));
			e->id = id++;
			e->name = title;
			e->stream_url = strdup(url);
			list->items[list->count] = e;
			list->count++;
		}
	}

	return list;
}

struct provider *
smithsonian_get_provider() {
	provider = calloc(1, sizeof(struct provider));

	provider->name = strdup("smithsonian");
	provider->load = load;

	return provider;
}
