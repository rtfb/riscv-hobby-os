#ifndef _USTR_H_
#define _USTR_H_

#include "userland.h"

int _userland ustrncmp(char const *a, char const *b, unsigned int num);
int _userland ustrlen(char const *s);
int _userland uatoi(char const *s, int *err);

#endif // ifndef _USTR_H_
