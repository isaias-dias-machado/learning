#include "refcy.h"
#include <stdlib.h>

void * refcy_malloc(size_t size) {
	struct refcy_hdr *p = malloc(REFCY_HDR_LEN + size);
	if (!p)
		return NULL:
	p->buf_len = size;
	p->count = 1;
	p->weak_list = NULL;
	return (void *)(p+1);
}

void * refcy_realloc(void *p, size_t size) {
	struct refcy_hdr *hdr = REFCY_HDR(p);
	return realloc(hdr, sizeof(struct refcy_hdr) + size);
}

void * refcy_calloc(int n, size_t size) {
	size_t total_size = n * size;
	struct refcy_hdr =
}

void * refcy_free(size_t size) {
}

int refcy_retain(const void *p) {
}

int refcy_release(const void *p) {
}

#ifdef _TEST
#include <stdio.h>
#include <assert.h>
struct user_obj {
	int some_data;
	int more_data;
};

int main(void) {
	struct user_obj *s = refcy_malloc(sizeof *s);
	struct refcy_hdr *hdr = REFCY_HDR(s);
	assert(hdr->buf_len == sizeof(struct user_obj));
	assert(hdr->count == 1);
	puts("All tests passed.");
}
#endif
