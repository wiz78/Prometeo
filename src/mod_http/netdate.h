/***************************************************************************
                                  netdate.h
                             -------------------
    revision             : $Id: netdate.h,v 1.1.1.1 2002-10-10 09:59:51 tellini Exp $
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

#ifndef NETDATE_H
#define NETDATE_H

#include <time.h>

class NetDate
{
public:
				NetDate() {}
				NetDate( time_t date ) { *this = date; }

	NetDate&	operator =( const NetDate& date );
	NetDate&	operator =( const char *str );
	NetDate&	operator =( time_t date );

	bool		operator >( const NetDate& rhs );
	bool		operator >=( const NetDate& rhs );
	bool		operator <( const NetDate& rhs );
	bool		operator <=( const NetDate& rhs );

	NetDate&	operator +=( int secs );

	time_t		ToEpoch( void ) const { return( Date ); }

	char		*ToStr( void );

	void		Set( void ); // set to current time

private:
	time_t		Date;
	char		Buffer[ 30 ];

	bool		ParseRFC( char *buffer, struct tm *dt );
	bool		ParseAsctime( char *buffer, struct tm *dt );
	bool		ParseLame( char *buffer, struct tm *dt );
};

#endif
