#include "linked_list.h"

struct block *init_page(size_t size)
{
    size_t total = size + sizeof(struct block);
    total = (total + 4096 - 1) / 4096 * 4096;
    void *mem = mmap(NULL, total, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mem == MAP_FAILED)
        return NULL;

    struct block *b = mem;
    b->size = total - sizeof(struct block);
    b->is_free = 1;
    b->next = NULL;
    b->prev = NULL;

    return b;
}

static struct block *add_page(struct block *head, size_t size)
{
    struct block *new_page = init_page(size);
    if (new_page == NULL)
        return NULL;

    struct block *current_block = head;
    while (current_block->next != NULL)
        current_block = current_block->next;
    current_block->next = new_page;
    new_page->prev = current_block;

    return new_page;
}

struct block *alloc_block(struct block *head, size_t size)
{
    if (head == NULL)
        return NULL;

    struct block *current_block = head;
    while (current_block != NULL
           && !(current_block->is_free == 1 && current_block->size >= size))
        current_block = current_block->next;
    if (current_block == NULL)
    {
        current_block = add_page(head, size);
        if (current_block == NULL)
            return NULL;
    }

    if (current_block->size >= size + sizeof(struct block) + 1)
    {
        struct block *b = (struct block *)((char *)(current_block + 1) + size);
        b->size = current_block->size - size - sizeof(struct block);
        b->is_free = 1;
        b->next = current_block->next;
        b->prev = current_block;
        if (b->next != NULL)
            b->next->prev = b;

        current_block->size = size;
        current_block->next = b;
    }

    current_block->is_free = 0;
    return current_block + 1;
}

static void fusion_block(struct block *b1, struct block *b2)
{
    b1->size += b2->size + sizeof(struct block);

    b1->next = b2->next;
    if (b2->next != NULL)
    {
        b2->next->prev = b1;
    }
}

int are_contiguous(struct block *a, struct block *b)
{
    return (char *)(a + 1) + a->size == (char *)b;
}

struct block *free_block(struct block *b)
{
    if (b == NULL || b->is_free)
        return NULL;
    b->is_free = 1;

    if (b->next != NULL && b->next->is_free && are_contiguous(b, b->next))
        fusion_block(b, b->next);

    if (b->prev != NULL && b->prev->is_free && are_contiguous(b->prev, b))
    {
        fusion_block(b->prev, b);
        b = b->prev;
    }

    return b;
}
