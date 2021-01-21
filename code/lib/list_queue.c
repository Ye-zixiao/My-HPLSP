#include "list_queue.h"
#include <string.h>


struct listq_node {
	struct listq_node* next;
	char val[1];
};


#define LISTNODESIZE(memsz)		\
		(sizeof(struct listq_node) + memsz - 1)


static struct listq_node* node_create(const void* val, size_t memsz) {
	struct listq_node* lq_node;

	if ((lq_node = malloc(LISTNODESIZE(memsz))) == NULL)
		return NULL;
	lq_node->next = NULL;
	memcpy(lq_node->val, val, memsz);
	return lq_node;
}


static void node_destroy(struct listq_node* lq_node) {
	struct listq_node* p;
	while (lq_node) {
		p = lq_node;
		lq_node = lq_node->next;
		free(p);
	}
}



struct listqueue* listq_create(void) {
	struct listqueue* lq;

	if ((lq = malloc(sizeof(struct listqueue))) == NULL)
		return NULL;
	if ((lq->front = malloc(sizeof(struct listq_node))) == NULL) {
		free(lq); return NULL;
	}
	lq->front->next = NULL;
	lq->back = lq->front;
	lq->size = 0;
	return lq;
}


void listq_destroy(struct listqueue* lq) {
	if (!lq) return;
	node_destroy(lq->front);
	free(lq);
}


int listq_size(const struct listqueue* lq) {
	return !lq ? 0 : lq->size;
}


int listq_empty(const struct listqueue* lq) {
	return listq_size(lq) == 0;
}


int listq_enqueue(struct listqueue* lq, const void* val, size_t memsz) {
	if (!lq) return -1;

	struct listq_node* lq_node;
	if ((lq_node = node_create(val, memsz)) == NULL)
		return -1;
	lq->back->next = lq_node;
	lq->back = lq_node;
	lq->size++;
	return 0;
}


int listq_dequeue(struct listqueue* lq, void* val, size_t memsz) {
	if (listq_empty(lq)) return -1;

	struct listq_node* todelete = lq->front->next;
	memcpy(val, todelete->val, memsz);
	lq->front->next = todelete->next;
	if (lq->size-- == 1)
		lq->back = lq->front;
	free(todelete);
	return 0;
}


int listq_front(const struct listqueue* lq, void* val, size_t memsz) {
	if (listq_empty(lq)) return -1;
	memcpy(val, lq->front->next, memsz);
	return 0;
}


int listq_back(const struct listqueue* lq, void* val, size_t memsz) {
	if (listq_empty(lq)) return -1;
	memcpy(val, lq->back, memsz);
	return 0;
}