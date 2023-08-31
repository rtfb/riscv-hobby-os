#include "ustr.h"

int _userland ustrncmp(char const *a, char const *b, unsigned int num) {
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
    return *a - *b;
}

int _userland ustrlen(char const *s) {
    unsigned int i = 0;
    while (*s++) i++;
    return i;
}

int _userland uatoi(char const *s, int *err) {
    int multiplier = 1;
    if (s[0] == '-') {
        multiplier = -1;
        s++;
    }
    int n_digits = 0;
    while (*s != 0) {
        if (*s < '0' || *s > '9') {
            *err = 1;
            return 0;
        }
        s++;
        n_digits++;
    }
    s--;
    int number = 0;
    int mult_10 = 1;
    while (n_digits != 0) {
        int digit = *s - '0';
        number += digit * mult_10;
        mult_10 *= 10;
        s--;
        n_digits--;
    }
    return number * multiplier;
}
