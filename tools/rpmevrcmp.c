/*
  Copyright (C) 2002-2004  Dmitry V. Levin <ldv@altlinux.org>

  rpm EVRs comparator.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <rpmlib.h>

int
main (int ac, const char *av[])
{
	const char *e1 = 0, *v1 = 0, *r1 = 0;
	const char *e2 = 0, *v2 = 0, *r2 = 0;
	char *arg1, *arg2;

	if (ac != 3)
	{
		fprintf (stderr,
			 "%s - compare EVRs.\n"
			 "Usage: %s <EVR1> <EVR2>\n"
			 "\nReport bugs to http://bugs.altlinux.ru/\n\n",
			 program_invocation_short_name,
			 program_invocation_short_name);
		return EXIT_FAILURE;
	}

	arg1 = strdup (av[1]);
	parseEVR (arg1, &e1, &v1, &r1);

	arg2 = strdup (av[2]);
	parseEVR (arg2, &e2, &v2, &r2);

	printf ("%d\n", rpmEVRcmp (e1, v1, r1, 0, e2, v2, r2, 0));

	free (arg2);
	free (arg1);

	return EXIT_SUCCESS;
}
