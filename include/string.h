#ifndef _STRING_H_
#define _STRING_H_

#include "sys.h"

int strncmp(char const *a, char const *b, unsigned int num);
int has_prefix(char const *str, char const *prefix, int *end_of_prefix);
char* strncpy(char *dest, char const *src, unsigned int num);
int kstrlen(char const *s);
int itoa(char *buf, int size, uint32_t num);

#endif // ifndef _STRING_H_
