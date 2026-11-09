#include <criterion/criterion.h>
#include <stdint.h>
#include <string.h>

#include "../src/linked_list.h"

/* Prototypes des fonctions renommees par -D */
void *my_malloc(size_t size);
void my_free(void *ptr);
void *my_calloc(size_t nmemb, size_t size);
void *my_realloc(void *ptr, size_t size);

#define ALIGNMENT _Alignof(long double)

/* ------------------------------------------------------------------ */
/* init_page                                                          */
/* ------------------------------------------------------------------ */
Test(init_page, basic_page)
{
    struct block *b = init_page(100);
    cr_assert_not_null(b);
    cr_assert_eq(b->is_free, 1);
    cr_assert_null(b->next);
    cr_assert_null(b->prev);
    cr_assert_geq(b->size, 100);
    munmap(b, b->size + sizeof(struct block));
}

Test(init_page, size_is_page_multiple)
{
    struct block *b = init_page(1);
    cr_assert_not_null(b);
    size_t total = b->size + sizeof(struct block);
    cr_assert_eq(total % 4096, 0, "total=%zu n'est pas multiple de 4096",
                 total);
    munmap(b, total);
}

Test(init_page, big_page)
{
    struct block *b = init_page(1024 * 1024);
    cr_assert_not_null(b);
    cr_assert_geq(b->size, 1024 * 1024);
    munmap(b, b->size + sizeof(struct block));
}

/* ------------------------------------------------------------------ */
/* are_contiguous                                                     */
/* ------------------------------------------------------------------ */
Test(are_contiguous, adjacent_blocks)
{
    char buf[256] __attribute__((aligned(16)));
    struct block *a = (struct block *)buf;
    a->size = 32;
    struct block *b = (struct block *)(buf + sizeof(struct block) + 32);
    cr_assert(are_contiguous(a, b));
}

Test(are_contiguous, non_adjacent_blocks)
{
    char buf[512] __attribute__((aligned(16)));
    struct block *a = (struct block *)buf;
    a->size = 32;
    struct block *b = (struct block *)(buf + 400);
    cr_assert_not(are_contiguous(a, b));
}

/* ------------------------------------------------------------------ */
/* malloc                                                             */
/* ------------------------------------------------------------------ */
Test(malloc, returns_non_null)
{
    void *p = my_malloc(42);
    cr_assert_not_null(p);
    my_free(p);
}

Test(malloc, alignment)
{
    for (size_t s = 1; s < 200; s++)
    {
        void *p = my_malloc(s);
        cr_assert_not_null(p);
        cr_assert_eq((uintptr_t)p % ALIGNMENT, 0, "malloc(%zu) mal aligne : %p",
                     s, p);
        my_free(p);
    }
}

Test(malloc, write_whole_block)
{
    size_t size = 1000;
    unsigned char *p = my_malloc(size);
    cr_assert_not_null(p);
    memset(p, 0xAB, size);
    for (size_t i = 0; i < size; i++)
        cr_assert_eq(p[i], 0xAB);
    my_free(p);
}

Test(malloc, two_allocations_dont_overlap)
{
    char *a = my_malloc(100);
    char *b = my_malloc(100);
    cr_assert_not_null(a);
    cr_assert_not_null(b);
    cr_assert_neq(a, b);
    memset(a, 'A', 100);
    memset(b, 'B', 100);
    for (int i = 0; i < 100; i++)
    {
        cr_assert_eq(a[i], 'A');
        cr_assert_eq(b[i], 'B');
    }
    my_free(a);
    my_free(b);
}

Test(malloc, many_small_allocations)
{
    enum
    {
        N = 1000
    };
    int *ptrs[N];
    for (int i = 0; i < N; i++)
    {
        ptrs[i] = my_malloc(sizeof(int));
        cr_assert_not_null(ptrs[i]);
        *ptrs[i] = i;
    }
    for (int i = 0; i < N; i++)
        cr_assert_eq(*ptrs[i], i);
    for (int i = 0; i < N; i++)
        my_free(ptrs[i]);
}

Test(malloc, large_allocation_multiple_pages)
{
    size_t size = 5 * 1024 * 1024;
    char *p = my_malloc(size);
    cr_assert_not_null(p);
    p[0] = 1;
    p[size - 1] = 2;
    cr_assert_eq(p[0], 1);
    cr_assert_eq(p[size - 1], 2);
    my_free(p);
}

Test(malloc, huge_allocation_fails)
{
    void *p = my_malloc((size_t)-1);
    cr_assert_null(p);
}

