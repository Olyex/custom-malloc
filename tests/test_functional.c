#include <criterion/criterion.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

void *my_malloc(size_t size);
void my_free(void *ptr);
void *my_calloc(size_t nmemb, size_t size);
void *my_realloc(void *ptr, size_t size);

/* ------------------------------------------------------------------ */
/* Scenarios realistes                                                */
/* ------------------------------------------------------------------ */

/* Simule une chaine de caracteres dynamique (comme un vecteur de char) */
Test(functional, dynamic_string_builder)
{
    size_t cap = 8;
    size_t len = 0;
    char *s = my_malloc(cap);
    cr_assert_not_null(s);

    const char *word = "hello ";
    for (int i = 0; i < 500; i++)
    {
        size_t wl = strlen(word);
        if (len + wl + 1 > cap)
        {
            cap *= 2;
            s = my_realloc(s, cap);
            cr_assert_not_null(s);
        }
        memcpy(s + len, word, wl);
        len += wl;
    }
    s[len] = '\0';
    cr_assert_eq(len, 6 * 500);
    cr_assert_eq(strncmp(s, "hello hello ", 12), 0);
    my_free(s);
}

/* Liste chainee d'entiers allouee avec mon malloc */
struct node
{
    int value;
    struct node *next;
};

Test(functional, linked_list_of_nodes)
{
    struct node *head = NULL;
    for (int i = 0; i < 2000; i++)
    {
        struct node *n = my_malloc(sizeof(*n));
        cr_assert_not_null(n);
        n->value = i;
        n->next = head;
        head = n;
    }
    int expected = 1999;
    for (struct node *n = head; n; n = n->next)
        cr_assert_eq(n->value, expected--);
    while (head)
    {
        struct node *next = head->next;
        my_free(head);
        head = next;
    }
}

/* Matrice 2D allouee ligne par ligne */
Test(functional, matrix_2d)
{
    enum
    {
        ROWS = 100,
        COLS = 100
    };
    int **m = my_malloc(ROWS * sizeof(int *));
    cr_assert_not_null(m);
    for (int i = 0; i < ROWS; i++)
    {
        m[i] = my_calloc(COLS, sizeof(int));
        cr_assert_not_null(m[i]);
    }
    for (int i = 0; i < ROWS; i++)
        for (int j = 0; j < COLS; j++)
            m[i][j] = i * COLS + j;
    for (int i = 0; i < ROWS; i++)
        for (int j = 0; j < COLS; j++)
            cr_assert_eq(m[i][j], i * COLS + j);
    for (int i = 0; i < ROWS; i++)
        my_free(m[i]);
    my_free(m);
}

/* Alloue en ordre, libere dans le desordre, realloue : teste la fusion */
Test(functional, fragmentation_pattern)
{
    enum
    {
        N = 200
    };
    void *ptrs[N];
    for (int i = 0; i < N; i++)
    {
        ptrs[i] = my_malloc(64 + (size_t)(i % 7) * 16);
        cr_assert_not_null(ptrs[i]);
        memset(ptrs[i], i & 0xFF, 64);
    }
    /* libere les pairs puis les impairs */
    for (int i = 0; i < N; i += 2)
        my_free(ptrs[i]);
    for (int i = 1; i < N; i += 2)
    {
        unsigned char *c = ptrs[i];
        cr_assert_eq(c[0], i & 0xFF, "bloc %d corrompu", i);
        my_free(ptrs[i]);
    }
    /* apres tout libere, une grosse allocation doit passer */
    void *big = my_malloc(N * 64);
    cr_assert_not_null(big);
    my_free(big);
}

/* qsort sur un tableau alloue dynamiquement */
static int cmp_int(const void *a, const void *b)
{
    int x = *(const int *)a;
    int y = *(const int *)b;
    return (x > y) - (x < y);
}

Test(functional, qsort_on_heap_array)
{
    enum
    {
        N = 5000
    };
    int *arr = my_malloc(N * sizeof(int));
    cr_assert_not_null(arr);
    unsigned int seed = 42;
    for (int i = 0; i < N; i++)
    {
        seed = seed * 1664525u + 1013904223u;
        arr[i] = (int)(seed >> 8);
    }
    qsort(arr, N, sizeof(int), cmp_int);
    for (int i = 1; i < N; i++)
        cr_assert_leq(arr[i - 1], arr[i]);
    my_free(arr);
}

/* ------------------------------------------------------------------ */
/* Performance (smoke test)                                           */
/* ------------------------------------------------------------------ */
Test(performance, one_million_alloc_free, .timeout = 10)
{
    for (int i = 0; i < 1000000; i++)
    {
        void *p = my_malloc(32 + (size_t)(i % 100));
        cr_assert_not_null(p);
        my_free(p);
    }
}

Test(performance, many_live_allocations, .timeout = 10)
{
    enum
    {
        N = 50000
    };
    void **ptrs = my_malloc(N * sizeof(void *));
    cr_assert_not_null(ptrs);
    for (int i = 0; i < N; i++)
    {
        ptrs[i] = my_malloc(16);
        cr_assert_not_null(ptrs[i]);
    }
    for (int i = 0; i < N; i++)
        my_free(ptrs[i]);
    my_free(ptrs);
}
