#include <stdio.h>

extern int returner();
extern void asmprinter();
extern int fib(int);

void main() {
	printf("hello, world\n");
	printf("returner returned %d\n", returner());
	asmprinter();

    for (int i = 1; i < 15; i++) {
        printf("fib(%d) = %d\n", i, fib(i));
    }
}
