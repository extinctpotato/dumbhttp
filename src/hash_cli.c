#include <stdlib.h>
#include <stdio.h>
#include "hash.h"

int main(int argc, char **argv) {
	printf("%lu\n", hash(argv[1]));
}
