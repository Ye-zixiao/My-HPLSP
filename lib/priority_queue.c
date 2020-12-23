#include "priority_queue.h"
#include <assert.h>
#include <string.h>


//函数宏中的参数最好都用括号包起来
#define VOIDARR_MEMSTART(arr, i, memsz)		\
		(void*)((arr) + (i) * (memsz))
#define ARRSIZE(nmem, memsz)		((nmem) * (memsz))



static void swap(void* arr, int i, int j, size_t memsz) {
	static void* temp = NULL;
	static size_t oldsz = 0;

	if (oldsz != memsz) {
		if (temp == NULL)
			assert((temp = malloc(memsz)) != NULL);
		else
			assert((temp = realloc(temp, memsz)) != NULL);
		oldsz = memsz;
	}
	memcpy(temp, VOIDARR_MEMSTART(arr, i, memsz), memsz);
	memcpy(VOIDARR_MEMSTART(arr, i, memsz), VOIDARR_MEMSTART(arr, j, memsz), memsz);
	memcpy(VOIDARR_MEMSTART(arr, j, memsz), temp, memsz);
}




struct priority_queue* pq_create(size_t nmem, size_t memsz, Comp* pf) {
	struct priority_queue* pq;

	if ((pq = malloc(sizeof(struct priority_queue))) == NULL)
		return NULL;
	if ((pq->arr = malloc(ARRSIZE(nmem, memsz))) == NULL) {
		free(pq); return NULL;
	}
	pq->iput = 0;
	pq->nmem = nmem;
	pq->memsz = memsz;
	pq->pf = pf;
	return pq;
}


void pq_destroy(struct priority_queue* pq) {
	free(pq->arr);
	free(pq);
}


int pq_size(const struct priority_queue* pq) {
	return !pq ? 0 : pq->iput;
}


int pq_empty(const struct priority_queue* pq) {
	return pq_size(pq) == 0;
}


/* 上浮函数，用于新加入的元素，使得它们上浮到合适的位置 */
static void swim(struct priority_queue* pq, int k) {
	int j;
	while ((j = (k - 1) / 2) >= 0) {
		//greater_n_equal
		if (pq->pf(VOIDARR_MEMSTART(pq->arr, j, pq->memsz),
			VOIDARR_MEMSTART(pq->arr, k, pq->memsz)))
			break;
		swap(pq->arr, j, k, pq->memsz);
		k = j;
	}
}


/* 下沉函数，用于删除堆顶元素后交换过来的元素，使其下沉到合适的位置 */
static void sink(struct priority_queue* pq, int k) {
	int j, N = pq_size(pq);
	while ((j = 2 * k + 1) < N) {
		if (j < N - 1 &&
			!pq->pf(VOIDARR_MEMSTART(pq->arr, j, pq->memsz), VOIDARR_MEMSTART(pq->arr, j + 1, pq->memsz)))
			j++;
		if (pq->pf(VOIDARR_MEMSTART(pq->arr, k, pq->memsz), VOIDARR_MEMSTART(pq->arr, j, pq->memsz)))
			break;
		swap(pq->arr, j, k, pq->memsz);
		k = j;
	}
}


static void resize(struct priority_queue* pq, size_t nmem) {
	assert((pq->arr = realloc(pq->arr, nmem * pq->memsz)) != NULL);
	pq->nmem = nmem;
}


int pq_push(struct priority_queue* pq, const void* val, size_t memsz) {
	if (!pq || !val || memsz <= 0) return -1;

	if (pq_size(pq) == pq->nmem)
		resize(pq, pq->nmem * 2);
	memcpy(VOIDARR_MEMSTART(pq->arr, pq->iput, memsz), val, memsz);
	swim(pq, pq->iput++);
	return 0;
}


int pq_pop(struct priority_queue* pq, void* val, size_t memsz) {
	if (!pq || pq_empty(pq) || !val || memsz <= 0) return -1;

	memcpy(val, VOIDARR_MEMSTART(pq->arr, 0, pq->memsz), memsz);
	swap(pq->arr, 0, pq->iput-- - 1, pq->memsz);
	sink(pq, 0);
	if (pq_size(pq) < pq->nmem / 2 && pq->nmem > 32)
		resize(pq, pq->nmem / 2);
	return 0;
}


int pq_top(const struct priority_queue* pq, void* val, size_t memsz) {
	if (!pq || pq_empty(pq) || !val || memsz <= 0) return -1;
	memcpy(val, VOIDARR_MEMSTART(pq->arr, 0, memsz), memsz);
	return 0;
}