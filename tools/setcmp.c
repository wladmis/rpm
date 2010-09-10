#include <stdio.h>
#include <assert.h>
#include "set.h"

int main(int argc, const char **argv)
{
    assert(argc == 3);
    int cmp = rpmsetcmp(argv[1], argv[2]);
    switch (cmp) {
    case 1:
    case 0:
    case -1:
    case -2:
	printf("%d\n", cmp);
	return 0;
    case -3:
	fprintf(stderr, "%s: set1 error\n", __FILE__);
	break;
    case -4:
	fprintf(stderr, "%s: set2 error\n", __FILE__);
	break;
    default:
	fprintf(stderr, "%s: unknown error\n", __FILE__);
	break;
    }
    return 1;
}
