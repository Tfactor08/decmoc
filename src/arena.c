#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define ARENA_BASE_POS (sizeof(Arena))
#define REGION_BASE_POS (sizeof(Region))
#define ARENA_ALIGN (sizeof(void *))

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define ALIGN_UP_POW2(n, p) (((size_t)(n) + ((size_t)(p) - 1)) & (~((size_t)(p) - 1)))

#define PUSH_STRING(arena, len) (char *)arena_push((arena), sizeof(char) * len, 0)
#define PUSH_STRUCT(arena, T) (T *)arena_push((arena), sizeof(T), 1)

typedef struct Region Region;

struct Region {
    Region *prev;
    size_t capacity;
    size_t pos;
};

typedef struct {
    Region *current;
} Arena;

static Region *region_create(size_t capacity)
{
    Region *region = malloc(sizeof(Region) + capacity);
    region->capacity = capacity;
    region->pos = REGION_BASE_POS;
    region->prev = NULL;
    return region;
}

void arena_create(Arena *arena, size_t capacity)
{
    arena->current = region_create(capacity);
}

void *arena_push(Arena *arena, size_t size, bool zero)
{
    Region *region = arena->current;
    size_t pos_aligned = ALIGN_UP_POW2(region->pos, ARENA_ALIGN);
    size_t new_pos = pos_aligned + size;
    if (new_pos > region->capacity) {
        size_t new_cap = region->capacity * 2;
        if (new_cap < size) new_cap = size;
        Region *new_region = region_create(new_cap);
        new_region->prev = region;
        arena->current = new_region;
        region = new_region;
    }
    region->pos = new_pos;
    uint8_t *out = (uint8_t *)region + pos_aligned;
    if (zero) {
        memset(out, 0, size);
    }
    return out;
}

#if 0
// TODO: reimplement with regions
void arena_pop(Arena *arena, size_t size)
{
    size = MIN(size, arena->pos - ARENA_BASE_POS);
    arena->pos -= size;
}

// TODO: reimplement with regions
void arena_pop_to(Arena *arena, size_t pos)
{
    size_t size = pos < arena->pos ? arena->pos - pos : 0;
    arena_pop(arena, size);
}

// TODO: reimplement with regions
void arena_clear(Arena *arena)
{
    arena_pop_to(arena, ARENA_BASE_POS);
}
#endif

void arena_destroy(Arena *arena)
{
    Region *region = arena->current;
    while (region) {
        Region *prev = region->prev;
        free(region);
        region = prev;
    }
    arena->current = NULL;
}

#if 0
int main()
{
    typedef struct {
        size_t a;
        size_t b;
        size_t c;
    } Foo;

    Arena arena;

    arena_create(&arena, 10);

    Foo *foo1 = PUSH_STRUCT(&arena, Foo);
    Foo *foo2 = PUSH_STRUCT(&arena, Foo);

    foo1->a = 42;
    foo1->b = 42;
    foo1->c = 80085;

    printf("%zu\n", foo2->a);
    printf("%zu\n", foo2->b);
    printf("%zu\n", foo2->c);

    arena_destroy(&arena);
}
#endif
