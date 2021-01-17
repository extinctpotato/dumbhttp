#include <stdlib.h>
#include <stdio.h>
#include "misc.h"

unsigned long hash(char *str) {
	unsigned long hash = 5381;
	int c;

	while ((c = *str++)) {
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
	}

	return hash;
}

void vprintc(char* str, char* pre) {
	char *p = str;

	if (pre != NULL) printf(pre);
	while(*p) {
		switch(*p) {
			case '\r': printf("\\r");break;
			case '\n': printf("\\n\n");break;
			default: putchar(*p);break;
		}
		p++;
	}
	if (pre != NULL) printf(ANSI_CODE_RESET);
}
