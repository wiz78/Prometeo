/***************************************************************************
                               processgroup.ch
                             -------------------
	revision             : $Id: processgroup.h,v 1.2 2002-11-01 22:23:46 tellini Exp $
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

#ifndef PROM_PROCESSGROUP_H
#define PROM_PROCESSGROUP_H

#include "list.h"
#include "buffer.h"
#include "linkedlist.h"

// in case of error, answer is NULL and len is errno
typedef void ( *Prom_IPC_Callback )( void *answer, int len, void *userdata );

class Process;
class IODispatcher;

class ProcessGroup
{
public:
					ProcessGroup( int numchildren, IODispatcher *io );
					~ProcessGroup();

	void			SendRequest( Buffer *req, Prom_IPC_Callback callback, void *userdata );
	void			RunQueue( void );

	void			AddChild( Process *child );

	// if more than MinChildren are present, it terminates those
	// that have been idle longer than ChildrenDecayTime seconds
	void			Flush( time_t now );
	
	void			ReloadCfg( void );
	
protected:
	IODispatcher 	*IO;
	List			Children;
	LinkedList		Requests;
	unsigned int	MinChildren;
	unsigned int	ChildrenDecayTime;

	Process 		*FindIdleProcess( void );
	void			Enqueue( Buffer *req, Prom_IPC_Callback callback, void *userdata );
};

class Prom_IPCRequest : public LinkedListNode
{
public:
	Prom_IPC_Callback	Callback;
	Buffer				*Request;
	void				*UserData;
};

#endif
