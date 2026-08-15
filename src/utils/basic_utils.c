#ifndef BASIC_UTILS
#define BASIC_UTILS

#include <stdlib.h>
#include <string.h>
#include <math.h>

#define ARRAY_LEN(arr) (sizeof((arr)) / sizeof(*(arr)))

#define MALLOC_CHECK(ptr)                                               \
    do {                                                                \
        if (!ptr) {                                                     \
            fprintf(stderr, "ERROR: malloc failed at %s:%d\n",          \
                    __FILE__, __LINE__);                                \
            exit(EXIT_FAILURE);                                         \
        }                                                               \
    } while (0)

static void itoa(int n, char *s, size_t len)
{
    snprintf(s, len, "%d", n);
}

static size_t int_len(int n)
{
    return log10(abs(n)) + 1;
}

#endif // BASIC_UTILS
