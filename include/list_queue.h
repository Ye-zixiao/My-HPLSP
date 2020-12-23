#ifndef LIST_QUUEUE32CX_H_
#define LIST_QUUEUE32CX_H_

#include <stdlib.h>
#include <stddef.h>

/* 使用链表实现的队列，支持指定任一类型对象的操作 */

struct listq_node;

struct listqueue {
	struct listq_node* front, * back;
	int size;
};

struct listqueue* listq_create(void);
void listq_destroy(struct listqueue* lq);
int listq_enqueue(struct listqueue* lq, const void* val, size_t memsz);
int listq_dequeue(struct listqueue* lq, void* val, size_t memsz);
int listq_front(const struct listqueue* lq, void* val, size_t memsz);
int listq_back(const struct listqueue* lq, void* val, size_t memsz);
int listq_size(const struct listqueue* lq);
int listq_empty(const struct listqueue* lq);


#endif // !LIST_QUUEUE32CX_H_
