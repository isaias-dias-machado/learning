#ifndef REFCY_H
#define REFCY_H

#include <stdint.h>
#include <stddef.h>

struct refcy_hdr {
	uint32_t count;
	uint32_t buf_len;
	uint64_t **weak_list;
	unsigned char buf[];
};

#define REFCY_HDR(obj) ((struct refcy_hdr *) obj - 1)
#define REFCY_HDR_LEN (sizeof(struct refcy_hdr))

void * refcy_calloc(int n, size_t size);
void * refcy_realloc(void *p, size_t size);
void * refcy_malloc(size_t size);
void * refcy_free(size_t size);

int refcy_retain(const void *p);
int refcy_release(const void *p);


#endif
