/***************************************************************************
                                procpool.cpp
                             -------------------
    revision             : $Id: procpool.cpp,v 1.1 2002-11-09 18:25:12 tellini Exp $
    copyright            : (C) 2002 by Simone Tellini
    email                : tellini@users.sourceforge.net

    description          : handles the pool of processes which serve the
                           clients
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
#include "registry.h"
#include "tcpsocket.h"
#include "procpool.h"
#include "client.h"

//---------------------------------------------------------------------------
SSLProcPool::SSLProcPool( string key, IODispatcher *io ) : ProcessGroup( 20, io )
{
	Key               = key;
	MinChildren       = 0;
	MaxChildren       = 20;
	ChildrenDecayTime = 60;

	Children.SetAllocBy( MaxChildren );

	Setup();
}
//---------------------------------------------------------------------------
void SSLProcPool::Setup( void )
{
	if( App->Cfg->OpenKey( Key.c_str(), false )) {

		MinChildren       = App->Cfg->GetInteger( "minchildren", MinChildren );
		MaxChildren       = App->Cfg->GetInteger( "maxchildren", MaxChildren );
		ChildrenDecayTime = App->Cfg->GetInteger( "childrenttl", ChildrenDecayTime );

		App->Cfg->CloseKey();
	}

	if( MinChildren > Children.Count() ) {

		while( MinChildren > Children.Count() ) {
			SSLClient	*proc = new SSLClient( Key );

			if( proc->Spawn( "SSL tunnel" ))
				AddChild( proc );
			else {
				App->Log->Log( LOG_ERR, "mod_ssl: couldn't spawn a new process" );
				delete proc;
			}
		}
	}
}
//---------------------------------------------------------------------------
void SSLProcPool::OnFork( void )
{
	// free some resources when someone else forks
	for( int i = 0; i < Children.Count(); i++ ) {
		SSLClient	*proc = (SSLClient *)Children[ i ];

		proc->OnFork();

		delete proc;
	}

	Children.Clear();
}
//---------------------------------------------------------------------------
void SSLProcPool::ServeClient( TcpSocket *sock )
{
	SSLClient	*proc = (SSLClient *)FindIdleProcess();
	bool	forked = false;

	if( !proc && ( Children.Count() < MaxChildren )) {

		proc   = new SSLClient( Key );
		forked = proc->Spawn( "SSL tunnel" );

		if( forked )
			AddChild( proc );
		else {
			delete proc;
			proc = NULL;
		}
	}

	if( proc )
		proc->Serve( sock, forked );
	else
		delete sock;
}
//---------------------------------------------------------------------------
bool SSLProcPool::AreIdle( void )
{
	bool	idle = true;

	for( int i = 0; idle && ( i < Children.Count() ); i++ ) {
		SSLClient	*proc = (SSLClient *)Children[ i ];

		if( proc->IsBusy() )
			idle = false;
	}

	return( idle );
}
//---------------------------------------------------------------------------
