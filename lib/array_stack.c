#include "array_stack.h"
#include <assert.h>
#include <string.h>

#define NMEM_LOWAT (16 * 2)
#define MEMSTART(astack, i, memsz)		\
		(astack->arr + i * memsz)


struct arrstack* arrstack_create(size_t nmem, size_t memsz) {
	struct arrstack* astack;
	
	if ((astack = malloc(sizeof(struct arrstack))) == NULL)
		return NULL;
	if ((astack->arr = malloc(nmem * memsz)) == NULL) {
		free(astack); return NULL;
	}
	astack->iget = -1;
	astack->iput = 0;
	astack->nmem = nmem;
	return astack;
}


void arrstack_destroy(struct arrstack* astack) {
	if (astack) {
		free(astack->arr);
		free(astack);
	}
}


int arrstack_size(const struct arrstack* astack) {
	return !astack ? 0 : astack->iput;
}


int arrstack_empty(const struct arrstack* astack) {
	return arrstack_size(astack) == 0;
}


static int 
arrstack_resize(struct arrstack* astack, size_t nmem, size_t memsz) {
	if (!astack) return -1;
	if ((astack->arr = realloc(astack->arr, nmem * memsz)) == NULL)
		return -1;
	astack->nmem = nmem;
	return 0;
}


int arrstack_push(struct arrstack* astack, const void* val, size_t memsz) {
	if (!astack || !val || memsz <= 0) return -1;

	if (arrstack_size(astack) >= astack->nmem)
		assert(arrstack_resize(astack, astack->nmem * 2, memsz) == 0);
	memcpy(MEMSTART(astack, astack->iput++, memsz), val, memsz);
	astack->iget++;
	return 0;
}


int arrstack_pop(struct arrstack* astack, void* val, size_t memsz) {
	if (!astack || arrstack_empty(astack) || !val || memsz <= 0) return -1;

	memcpy(val, MEMSTART(astack, astack->iget--, memsz), memsz);
	astack->iput--;
	if (arrstack_size(astack) < astack->nmem / 2 && astack->nmem > NMEM_LOWAT)
		assert(arrstack_resize(astack, astack->nmem / 2, memsz) == 0);
	return 0;
}


int arrstack_top(const struct arrstack* astack, void* val, size_t memsz) {
	if (!astack || arrstack_empty(astack) || !val || memsz <= 0) return -1;

	memcpy(val, MEMSTART(astack, astack->iget, memsz), memsz);
	return 0;
}