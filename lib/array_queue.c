#include "array_queue.h"
#include <string.h>


/* 每一个数组实现的队列都应该多分配一个数组元素，
	以方便区分队列空和满的情况 */
#define ARRSIZE(memsz, nmem)		\
		(sizeof(struct arrqueue) + memsz * (nmem + 1) - 1)
#define MEMSTART(aq, memsz, i)		\
		(aq->arr + memsz * i)


struct arrqueue* arrq_create(size_t nmem, size_t memsz) {
	struct arrqueue* aq;

	if ((aq = malloc(ARRSIZE(memsz, nmem))) == NULL)
		return NULL;
	aq->iget = aq->iput = 0;
	aq->nmem = nmem + 1;
	return aq;
}


void arrq_destroy(struct arrqueue* aq) {
	free(aq);
}


int arrq_size(const struct arrqueue* aq) {
	if (aq->iput >= aq->iget)
		return aq->iput - aq->iget;
	return aq->nmem - (aq->iget - aq->iput);
}


int arrq_empty(const struct arrqueue* aq) {
	return aq->iget == aq->iput;
}


int arrq_full(const struct arrqueue* aq) {
	return ((aq->iput + 1) % aq->nmem) == aq->iget;
}


int arrq_enqueue(struct arrqueue* aq, const void* val, size_t memsz) {
	if (arrq_full(aq)) return -1;
	memcpy(MEMSTART(aq, memsz, aq->iput++), val, memsz);
	aq->iput %= aq->nmem;
	return 0;
}


int arrq_dequeue(struct arrqueue* aq, void* val, size_t memsz) {
	if (arrq_empty(aq)) return -1;
	memcpy(val, MEMSTART(aq, memsz, aq->iget++), memsz);
	aq->iget %= aq->nmem;
	return 0;
}
