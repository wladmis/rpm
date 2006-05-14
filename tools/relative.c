/*
  Copyright (C) 2001  Dmitry V. Levin <ldv@altlinux.org>

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
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/param.h>

static	void	result( const char *str ) __attribute__ ((noreturn));

static	const char*	normalize( char *str )
{
	char *p;
	size_t	len = strlen( str );

	for ( p = strstr( str, "//" ); p; p = strstr( str, "//" ) )
		memmove( p, p + 1, strlen( p ) );

	for ( p = strstr( str, "/./" ); p; p = strstr( str, "/./" ) )
		memmove( p, p + 2, strlen( p + 1 ) );

	if ( (len >= 2) && ('/' == str[len-2]) && ('.' == str[len-1]) )
		str[len-1] = '\0';

	return str;
}

static	void	result( const char *str )
{
	puts( str );
	exit( 0 );
}

static	void	strip_trailing( char *str, const char sym )
{
	char *p;
	for ( p = strrchr( str, sym ); p && (p >= str) && (sym == *p); --p )
		*p = '\0';
}

static	const char*	base_name( const char *name )
{
	const char*	p = strrchr( name, '/' );
	if ( p )
		return p + 1;
	else
		return name;
}

static	char* lookup_back( const char *str, const char sym, const char *pos )
{
	for ( ; pos >= str ; --pos )
		if ( sym == *pos )
			return (char *)pos;

	return 0;
}

extern const char *__progname;

int	main( int ac, char *av[] )
{
	if ( ac < 3 )
	{
		fprintf( stderr, "Usage: %s <what> <to>\n", __progname );
		return 1;
	}
	else
	{
		const char	*orig_what = normalize( av[1] );
		normalize( av[2] );

		{
			unsigned	reslen;
			char	*what_p, *to_p;

			char	what[ 1 + strlen( av[1] ) ],
					to[ 1 + strlen( av[2] ) ];

			memcpy( what, av[1], sizeof(what) );
			memcpy( to, av[2], sizeof(to) );

			if ( '/' != *what )
				result( what );

			if ( '/' != *to )
			{
				fputs( "relative: <to> must be absolute filename\n", stderr );
				return 1;
			}

			reslen = PATH_MAX + strlen( what ) + strlen( to );

			strip_trailing( what, '/' );
			strip_trailing( to, '/' );

			for ( what_p = what, to_p = to; *what_p && *to_p ; ++what_p, ++to_p )
				if ( *what_p != *to_p )
					break;

			if ( !*what_p && !*to_p )
				result( base_name( orig_what ) );
			else
			{
				char	res[ reslen ];
				memset( res, 0, sizeof(res) );

				if ( ('/' == *what_p) && !*to_p )
					result( orig_what + (++what_p - what) );

				if ( '/' != *to_p || *what_p )
				{
					what_p = lookup_back( what, '/', what_p );
					strcpy( res, ".." );
				}

				for ( ; *to_p; ++to_p )
				{
					if ( '/' == *to_p )
					{
						if ( *res )
							strcat( res, "/.." );
						else
							strcpy( res, ".." );
					}
				}

				strcat( res, orig_what + (what_p - what) );
				result( res );
			}
		}
	}
}