Test(malloc, size_zero_does_not_crash)
{
    void *p = my_malloc(0);
    my_free(p);
}

/* ------------------------------------------------------------------ */
/* free                                                               */
/* ------------------------------------------------------------------ */
Test(free, null_pointer)
{
    my_free(NULL);
}

Test(free, memory_is_reusable)
{
    void *a = my_malloc(128);
    cr_assert_not_null(a);
    my_free(a);
    void *b = my_malloc(128);
    cr_assert_not_null(b);
    cr_assert_eq(a, b, "bloc libere non reutilise immediatement");
    my_free(b);
}

Test(free, coalescing_adjacent_blocks)
{
    void *a = my_malloc(100);
    void *b = my_malloc(100);
    void *c = my_malloc(100);
    cr_assert_not_null(a);
    cr_assert_not_null(b);
    cr_assert_not_null(c);
    my_free(a);
    my_free(b);
    /* a et b fusionnes : on doit pouvoir allouer ~200 octets au meme endroit */
    void *d = my_malloc(200);
    cr_assert_not_null(d);
    cr_assert_eq(d, a, "fusion de blocs non effectuee");
    my_free(d);
    my_free(c);
}

Test(free, double_alloc_free_cycles)
{
    for (int i = 0; i < 10000; i++)
    {
        void *p = my_malloc(64);
        cr_assert_not_null(p);
        my_free(p);
    }
}

/* ------------------------------------------------------------------ */
/* calloc                                                             */
/* ------------------------------------------------------------------ */
Test(calloc, zero_initialized)
{
    unsigned char *p = my_calloc(100, 10);
    cr_assert_not_null(p);
    for (size_t i = 0; i < 1000; i++)
        cr_assert_eq(p[i], 0);
    my_free(p);
}

Test(calloc, zero_even_after_dirty_reuse)
{
    char *p = my_malloc(256);
    cr_assert_not_null(p);
    memset(p, 0xFF, 256);
    my_free(p);
    unsigned char *q = my_calloc(256, 1);
    cr_assert_not_null(q);
    for (int i = 0; i < 256; i++)
        cr_assert_eq(q[i], 0, "octet %d non nul apres calloc", i);
    my_free(q);
}

Test(calloc, overflow_returns_null)
{
    void *p = my_calloc((size_t)-1, 2);
    cr_assert_null(p);
}

Test(calloc, overflow_large_values)
{
    void *p = my_calloc(SIZE_MAX / 2 + 1, 2);
    cr_assert_null(p);
}

/* ------------------------------------------------------------------ */
/* realloc                                                            */
/* ------------------------------------------------------------------ */
Test(realloc, null_ptr_acts_as_malloc)
{
    void *p = my_realloc(NULL, 64);
    cr_assert_not_null(p);
    memset(p, 1, 64);
    my_free(p);
}

Test(realloc, size_zero_acts_as_free)
{
    void *p = my_malloc(64);
    cr_assert_not_null(p);
    void *q = my_realloc(p, 0);
    cr_assert_null(q);
}

Test(realloc, grow_preserves_data)
{
    char *p = my_malloc(50);
    cr_assert_not_null(p);
    for (int i = 0; i < 50; i++)
        p[i] = (char)i;
    char *q = my_realloc(p, 5000);
    cr_assert_not_null(q);
    for (int i = 0; i < 50; i++)
        cr_assert_eq(q[i], (char)i, "donnee perdue a l'index %d", i);
    my_free(q);
}

Test(realloc, shrink_preserves_data)
{
    char *p = my_malloc(1000);
    cr_assert_not_null(p);
    for (int i = 0; i < 1000; i++)
        p[i] = (char)(i % 127);
    char *q = my_realloc(p, 100);
    cr_assert_not_null(q);
    for (int i = 0; i < 100; i++)
        cr_assert_eq(q[i], (char)(i % 127));
    my_free(q);
}

Test(realloc, same_size)
{
    char *p = my_malloc(100);
    cr_assert_not_null(p);
    memset(p, 'Z', 100);
    char *q = my_realloc(p, 100);
    cr_assert_not_null(q);
    for (int i = 0; i < 100; i++)
        cr_assert_eq(q[i], 'Z');
    my_free(q);
}

Test(realloc, repeated_growth)
{
    char *p = my_malloc(1);
    cr_assert_not_null(p);
    p[0] = 42;
    for (size_t s = 2; s <= 4096; s *= 2)
    {
        p = my_realloc(p, s);
        cr_assert_not_null(p);
        cr_assert_eq(p[0], 42);
    }
    my_free(p);
}
