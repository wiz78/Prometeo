/***************************************************************************
                                   url.h
                             -------------------
    revision             : $Id: url.h,v 1.2 2002-11-14 18:14:01 tellini Exp $
    copyright            : (C) 2002 by Simone Tellini
    email                : tellini@users.sourceforge.net

    description          : URL parser
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef URL_H
#define URL_H

#include <string>

class URL
{
public:
				URL();
				URL( const char *string );

	const char	*GetScheme( void )   { return( Scheme.c_str() ); }
	const char	*GetHost( void )     { return( Host.c_str() ); }
	int			GetPort( void )      { return( Port ); }
	const char	*GetHostPort( void );
	const char	*GetRest( void )     { return( Rest.c_str() ); }
	const char	*GetUser( void )     { return( User.c_str() ); }
	const char	*GetPassword( void ) { return( Password.c_str() ); }

	void		SetHost( const char *host )     { Host = host; }
	void		SetPort( int port )             { Port = port; }
	void		SetScheme( const char *scheme ) { Scheme = scheme; }

	void		Decode( const char *string );
	const char	*Encode( void );

	void		Clear( void );

private:
	string		Scheme;
	string		Host;
	string		Rest;
	string		User;
	string		Password;
	string		HostPort;
	string		EncodedURL;
	int         Port;
};

#endif
