#include "system.h"

#ifdef linux
#include <linux/limits.h>
#endif

int rpm_close_all (void)
{
	int fd, max;

	max = sysconf (_SC_OPEN_MAX);
	if (max <= 0)
		return -1;

#ifdef linux
	if (max < NR_OPEN)
		max = NR_OPEN;
#endif

	for (fd = STDERR_FILENO + 1; fd < max; ++fd)
	{
		if (close (fd) && errno != EBADF)
			/*
			 * Possible errors are:
			 * EINTR: the close() call was interrupted by a signal;
			 * EIO: an I/O error occurred.
			*/
			return -1;
	}

	return 0;
}
