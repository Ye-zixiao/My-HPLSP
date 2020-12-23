#ifndef ARRQUEUE43XSF_H_
#define ARRQUEUE43XSF_H_

#include <stdlib.h>
#include <stddef.h>

/**
 * 使用数组实现的队列，支持任一指定类型
 */

struct arrqueue {
	int iput, iget;
	size_t nmem;
	char arr[1];
};


struct arrqueue* arrq_create(size_t nmem, size_t memsz);
void arrq_destroy(struct arrqueue* aq);
int arrq_enqueue(struct arrqueue* aq, const void* val, size_t memsz);
int arrq_dequeue(struct arrqueue* aq, void* val, size_t memsz);
int arrq_size(const struct arrqueue* aq);
int arrq_empty(const struct arrqueue* aq);
int arrq_full(const struct arrqueue* aq);


#endif // !ARRQUEUE43XSF_H_
