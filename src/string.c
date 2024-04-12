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

// itoa() converts a given integer num to its string representation. If
// successful, returns the length of the string written to buf (including the
// terminating zero). Otherwise, returns a negative required size.
//
// Only supports unsigned integers for now.
int itoa(char *buf, int size, uint32_t num) {
    uint32_t orig_num = num;
    int need_size = 0;
    do {
        need_size++;
        num /= 10;
    } while (num > 0);
    need_size++; // for terminating zero
    if (need_size > size) {
        return -need_size;
    }
    int i = need_size - 1;
    buf[i] = 0;
    do {
        i--;
        buf[i] = (orig_num % 10) + '0';
        orig_num /= 10;
    } while (orig_num > 0);
    return need_size;
}
