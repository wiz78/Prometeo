/***************************************************************************
                                hostmapper.h
                             -------------------
    revision             : $Id: hostmapper.h,v 1.1 2002-11-18 17:46:41 tellini Exp $
    copyright            : (C) 2002 by Simone Tellini
    email                : tellini@users.sourceforge.net

    description          : map hostnames to a host/port pair
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef HOSTMAPPER_H
#define HOSTMAPPER_H

using namespace std;

#include <string>

#include "strhash.h"

class HostMapper
{
public:
				HostMapper( const string key );
				~HostMapper();

	void		ReloadCfg( void );

	const char	*Map( const char *host, short *port );

private:
	StrHash		HostMap;
	string		Key;

	void		Clear( void );
};

class Host
{
public:
				Host( const char *host, short port )
				{
					Hostname = host;
					Port     = port;
				}

	string&		GetHost( void ) { return( Hostname ); }
	short		GetPort( void ) const { return( Port ); }

private:
	string		Hostname;
	short		Port;
};

#endif
