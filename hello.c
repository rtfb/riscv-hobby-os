#include <stdio.h>

extern int returner();
extern void asmprinter();

void main() {
	printf("hello, world\n");
	printf("returner returned %d\n", returner());
	asmprinter();
}
