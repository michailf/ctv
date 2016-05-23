//
//  channel.c
//  ctv
//
//  Created by Serge Voilokov on 5/20/16.
//
//

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include "provider.h"

static struct provider *provider;

static struct movie_list *
load(struct provider *p)
{
	FILE *f = fopen("/Users/svoilokov/src/xtree/ctests/smith.txt", "rt");
	struct movie_list *list = calloc(1, sizeof(struct movie_list));

	char s[2000], *ptr, *title, *url;
	int id = 1;

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
