# malloc

A C reimplementation of the libc memory allocation functions — `malloc`, `free`, `calloc` and `realloc` — built as a shared library (`libmalloc.so`) that can be injected into any program with `LD_PRELOAD`.

Developed as part of the engineering curriculum at EPITA.

## Features

- **`malloc`**: *first-fit* allocation over a doubly linked list of blocks, splitting the chosen block when enough space remains.
- **`free`**: releases a block, coalesces adjacent free blocks, and returns the page to the OS (`munmap`) once it is entirely free again.
- **`calloc`**: zero-initialized allocation, with overflow detection on `nmemb * size`.
- **`realloc`**: resizes while preserving data (`realloc(NULL, n)` behaves like `malloc(n)`, `realloc(p, 0)` frees `p`).
- Memory requested directly from the kernel through `mmap`, in multiples of the page size (4096 bytes).
- Sizes aligned to 16 bytes.
- Only the four public symbols are exported (`-fvisibility=hidden`).

## How it works

Every allocated area is preceded by a metadata header:

```c
struct block
{
    size_t size;        // size of the data (excluding metadata)
    int is_free;        // 1 = free, 0 = in use
    struct block *next; // next block
    struct block *prev; // previous block (used for coalescing on free)
};
```

```
 mmap'd page (multiple of 4096 bytes)
┌────────┬──────────────┬────────┬──────────────┬────────┬─────────────────┐
│ header │     data     │ header │     data     │ header │   free space    │
│ in use │              │ in use │              │  free  │                 │
└────────┴──────────────┴────────┴──────────────┴────────┴─────────────────┘
     ▲ pointer returned by malloc = header + 1
```

1. `malloc` walks the list looking for the first free block that is large enough. If none is found, it maps a new page and appends it to the list.
2. If the chosen block is larger than needed, the remainder is split off into a new free block.
3. `free` marks the block as free, then merges it with its neighbours if they are free and contiguous in memory. If the resulting block spans a whole page, the page is returned to the OS.

## Project structure

```
.
├── Makefile
├── src
│   ├── malloc.c          # malloc / free / calloc / realloc
│   ├── linked_list.c     # page and block management
│   └── linked_list.h
└── tests
    ├── test_unit.c       # unit tests (Criterion)
    ├── test_functional.c # functional and performance tests (Criterion)
    └── run_preload.sh    # runs real programs with LD_PRELOAD
```

## Building

Requirements: `gcc`, `make`, and [Criterion](https://github.com/Snaipe/Criterion) for the tests.

```bash
make            # builds libmalloc.so
make debug      # debug build (-g3 -O0)
make clean
```

## Usage

Replace the allocator of any program:

```bash
LD_PRELOAD=./libmalloc.so ls -la
```

## Tests

```bash
make check          # unit and functional tests (Criterion)
make check-preload  # runs ls, find, git, sort, less… with libmalloc.so preloaded
```

The Criterion tests compile the allocator under the names `my_malloc`, `my_free`, etc., so it can be tested without replacing the test runner's own allocator.

## Limitations

- **Not thread-safe**: no lock protects the block list, so use in multi-threaded programs is not guaranteed to work.
- Linear *first-fit* search: allocation cost grows with the number of blocks.
- `realloc` always allocates a new block and copies the data, even when the current block could be grown in place.

## Author

Eliot Soumeillan
