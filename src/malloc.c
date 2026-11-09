#include <assert.h>
#include <err.h>
#include <errno.h>
#include <pthread.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>

#include "linked_list.h"

static struct block *head = NULL;

__attribute__((visibility("default"))) void *malloc(size_t size)
{
    if (size == 0 || size > SIZE_MAX - sizeof(struct block) - 4096)
        return NULL;
    size = (size + 15) / 16 * 16;
    if (head == NULL)
    {
        head = init_page(size);
        if (head == NULL)
            return NULL;
    }
    return alloc_block(head, size);
}

__attribute__((visibility("default"))) void free(void *ptr)
{
    if (ptr == NULL)
        return;
    struct block *b = free_block((struct block *)ptr - 1);
    if (b == NULL)
        return;
    int free_previous_page = (b->prev == NULL) || !are_contiguous(b->prev, b);
    int free_next_page = (b->next == NULL) || !are_contiguous(b, b->next);
    if (b->is_free && free_previous_page && free_next_page)
    {
        if (b->prev != NULL)
            b->prev->next = b->next;
        if (b->next != NULL)
            b->next->prev = b->prev;
        if (b == head)
            head = b->next;

        munmap(b, b->size + sizeof(struct block));
    }
}

__attribute__((visibility("default"))) void *realloc(void *ptr, size_t size)
{
    if (size == 0 || size > SIZE_MAX - sizeof(struct block) - 4096)
    {
        if (ptr != NULL)
            free(ptr);
        return NULL;
    }

    size = (size + 15) / 16 * 16;

    if (ptr == NULL)
        return malloc(size);

    void *p = malloc(size);
    if (p == NULL)
        return NULL;

    struct block *b = (struct block *)ptr - 1;
    memcpy(p, ptr, b->size < size ? b->size : size);
    free(ptr);
    return p;
}

__attribute__((visibility("default"))) void *calloc(size_t nmemb, size_t size)
{
    size_t new_size;
    if (__builtin_mul_overflow(nmemb, size, &new_size))
        return NULL;

    void *p = malloc(new_size);
    if (p == NULL)
        return NULL;
    memset(p, 0, new_size);

    return p;
}
