/***************************************************************************
                               prometeoctl.cpp
                             -------------------
	revision             : $Id: prometeoctl.cpp,v 1.2 2002-10-13 15:40:12 tellini Exp $
    copyright            : (C) 2002 by Simone Tellini
    email                : tellini@users.sourceforge.net

	description          : utility to administer prometeo
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "api.h"

#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <pwd.h>
#include <iostream>
#include <fstream>

#include "unixsocket.h"

using namespace std;

//--------------------------------------------------------------------------
int main( int argc, char *argv[] )
{
	UnixSocket			sock;
	static const char	name[] = PROM_CTRL_SOCKNAME".ctl";

	if( sock.Bind( name )) {

		chmod( name, S_IRWXU | S_IRWXG | S_IRWXO );

		if( sock.Connect( PROM_CTRL_SOCKNAME )) {
			char	buf[ 512 ];
			int		len;

			// send the command line to prometeo
			for( int i = 1; i < argc; i++ ) {

				if( i > 1 )
					sock.Send( " ", 1 );

				sock.Send( argv[ i ], strlen( argv[ i ] ));
			}

			sock.Send( "", 1 );

			// print the response
			while(( len = sock.Recv( buf, sizeof( buf ))) > 0 )
				cout.write( buf, len );

		} else
			perror( "can't connect to prometeo" );

		unlink( name );

	} else
		perror( "can't bind the socket" );

	return( 0 );
}
//--------------------------------------------------------------------------
