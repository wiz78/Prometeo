/***************************************************************************
                               CmdFilter.cpp
                             -------------------
    revision             : $Id: cmdfilter.cpp,v 1.1 2004-04-24 13:51:48 tellini Exp $
    copyright            : (C) 2003-2004 by Simone Tellini
    email                : tellini@users.sourceforge.net

    description          : class to filter messages through external commands
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
#include "cmdfilter.h"

//---------------------------------------------------------------------------
namespace mod_pop3
{
//---------------------------------------------------------------------------
CmdFilter::CmdFilter( const string& cmd )
{
	Cmd = cmd;
}
//---------------------------------------------------------------------------
bool CmdFilter::Process( const string& msg )
{
	bool	ok = false;
	int		rfds[2];

	Clean();

	if( !pipe( rfds )) {
		int wfds[2];

		if( !pipe( wfds )) {

			switch( App->Fork( "mod_pop3 filter" )) {
			
				case -1:
					break;

				case 0:
					Child( wfds, rfds );
					break;

				default:
					ok = Parent( msg, wfds, rfds );
					break;
			}
		}
	} 

	if( !ok )
		App->Log->Log( LOG_ERR, 
					   "mod_pop3: cannot filter through %s (%d: %s)", 
					   Cmd.c_str(), errno, strerror( errno ));

	return( ok );
}
//---------------------------------------------------------------------------
bool CmdFilter::Parent( const string &msg, int wfds[2], int rfds[2] )
{
	bool ret = false;

	close( wfds[0] );
	close( rfds[1] );

	if( write( wfds[1], msg.c_str(), msg.length() ) >= 0 ) {
		char	buf[ 1024 * 32 ];
		int		len;

		close( wfds[1] );
	
		while(( len = read( rfds[0], buf, sizeof( buf ) - 1 )) > 0 ) {

			buf[ len ] = '\0';

			FilteredMsg += buf;
		}

		ret = len >= 0;

		if( !ret )
			App->Log->Log( LOG_ERR, 
						   "mod_pop3: cannot read back the message from the filter (%d: %s)", 
						   errno, strerror( errno ));


	} else {

		close( wfds[1] );

		App->Log->Log( LOG_ERR, 
					   "mod_pop3: cannot pipe the message to the filter (%d: %s)", 
					   errno, strerror( errno ));
	}

	close( rfds[0] );

	return( ret );
}
//---------------------------------------------------------------------------
void CmdFilter::Child( int wfds[2], int rfds[2] )
{
	close( wfds[1] );
	close( rfds[0] );

	dup2( wfds[0], STDIN_FILENO );
	dup2( rfds[1], STDOUT_FILENO );

	execl( "/bin/sh", "sh", "-c", Cmd.c_str(), NULL );

	App->Log->Log( LOG_ERR, "mod_pop3: execl failed for %s (%d: %s)", Cmd.c_str(), errno, strerror( errno ));
	
	exit( 0 );
}
//---------------------------------------------------------------------------
}; // namespace
//---------------------------------------------------------------------------

