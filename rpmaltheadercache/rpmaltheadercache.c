#include <rpm/rpmaltheadercache.h>

int rpmaltheadercacheHACKGet(FD_t fd, const char * fn, const struct stat *st, Header * hdrp)
{
    return 1;
}

void rpmaltheadercacheHACKSet(FD_t fd, const char * fn, const struct stat *st, Header * hdrp)
{
    return;
}
