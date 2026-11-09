CC = gcc

CPPFLAGS = -D_DEFAULT_SOURCE
CFLAGS = -std=c99 -pedantic -Werror -Wall -Wextra -Wvla \
         -fvisibility=hidden -fPIC
LDFLAGS = -shared -Wl,--no-undefined

TARGET_LIB = libmalloc.so

SRC = src/malloc.c src/linked_list.c
OBJ = $(SRC:.c=.o)

TEST_SRC = tests/test_unit.c tests/test_functional.c
TEST_BIN = tests/testsuite
TEST_CFLAGS = -std=gnu99 -Wall -Wextra -g -O0 -I src \
              -Dmalloc=my_malloc -Dfree=my_free \
              -Dcalloc=my_calloc -Drealloc=my_realloc \
              -fvisibility=default

.PHONY: all library debug check clean

all: library

library: $(TARGET_LIB)

$(TARGET_LIB): $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^

debug: CFLAGS += -g3 -O0
debug: clean $(TARGET_LIB)

$(TEST_BIN): $(SRC) $(TEST_SRC)
	$(CC) $(CPPFLAGS) $(TEST_CFLAGS) $^ -o $@ -lcriterion

check-preload: library
	sh tests/run_preload.sh

check: $(TEST_BIN)
	./$(TEST_BIN) --verbose

clean:
	$(RM) $(OBJ) $(TARGET_LIB) $(TEST_BIN)
