#if !defined H_CACHEHACK
# define H_CACHEHACK

# include <sys/types.h>
# include <sys/stat.h>
# include <unistd.h>
# include <rpm/rpmtypes.h>

# if defined __cplusplus
extern "C" {
# endif

int rpmaltheadercacheHACKGet(FD_t fd, const char * fn, const struct stat *st, Header * hdrp);

void rpmaltheadercacheHACKSet(FD_t fd, const char * fn, const struct stat *st, Header * hdrp);

# if defined __cplusplus
}
# endif

#endif
