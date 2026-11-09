#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#include <assert.h>
#include <err.h>
#include <errno.h>
#include <pthread.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>

struct block
{
    size_t size; // taille des donnees (sans les metadonnees)
    int is_free; // 1 = libre, 0 = occupe
    struct block *next; // bloc suivant
    struct block *prev; // bloc precedent (pour la fusion au free)
};

struct linked_list
{
    struct block *head;
    struct block *tail;
};

struct block *init_page(size_t size);
struct block *alloc_block(struct block *head, size_t size);
int are_contiguous(struct block *a, struct block *b);
struct block *free_block(struct block *b);

#endif /* ! LINKED_LIST_H */
