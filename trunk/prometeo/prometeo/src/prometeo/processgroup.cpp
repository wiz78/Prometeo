/***************************************************************************
                              processgroup.cpp
                             -------------------
	revision             : $Id: processgroup.cpp,v 1.1 2002-10-10 10:22:59 tellini Exp $
    copyright            : (C) 2002 by Simone Tellini
    email                : tellini@users.sourceforge.net

	description          : this class offers an IPC interface to communicate
	                       with a group of sub-processes.

						   It accepts requests, finds a child to submit it
						   to and returns answers to callers when done.
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

#include "processgroup.h"
#include "process.h"

//---------------------------------------------------------------------------
ProcessGroup::ProcessGroup( int numchildren, IODispatcher *io )
{
	Children.SetAllocBy( numchildren );

	IO                = io;
	MinChildren       = 0xffffffff; // no limits
	ChildrenDecayTime = 30;
}
//---------------------------------------------------------------------------
ProcessGroup::~ProcessGroup()
{
	for( int i = 0; i < Children.Count(); i++ )
		delete (Process *)Children[ i ];

	while( Prom_IPCRequest *req = (Prom_IPCRequest *)Requests.RemTail() ) {
		delete req->Request;
		delete req;
	}
}
//---------------------------------------------------------------------------
void ProcessGroup::AddChild( Process *proc )
{
	proc->SetGroup( this );
	proc->UseDispatcher( IO );

	Children.Add( proc );
	RunQueue();
}
//---------------------------------------------------------------------------
void ProcessGroup::Flush( time_t now )
{
	bool	go = true;
		
	while(( MinChildren > Children.Count() ) && go ) {
		Process *proc = FindIdleProcess();

		if( proc && ( now - proc->GetLastReqTime() > ChildrenDecayTime )) {

			Children.Remove( proc );

			delete proc;
				
		} else
			go = false;
	}
}
//---------------------------------------------------------------------------
void ProcessGroup::SendRequest( Buffer *req, Prom_IPC_Callback callback, void *userdata )
{
	Process	*proc = FindIdleProcess();

	if( proc )
		proc->SendRequest( req, callback, userdata );
	else
		Enqueue( req, callback, userdata );
}
//---------------------------------------------------------------------------
Process *ProcessGroup::FindIdleProcess( void )
{
	Process	*proc = NULL;

	for( int i = 0; !proc && ( i < Children.Count() ); i++ ) {
		Process	*p = (Process *)Children[ i ];

		if( p->IsRunning() && p->IsIdle() )
			proc = p;
	}

	return( proc );
}
//---------------------------------------------------------------------------
void ProcessGroup::Enqueue( Buffer *req, Prom_IPC_Callback callback, void *userdata )
{
	Prom_IPCRequest *ipcreq = new Prom_IPCRequest();

	ipcreq->Request  = req;
	ipcreq->Callback = callback;
	ipcreq->UserData = userdata;

	Requests.AddTail( ipcreq );
}
//---------------------------------------------------------------------------
void ProcessGroup::RunQueue( void )
{
	Process *proc;

	while( !Requests.IsEmpty() && ( proc = FindIdleProcess() )) {
		Prom_IPCRequest *req = (Prom_IPCRequest *)Requests.RemHead();

		proc->SendRequest( req->Request, req->Callback, req->UserData );

		delete req;
	}
}
//---------------------------------------------------------------------------
