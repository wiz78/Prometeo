/***************************************************************************
                                 netdate.cpp
                             -------------------
    revision             : $Id: netdate.cpp,v 1.1.1.1 2002-10-10 09:59:51 tellini Exp $
    copyright            : (C) 2002 by Simone Tellini
    email                : tellini@users.sourceforge.net

    description          : RFC-1123, RFC-850, asctime date format parser
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "netdate.h"

static const char *Days[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };

static const char *Months[] = { "Jan", "Feb", "Mar",
								"Apr", "May", "Jun",
								"Jul", "Aug", "Sep",
								"Oct", "Nov", "Dec" };

//---------------------------------------------------------------------------
NetDate& NetDate::operator =( const NetDate& date )
{
	Date = date.Date;

	return( *this );
}
//---------------------------------------------------------------------------
NetDate& NetDate::operator =( const char *str )
{
	if( str ) {
		char		buffer[ 60 ];
		struct tm	dt;
		bool		ok = false;

		strncpy( buffer, str, sizeof( buffer ));
		buffer[ sizeof( buffer ) - 1 ] = '\0';

		Set();

		memset( &dt, 0, sizeof( dt ));

		if( strchr( buffer, ',' ))		// RFC format?
			ok = ParseRFC( buffer, &dt );

		else if( strchr( buffer, ':' ))	// asctime format?
			ok = ParseAsctime( buffer, &dt );

		else							// lame format?
			ok = ParseLame( buffer, &dt );

		if( ok ) {

			tzset();

			dt.tm_sec -= timezone; // convert to local time
			Date       = mktime( &dt );
		}
	}

	return( *this );
}
//---------------------------------------------------------------------------
NetDate& NetDate::operator =( time_t date )
{
	Date = date;

	return( *this );
}
//---------------------------------------------------------------------------
bool NetDate::operator >( const NetDate& rhs )
{
	return( Date > rhs.Date );
}
//---------------------------------------------------------------------------
bool NetDate::operator >=( const NetDate& rhs )
{
	return( Date >= rhs.Date );
}
//---------------------------------------------------------------------------
bool NetDate::operator <( const NetDate& rhs )
{
	return( Date < rhs.Date );
}
//---------------------------------------------------------------------------
bool NetDate::operator <=( const NetDate& rhs )
{
	return( Date <= rhs.Date );
}
//---------------------------------------------------------------------------
NetDate& NetDate::operator +=( int secs )
{
	Date += secs;

	return( *this );
}
//---------------------------------------------------------------------------
bool NetDate::ParseRFC( char *buffer, struct tm *dt )
{
	int		i;
	char	*ptr;
	bool	ok = false;

	if( ptr = strtok( buffer, ", " )) {

		for( i = 0; i < 7; i++ )
			if( !strncasecmp( ptr, Days[ i ], 3 )) {
				dt->tm_wday = i;
				break;
			}

		if( ptr = strtok( NULL, " -" )) {

			dt->tm_mday = atoi( ptr );

			if( ptr = strtok( NULL, " -" )) {

				for( i = 0; i < 12; i++ )
					if( !strcasecmp( ptr, Months[ i ] )) {
						dt->tm_mon = i;
						break;
					}

				if( ptr = strtok( NULL, " " )) {

					dt->tm_year = atoi( ptr );

					if( dt->tm_year > 1900 )
						dt->tm_year -= 1900;

					if( ptr = strtok( NULL, ":" )) {

						dt->tm_hour = atoi( ptr );

						if( ptr = strtok( NULL, ":" )) {

							dt->tm_min = atoi( ptr );

							if( ptr = strtok( NULL, " " )) {
								dt->tm_sec = atoi( ptr );
								ok = true;
							}
						}
					}
				}
			}
		}
	}

	return( ok );
}
//---------------------------------------------------------------------------
bool NetDate::ParseAsctime( char *buffer, struct tm *dt )
{
	int		i;
	char	*ptr;
	bool	ok = false;

	if( ptr = strtok( buffer, " " )) {

		for( i = 0; i < 7; i++ )
			if( !strcasecmp( ptr, Days[ i ] )) {
				dt->tm_wday = i;
				break;
			}

		if( ptr = strtok( NULL, " " )) {

			for( i = 0; i < 12; i++ )
				if( !strcasecmp( ptr, Months[ i ] )) {
					dt->tm_mon = i;
					break;
				}

			if( ptr = strtok( NULL, " " )) {

				dt->tm_mday = atoi( ptr );

				if( ptr = strtok( NULL, ":" )) {

					dt->tm_hour = atoi( ptr );

					if( ptr = strtok( NULL, ":" )) {

						dt->tm_min = atoi( ptr );

						if( ptr = strtok( NULL, " " )) {

							dt->tm_sec = atoi( ptr );

							if( ptr = strtok( NULL, " " )) {
								dt->tm_year = atoi( ptr ) - 1900;
								ok = true;
							}
						}
					}
				}
			}
		}
	}

	return( ok );
}
//---------------------------------------------------------------------------
bool NetDate::ParseLame( char *buffer, struct tm *dt )
{
	bool	ok = false;
	int		day = atoi( buffer );

	if( day ) {
		char *ptr;

		dt->tm_mday = day;

		if( ptr = strtok( buffer, " " ))
			for( int i = 0; i < 12; i++ )
				if( !strncasecmp( ptr, Months[ i ], 3 )) {
					dt->tm_mon = i;
					break;
				}

		if( ptr = strtok( NULL, " " ))
			dt->tm_year = atoi( ptr );

		if( dt->tm_year < 1900 )
			dt->tm_year += 1900;

		ok = true;
	}

	return( ok );
}
//---------------------------------------------------------------------------
char *NetDate::ToStr( void )
{
	struct tm *dt = gmtime( &Date );

	sprintf( Buffer, "%s, %02d %s %d %02d:%02d:%02d GMT",
			 Days[ dt->tm_wday ], dt->tm_mday, Months[ dt->tm_mon ],
			 dt->tm_year + 1900, dt->tm_hour, dt->tm_min, dt->tm_sec );

	return( Buffer );
}
//---------------------------------------------------------------------------
void NetDate::Set( void )
{
	time( &Date );
}
//---------------------------------------------------------------------------
