#include "string.h"

int strncmp(char const *a, char const *b, unsigned int num) {
    unsigned int i = 0;
    do {
        if (!*a && !*b) {
            return 0;
        }
        if (!*a) {
            return -1;
        }
        if (!*b) {
            return 1;
        }
        if (*a != *b) {
            break;
        }
        i++;
        a++;
        b++;
    } while(i < num);
    if (i == num) {
        return 0;
    }
    return *a - *b;
}

// has_prefix checks whether str has a given prefix. Returns 0 if at least one
// character of str is different than the corresponding character in prefix. If
// end_of_prefix is not null, it will be set to the index of the first
// character within str after the prefix.
int has_prefix(char const *str, char const *prefix, int *end_of_prefix) {
    int prefix_len = kstrlen(prefix);
    if (strncmp(str, prefix, prefix_len)) {
        return 0;
    }
    if (end_of_prefix != 0) {
        *end_of_prefix = prefix_len;
    }
    return 1;
}

char* strncpy(char *dest, char const *src, unsigned int num) {
    char *orig_dest = dest;
    for (unsigned int i = 0; i < num - 1 && *src; i++) {
        *dest++ = *src++;
    }
    *dest = '\0';
    return orig_dest;
}

int kstrlen(char const *s) {
    unsigned int i = 0;
    while (*s++) i++;
    return i;
}
