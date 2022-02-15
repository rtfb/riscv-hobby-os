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

char* strncpy(char *dest, char const *src, unsigned int num) {
    char *orig_dest = dest;
    for (unsigned int i = 0; i < num - 1 && *src; i++) {
        *dest++ = *src++;
    }
    *dest = '\0';
    return orig_dest;
}
