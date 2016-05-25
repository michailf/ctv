#include <stdio.h>
#include <stdlib.h>
#include "provider.h"
#include "smithsonian.h"

int main()
{
	struct provider *p = smithsonian_get_provider();
	struct movie_list *list = p->load();

	for (int i = 0; i < list->count; i++) {
		struct movie_entry *e = list->items[i];
		printf("%s\n", e->name);
		printf("    url: %s\n", e->stream_url);
	}

}
