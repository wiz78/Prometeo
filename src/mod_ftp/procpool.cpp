/***************************************************************************
                                procpool.cpp
                             -------------------
    revision             : $Id: procpool.cpp,v 1.3 2002-11-01 22:23:51 tellini Exp $
    copyright            : (C) 2002 by Simone Tellini
    email                : tellini@users.sourceforge.net

    description          : handles the pool of processes which serve the ftp
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
ProcPool::ProcPool( string key, IODispatcher *io ) : ProcessGroup( 20, io )
{
	Key               = key;
	MinChildren       = 0;
	MaxChildren       = 20;
	ChildrenDecayTime = 60;

	Children.SetAllocBy( MaxChildren );

	Setup();
}
//---------------------------------------------------------------------------
void ProcPool::Setup( void )
{
	if( App->Cfg->OpenKey( Key.c_str(), false )) {

		MinChildren       = App->Cfg->GetInteger( "minchildren", MinChildren );
		MaxChildren       = App->Cfg->GetInteger( "maxchildren", MaxChildren );
		ChildrenDecayTime = App->Cfg->GetInteger( "childrenttl", ChildrenDecayTime );

		App->Cfg->CloseKey();
	}

	if( MinChildren > Children.Count() ) {

		while( MinChildren > Children.Count() ) {
			Client	*proc = new Client( Key );

			if( proc->Spawn( "FTP proxy" ))
				AddChild( proc );
			else {
				App->Log->Log( LOG_ERR, "mod_ftp: couldn't spawn a new process" );
				delete proc;
			}
		}
	}
}
//---------------------------------------------------------------------------
void ProcPool::OnFork( void )
{
	// free some resources when someone else forks
	for( int i = 0; i < Children.Count(); i++ ) {
		Client	*proc = (Client *)Children[ i ];

		proc->OnFork();

		delete proc;
	}

	Children.Clear();
}
//---------------------------------------------------------------------------
void ProcPool::ServeClient( TcpSocket *sock )
{
	Client	*proc = (Client *)FindIdleProcess();
	bool	forked = false;

	if( !proc && ( Children.Count() < MaxChildren )) {

		proc   = new Client( Key );
		forked = proc->Spawn( "FTP proxy" );

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
		sock->Printf( "421 Too many clients.\r\n" );
}
//---------------------------------------------------------------------------
bool ProcPool::AreIdle( void )
{
	bool	idle = true;

	for( int i = 0; idle && ( i < Children.Count() ); i++ ) {
		Client	*proc = (Client *)Children[ i ];

		if( proc->IsBusy() )
			idle = false;
	}

	return( idle );
}
//---------------------------------------------------------------------------
