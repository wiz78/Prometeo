/***************************************************************************
                               iodispatcher.h
                             -------------------
	revision             : $Id: iodispatcher.h,v 1.1.1.1 2002-10-10 09:59:11 tellini Exp $
    copyright            : (C) 2002 by Simone Tellini
    email                : tellini@users.sourceforge.net

	description          : this class handles the multiplexed I/O,
                           dispatching the read/write/exception events
						   to the right objects
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef PROM_IODISPATCHER_H
#define PROM_IODISPATCHER_H

#include <sys/select.h>
#include <sys/types.h>

class Fd;

#define MAX_FDS		1024 // max number of fd's we can handle

class IODispatcher
{
public:
				IODispatcher();

	void		AddFD( Fd *fd, int flags );
	void		RemFD( Fd *fd, int flags );

	// return false for errors or timeout
	bool		WaitEvents( int timeout = -1 );
	
	void		CheckTimeouts( time_t now );
	
private:
	struct {
		Fd		*FD;
		int		Flags;
	}			FDTable[ MAX_FDS ];
	fd_set		ReadSet;
	fd_set		WriteSet;
	fd_set		ExceptSet;
	int			MaxFD;

	inline void	Dispatch( fd_set *fds, int type );
};

// AddFD()/RemFD() flags
#define PROM_IOF_READ		(1 << 0)
#define PROM_IOF_WRITE		(1 << 1)
#define PROM_IOF_EXCEPT		(1 << 2)

#endif

