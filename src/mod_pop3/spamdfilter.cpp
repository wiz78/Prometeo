/***************************************************************************
                               spamdfilter.cpp
                             -------------------
    revision             : $Id: spamdfilter.cpp,v 1.1 2004-04-24 13:51:49 tellini Exp $
    copyright            : (C) 2003-2004 by Simone Tellini
    email                : tellini@users.sourceforge.net

    description          : class to filter messages through SpamAssassin
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "main.h"
#include "tcpsocket.h"
#include "dnscache.h"

#include "spamdfilter.h"

#define TIMEOUT_SPAMD		60

//---------------------------------------------------------------------------
namespace mod_pop3
{
//---------------------------------------------------------------------------
SpamdFilter::SpamdFilter( const string host, int port )
{
	SpamdPort = port;
	SpamdHost = host;
}
//---------------------------------------------------------------------------
bool SpamdFilter::Process( const string& msg )
{
	TcpSocket  *sock = new TcpSocket();
	Prom_Addr	addr;
	bool		ok = false;

	Clean();

	if( sock->IsValid() && 
		App->DNS->Resolve( SpamdHost.c_str(), &addr ) &&
		sock->Connect( &addr, SpamdPort )) {
		bool loop = true;
		char buf[ 1024 * 16 ];

		sock->Printf( "PROCESS SPAMC/1.0\r\n" );
		sock->Send( msg.c_str(), msg.length() );

		sock->Shutdown( 1 ); // send EOF to spamd

		if(( sock->RecvLine( buf, sizeof( buf ), TIMEOUT_SPAMD ) > 0 ) &&
		   !strncmp( buf, "SPAMD/", 6 )) {

			while( sock->RecvLine( buf, sizeof( buf ), TIMEOUT_SPAMD ) > 0 ) {

				if( buf[0] == '.' )
					FilteredMsg += ".";

				FilteredMsg += buf;
				FilteredMsg += "\r\n";
			}

			ok = true;
		}
			
	} else
		App->Log->Log( LOG_ERR, 
					   "mod_pop3: cannot connect to spamd on %s:%d (%d: %s)", 
					   SpamdHost.c_str(), SpamdPort, errno, strerror( errno ));
	
	delete sock;

	return( ok );
}
//---------------------------------------------------------------------------
}; // namespace
//---------------------------------------------------------------------------

