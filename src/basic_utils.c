#ifndef BASIC_UTILS
#define BASIC_UTILS

#include <stdlib.h>
#include <string.h>
#include <math.h>

static void itoa(int n, char *s, size_t len)
{
    snprintf(s, len, "%d", n);
}

static size_t int_len(int n)
{
    return log10(abs(n)) + 1;
}

#endif // BASIC_UTILS
