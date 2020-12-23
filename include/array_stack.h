#ifndef STATCK43SX_H_
#define STATCK43SX_H_

#include <stdlib.h>
#include <stddef.h>

/* 使用数组实现的栈 */

struct arrstack {
	int iget, iput;
	size_t nmem;
	void* arr;
};


struct arrstack* arrstack_create(size_t cap, size_t memsz);
void arrstack_destroy(struct arrstack* astack);
int arrstack_size(const struct arrstack* astack);
int arrstack_empty(const struct arrstack* astack);
int arrstack_push(struct arrstack* astack, const void* val, size_t memsz);
int arrstack_pop(struct arrstack* astack, void* val, size_t memsz);
int arrstack_top(const struct arrstack* astack, void* val, size_t memsz);



#endif // !STATCK43SX_H_
