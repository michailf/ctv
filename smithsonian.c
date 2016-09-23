#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "provider.h"
#include "common/log.h"
#include "common/fs.h"
#include "common/net.h"
#include "common/regexp.h"
#include "util.h"

static struct provider *provider;
static char last_error[4096];

static int
fetch(const char *url, const char *name, struct buf *buf)
{
	int rc;
	char fname[PATH_MAX];

	snprintf(fname, PATH_MAX-1, "%s/.cache/etvcc/smithsonian-%s.txt", getenv("HOME"), name);
	if (!expired(fname, 2*24*3600))
		return read_text(fname, buf);

	struct httpreq_opts opts = {
		.resp_fname = fname,
		.error = last_error,
		.error_size = 4096
	};

	rc = httpreq(url, NULL, &opts);
	if (rc != 0) {
		provider->error_number = rc;
		return rc;
	}

	return read_text(fname, buf);
}

static void
replace(char *s, char what, char with)
{
	while (*s) {
		if (*s == what)
			*s = with;
		s++;
	}
}

static const char *
smith_error()
{
	return last_error;
}

static struct movie_list *
smith_load(struct provider *p)
{
	const int N = 20;
	regex_t rex_episode, rex_title, rex_bcid;
	struct buf episodes_html;
	struct buf title_html;
	regmatch_t m[N];
	char *chunks[N];
	char full_url[1024];
	char name[1024];
	int rc, i;

	regex_compile(&rex_episode, "href=\"([^\"]+)\".*srcset=\"([^\"]+)\"");
	regex_compile(&rex_title, "property=\"og:title\" content=\"([^\"]+)\"");
	regex_compile(&rex_bcid, "data-bcid=\"([^\"]+)\"");

	buf_init(&episodes_html);
	memset(chunks, 0, sizeof(char*) * N);

	rc = fetch("http://www.smithsonianchannel.com/full-episodes", "episodes", &episodes_html);
	if (rc != 0) {
		provider->error_number = rc;
		return NULL;
	}

	rc = match_chunks(episodes_html.s, N, m, "data-premium=\"", "</li>");
	if (rc != 0) {
		provider->error_number = 1;
		snprintf(last_error, 4095, "match_chunks: %d", rc);
		return NULL;
	}

	split_chunks(episodes_html.s, N, m, chunks);

	struct movie_list *list = calloc(1, sizeof(struct movie_list));
	list->items = calloc(N, sizeof(struct movie_entry));

	for (i = 0; i < N && chunks[i] != NULL; i++) {
		rc = regexec(&rex_episode, chunks[i], 4, m, 0);
		if (rc != 0) {
			provider->error_number = 1;
			snprintf(last_error, 4095, "rex_episode: %d, i: %d, chunk: %s", rc, i, chunks[i]);
			return NULL;
		}

		const char *url = &chunks[i][m[1].rm_so];
		char *end = &chunks[i][m[1].rm_eo];
		*end = 0;

		strcpy(full_url, "http://www.smithsonianchannel.com");
		strcat(full_url, url);

		strcpy(name, url);
		strcat(name, "-title");
		replace(name, '/', '-');

		buf_init(&title_html);
		rc = fetch(full_url, name, &title_html);
		if (rc != 0) {
			provider->error_number = 1;
			return NULL;
		}

		struct movie_entry *e = calloc(1, sizeof(struct movie_entry));

		rc = regexec(&rex_title, title_html.s, 2, m, 0);
		if (rc != 0) {
			provider->error_number = 1;
			return NULL;
		}
		asprintf(&e->name, "%.*s", (int)(m[1].rm_eo - m[1].rm_so), &title_html.s[m[1].rm_so]);

		rc = regexec(&rex_bcid, title_html.s, 2, m, 0);
		if (rc != 0) {
			provider->error_number = 1;
			return NULL;
		}

		const char *bcid = &title_html.s[m[1].rm_so];
		end = &title_html.s[m[1].rm_eo];
		*end = 0;

		asprintf(&e->stream_url,
			 "http://c.brightcove.com/services/mobile/streaming"
			 "/index/master.m3u8?videoId=%s&pubId=1466806621001", bcid);
		
		
		buf_clean(&title_html);

		e->id = i;
		list->items[list->count] = e;
		list->count++;
	}

	return list;
}

struct provider *
smithsonian_get_provider() {
	provider = calloc(1, sizeof(struct provider));

	provider->name = strdup("smithsonian");
	provider->load = smith_load;
	provider->error = smith_error;

	return provider;
}
