/***************************************************************************
                              iodispatcher.cpp
                             -------------------
	revision             : $Id: iodispatcher.cpp,v 1.1.1.1 2002-10-10 09:59:20 tellini Exp $
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

#include "main.h"
#include "iodispatcher.h"
#include "fd.h"

#define PROM_IODISPATCHER_FAST_CHECK 1

//---------------------------------------------------------------------------
IODispatcher::IODispatcher()
{
	memset( FDTable, 0, sizeof( FDTable ));

	FD_ZERO( &ReadSet );
	FD_ZERO( &WriteSet );
	FD_ZERO( &ExceptSet );

	MaxFD = 0;
}
//---------------------------------------------------------------------------
void IODispatcher::AddFD( Fd *fd, int flags )
{
	if( fd->IsValid() ) {
		int	fdnum = fd->GetFD();

		FDTable[ fdnum ].FD     = fd;
		FDTable[ fdnum ].Flags |= flags;

		if( flags & PROM_IOF_READ )
			FD_SET( fdnum, &ReadSet );

		if( flags & PROM_IOF_WRITE )
			FD_SET( fdnum, &WriteSet );

		if( flags & PROM_IOF_EXCEPT )
			FD_SET( fdnum, &ExceptSet );

		if( ++fdnum > MaxFD )
			MaxFD = fdnum;
	}
}
//---------------------------------------------------------------------------
void IODispatcher::RemFD( Fd *fd, int flags )
{
	int	fdnum = fd->GetFD();
		
	if( fd->IsValid() && ( FDTable[ fdnum ].FD == fd )) {

		if( flags & PROM_IOF_READ )
			FD_CLR( fdnum, &ReadSet );

		if( flags & PROM_IOF_WRITE )
			FD_CLR( fdnum, &WriteSet );

		if( flags & PROM_IOF_EXCEPT )
			FD_CLR( fdnum, &ExceptSet );

		FDTable[ fdnum ].Flags &= ~flags;

		if( !FDTable[ fdnum ].Flags ) {

			FDTable[ fdnum ].FD = NULL;

			// find out which the new max fd is
			MaxFD--;

			while(( MaxFD >= 0 ) && !FDTable[ MaxFD ].FD )
				MaxFD--;
			
			// we actually need max fd + 1
			MaxFD++;
		}
	}
}
//---------------------------------------------------------------------------
bool IODispatcher::WaitEvents( int timeout )
{
	bool			ret = false;
	struct timeval	tv, *to = NULL;
	fd_set			read, write, exc;

	if( timeout != -1 ) {

		memset( &tv, 0, sizeof( tv ));

		tv.tv_sec = timeout;
		to        = &tv;
	}

	memcpy( &read,  &ReadSet,   sizeof( read  ));
	memcpy( &write, &WriteSet,  sizeof( write ));
	memcpy( &exc,   &ExceptSet, sizeof( exc   ));

	if( select( MaxFD, &read, &write, &exc, to ) > 0 ) {

		ret = true;

		Dispatch( &read,  PROM_IOF_READ   );
		Dispatch( &write, PROM_IOF_WRITE  );
		Dispatch( &exc,   PROM_IOF_EXCEPT );
	}

	return( ret );
}
//---------------------------------------------------------------------------
void IODispatcher::CheckTimeouts( time_t now )
{
	for( int i = 0; i < MaxFD; i++ ) {
		Fd	*fd = FDTable[ i ].FD;

		if( fd && fd->HasTimedOut( now ))
			fd->HandleTimeout();
	}
}
//---------------------------------------------------------------------------
void IODispatcher::Dispatch( fd_set *fds, int type )
{
	// call handlers for the fd's that received an event
#if PROM_IODISPATCHER_FAST_CHECK
	// this method is the fastest one, although I don't know how much
	// portable it is

#define DISPATCH( fd ) \
{ \
	int stop = (fd) + 8; \
	if( MaxFD < stop ) \
		stop = MaxFD; \
	for( int i = (fd); i < stop; i++ ) \
		if( FD_ISSET( i, fds ) && ( FDTable[ i ].Flags & type )) { \
\
			switch( type ) {\
\
				case PROM_IOF_READ:\
					FDTable[ i ].FD->HandleRead();\
					break;\
\
				case PROM_IOF_WRITE:\
					FDTable[ i ].FD->HandleWrite();\
					break;\
\
				case PROM_IOF_EXCEPT:\
					FDTable[ i ].FD->HandleExcept();\
					break;\
			}\
		}\
}

	int				bytes = ( MaxFD + 7 ) / 8;
	int 			words = bytes / 4;
	int				fdnum = 0;
	unsigned int	*ptr = (unsigned int *)fds;
	unsigned char 	*bptr;

	// check 32 fds at once
	while( words-- > 0 ) {

		if( *ptr ) { // at least an FD is set here

			bptr = (unsigned char *)ptr;

			if( *bptr++ )
				DISPATCH( fdnum );

			if( *bptr++ )
				DISPATCH( fdnum + 8 );

			if( *bptr++ )
				DISPATCH( fdnum + 16 );

			if( *bptr )
				DISPATCH( fdnum + 24 );
		}

		ptr++;

		fdnum += 32;
		bytes -= 4;
	}

	bptr = (unsigned char *)ptr;

	// check the remaining bytes
	while( bytes-- > 0 ) {

		if( *bptr++ )
			DISPATCH( fdnum );

		fdnum += 8;
	}

#else
	// plain, standard, boring way :)
	for( int i = 0; i < MaxFD; i++ ) {

		if( FD_ISSET( i, fds ) && ( FDTable[ i ].Flags & type )) {

			switch( type ) {

				case PROM_IOF_READ:
					FDTable[ i ].FD->HandleRead();
					break;

				case PROM_IOF_WRITE:
					FDTable[ i ].FD->HandleWrite();
					break;

				case PROM_IOF_EXCEPT:
					FDTable[ i ].FD->HandleExcept();
					break;
			}
		}
	}
#endif
}
//---------------------------------------------------------------------------
