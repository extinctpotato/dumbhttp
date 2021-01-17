#include <stdlib.h>
#include <stdio.h>

unsigned long hash(char *str) {
	unsigned long hash = 5381;
	int c;

	while ((c = *str++)) {
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
	}

	return hash;
}

void vprint(char* str) {
	char *p = str;

	while(*p) {
		switch(*p) {
			case '\r': printf("\\r");break;
			case '\n': printf("\\n\n");break;
			default: putchar(*p);break;
		}
		p++;
	}
}
