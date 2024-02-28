#ifndef _STRING_H_
#define _STRING_H_

int strncmp(char const *a, char const *b, unsigned int num);
int has_prefix(char const *str, char const *prefix, int *end_of_prefix);
char* strncpy(char *dest, char const *src, unsigned int num);
int kstrlen(char const *s);

#endif // ifndef _STRING_H_
