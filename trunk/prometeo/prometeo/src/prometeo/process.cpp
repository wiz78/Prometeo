/***************************************************************************
                                 process.cpp
                             -------------------
	revision             : $Id: process.cpp,v 1.4 2002-11-01 22:23:52 tellini Exp $
    copyright            : (C) 2002 by Simone Tellini
    email                : tellini@users.sourceforge.net

	description          : this class offers an IPC interface to a spwaned
                           processes.
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

#include <sys/socket.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>

#include "process.h"
#include "processgroup.h"
#include "unixsocket.h"

// how much space alloc for each new chunk of request
// answers
#define CHUNK_SIZE	1024

static void AsyncCallback( Socket *sock, Prom_SC_Reason reason, int data, void *userdata );

//---------------------------------------------------------------------------
static Process *Proc;
//---------------------------------------------------------------------------
static void SigHup( int signum )
{
	Proc->ReloadCfg();
		
	signal( SIGHUP, SigHup );
}
//---------------------------------------------------------------------------
Process::Process()
{
	Group    = NULL;
	Socket   = NULL;
	Callback = NULL;
}
//---------------------------------------------------------------------------
Process::~Process()
{
	Terminate();

	delete Socket;
}
//---------------------------------------------------------------------------
bool Process::Spawn( char *ident )
{
	bool	ok = false;
	int		fd[2];

	if( socketpair( AF_UNIX, SOCK_STREAM, 0, fd ) >= 0 ) {

		ChildPid = App->Fork( ident );
		ok  	 = ChildPid >= 0;

		if( ok ) {

			if( ChildPid == 0 ) {
				// child

				close( fd[0] );

				ChildPid = getpid();

				App->Log->Log( LOG_INFO,
							   "%s: new process spawned (pid: %d)",
							   ident, ChildPid );

				Socket = new UnixSocket( fd[1] );
				Proc   = this;

				signal( SIGHUP, SigHup );
				
				WaitRequest();

				delete Socket;

				exit( 0 );

			} else {
				// parent

				close( fd[1] );

				Socket = new UnixSocket( fd[0] );

				Socket->SetAsyncCallback( AsyncCallback, this );
				Flags.Set( PROM_PROCF_RUNNING );
			}

		} else {

			close( fd[0] );
			close( fd[1] );
		}
	}

	return( ok );
}
//---------------------------------------------------------------------------
void Process::Terminate( void )
{
	// this method is called in the parent process context
	if( IsRunning() ) {
		int status;

		DBG( App->Log->Log( LOG_INFO, "Process::Terminate() - sending SIGTERM to %d", ChildPid ));

		if( kill( ChildPid, SIGTERM ) == 0 )
			waitpid( ChildPid, &status, 0 );

		Flags.Clear( PROM_PROCF_RUNNING );
	}
}
//---------------------------------------------------------------------------
void Process::SocketEvent( Prom_SC_Reason reason, int data )
{
	// this method is called in the parent process context
	bool done = false;

	switch( reason ) {

		case PROM_SOCK_READ:
			if( data )
				done = ReadAnswer( data );
			else {
				// the child must have crashed...
				App->Log->Log( LOG_ERR, "process: lost connection to child" );
				Flags.Clear( PROM_PROCF_RUNNING );
				done = true;
			}
			break;

		case PROM_SOCK_ERROR:
			App->Log->Log( LOG_ERR, "process: error on IPC socket - %d: %s", data, strerror( data ));

			if( Callback )
				( *Callback )( NULL, data, CallbackData );
			break;
	}

	if( done ) {

		time( &LastReqTime );

		DataLen = 0;
		Data.Clear();
		Flags.Clear( PROM_PROCF_BUSY );

		if( Group )
			Group->RunQueue();
	}
}
static void AsyncCallback( Socket *sock, Prom_SC_Reason reason, int data, void *userdata )
{
	((Process *)userdata )->SocketEvent( reason, data );
}
//---------------------------------------------------------------------------
bool Process::ReadAnswer( int len )
{
	bool	done;

	DataLen += len;
	done     = IsAnswerComplete();

	if( !done ) {
		unsigned int size = Data.GetSize(), left = size - DataLen;

		if( left <= 0 ) {
			Data.Resize( size + CHUNK_SIZE );
			left = CHUNK_SIZE;
		}

		Socket->AsyncRecv( Data.GetData() + DataLen, left );

	} else if( Callback )
		( *Callback )( Data.GetData() + sizeof( int ),
					   DataLen - sizeof( int ),
					   CallbackData );

	return( done );
}
//---------------------------------------------------------------------------
bool Process::IsAnswerComplete( void )
{
	// this method is called in the parent process context
	unsigned int *size = (unsigned int *)Data.GetData();

	return(( DataLen >= sizeof( *size )) &&
	       ( *size == DataLen - sizeof( *size )));
}
//---------------------------------------------------------------------------
void Process::SendRequest( Buffer *req, Prom_IPC_Callback callback, void *userdata )
{
	// this method is called in the parent process context
	unsigned int len;

	Flags.Set( PROM_PROCF_BUSY );

	Callback     = callback;
	CallbackData = userdata;
	len          = req->GetSize();

	Socket->Send( &len, sizeof( len ));
	Socket->Send( req->GetData(), len );

	delete req;

	Data.Resize( CHUNK_SIZE );
	
	DataLen = 0;

	Socket->AsyncRecv( Data.GetData(), CHUNK_SIZE );
}
//---------------------------------------------------------------------------
void Process::WaitRequest( void )
{
	// this method is called in the child process context
	fd_set	fds;
	int		FD = Socket->GetFD();
	bool	ok = true;

	FD_ZERO( &fds );
	FD_SET( FD, &fds );

	Prom_set_ps_display( "waiting for requests" );

	// loop until we recv some requests or we get killed
	while( ok && ( select( FD + 1, &fds, NULL, NULL, NULL ) > 0 )) {
		unsigned int	len = 0;

		// read the length of the request
		if( Socket->Recv( &len, sizeof( len )) == sizeof( len )) {
			char 	*ptr;

			Data.Resize( len );

			ptr = Data.GetData();

			// read the request
			do {
				int ret;

				ret = Socket->Recv( ptr, len );

				if( ret <= 0 )
					ok = false;

				else {
					len -= ret;
					ptr += ret;
				}

			} while( ok && ( len > 0 ));

			if( ok )
				Dispatch();

			Data.Clear();
		}

		FD_ZERO( &fds );
		FD_SET( FD, &fds );

		Prom_set_ps_display( "waiting for requests" );
	}
}
//---------------------------------------------------------------------------
void Process::UseDispatcher( IODispatcher *io )
{
	if( Socket )
		Socket->UseDispatcher( io );
}
//---------------------------------------------------------------------------
