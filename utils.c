#include <stdio.h>
#include <string.h>

void reverse(char *s)
{
    char t;

    for (int i = 0, j = strlen(s)-1; i < j; i++, j--) {
        t = s[i], s[i] = s[j], s[j] = t;
    }
}

void itoa(int n, char *s)
{
    int sign;

    if ((sign = n) < 0)
        n = -n;
    int i = 0;
    do {
        s[i++] = n % 10 + '0';
    } while ((n /= 10) > 0);
    if (sign < 0)
        s[i++] = '-';
    s[i] = '\0';
    reverse(s);
}

size_t int_len(int n)
{
    int len = 1;
    do {
        len += 1;
    } while ((n /= 10) > 0);
    return len;
}

#if 0
int main(void)
{
    printf("%zu\n", int_len(0));

    return 0;
}
#endif
