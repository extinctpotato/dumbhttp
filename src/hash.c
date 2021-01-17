#include <stdlib.h>
#include <stdio.h>
#include "misc.h"

int main(int argc, char **argv) {
	printf("%lu\n", hash(argv[1]));
}
