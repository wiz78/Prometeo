/***************************************************************************
                                  process.h
                             -------------------
	revision             : $Id: process.h,v 1.2 2002-11-01 22:23:44 tellini Exp $
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

#ifndef PROM_PROCESS_H
#define PROM_PROCESS_H

#include "bitfield.h"
#include "buffer.h"
#include "processgroup.h"
#include "socket.h"

#define PROM_PROCF_BUSY		(1 << 0)
#define PROM_PROCF_RUNNING	(1 << 1)

class UnixSocket;
class IODispatcher;

class Process
{
public:
						Process();
						~Process();

	bool				Spawn( char *ident );
	void				Terminate( void );
	
	virtual void		OnFork( void ) {}
	virtual void		ReloadCfg( void ) {} 

	virtual void		SocketEvent( Prom_SC_Reason reason, int data );

	void				SendRequest( Buffer *req, Prom_IPC_Callback callback, void *userdata );

	bool				IsBusy( void ) const    { return( Flags.IsSet( PROM_PROCF_BUSY )); }
	bool				IsIdle( void ) const    { return( !IsBusy() ); }
	bool				IsRunning( void ) const { return( Flags.IsSet( PROM_PROCF_RUNNING )); }

	void				SetGroup( ProcessGroup *group ) { Group = group; }
	void				UseDispatcher( IODispatcher *io );

	pid_t				GetChildPid( void ) const { return( ChildPid ); }
	time_t				GetLastReqTime( void ) const { return( LastReqTime ); }
	
protected:
	pid_t				ChildPid;
	UnixSocket			*Socket;
	BitField			Flags;
	Buffer				Data;
	unsigned int		DataLen;
	Prom_IPC_Callback	Callback;
	void				*CallbackData;
	ProcessGroup		*Group;
	time_t				LastReqTime;

	// default protocol: the first 4 bytes of the answer
	// are an unsigned int carrying the size of the answer
	// (those 4 bytes are NOT included in the total)
	bool				ReadAnswer( int len );
	// called by the above just after updating DataLen
	virtual bool		IsAnswerComplete( void );

	// default protocol: the first 4 bytes of the request
	// are an unsigned int carrying the size of the answer
	// (those 4 bytes are NOT included in the total)
	virtual void		WaitRequest( void );
	virtual void		Dispatch( void ) = 0;
};

#endif
