/***************************************************************************
                                   file.h
                             -------------------
	revision             : $Id: file.h,v 1.3 2003-02-16 20:31:14 tellini Exp $
    copyright            : (C) 2002 by Simone Tellini
    email                : tellini@users.sourceforge.net

	description          : file I/O class
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef FILE_H
#define FILE_H

#include <string>

#include "fd.h"
#include "bitfield.h"
#include "linkedlist.h"

class File : public Fd
{
public:
						File();
						File( const char *name, int flags );
						File( int fd );
	virtual				~File();

	bool				Open( const char *name, int flags );
	// returns false if there are unwritten buffers pending
	bool				Close( void );

	int					Read( void *buf, int len );
	bool				Write( const void *buf, int len );
	bool				Seek( int offset, int whence );

	bool				Write( const string& str );
	bool				Read( string& str ); // NOTE! Won't work with non-blocking files!

	unsigned int		GetSize( void );

	// if keep = true, pending buffers will be written to disk
	// otherwise they'll just be discarded
	void				FlushWriteBuffers( bool keep = false );
	// in case you want to try again a failed async write
	void				ResumeWrite( void );

	// a callback MUST be set when using PROM_FILEF_NONBLOCK
	// alternatively, a descendant class may override the Callback method
	void				SetAsyncCallback( Prom_FileCallback callback, void *userdata );

	// Fd interface implementation
	virtual void		HandleRead( void );
	virtual void		HandleWrite( void );

protected:
	BitField			Flags;

private:
	LinkedList			WriteBuffers;
	void				*ReadBuf;
	int					ReadBufLen;
	Prom_FileCallback	AsyncCallback;
	void				*AsyncUserData;

	bool				EnqueueData( const void *buf, int len );
	virtual void		Callback( Prom_FC_Reason reason, int data );
};

// Flags
// Open() flags
#define PROM_FILEF_NONBLOCK		(1 << 0)
#define PROM_FILEF_READ			(1 << 1)	// open for reading
#define PROM_FILEF_WRITE		(1 << 2)	// open for writing
#define PROM_FILEF_APPEND		(1 << 3)	// open in R/W mode, don't truncate
// private flags
#define PROM_FILEF_WRITING		(1 << 4)
#define PROM_FILEF_FLUSH		(1 << 5)
#define PROM_FILEF_READING		(1 << 6)

// Seek whence
#define PROM_FILE_SEEK_BEG		0
#define PROM_FILE_SEEK_CUR		1
#define PROM_FILE_SEEK_END		2

#define PROM_BUF_SIZE			4096

class FileBuf : public LinkedListNode
{
public:
				FileBuf() : Size( 0 ), Done( 0 ) {}

	int			GetSize( void ) const { return( Size - Done ); }
	char		*GetData( void ) { return( &Data[ Done ] ); }

	int			Append( const void *buf, int len );
	bool		Processed( int len ) { Done += len; return( Done >= Size ); }

	bool		IsFull( void ) const { return( Size >= sizeof( Data )); }

private:
	char		Data[ PROM_BUF_SIZE ];
	int			Size;
	int			Done;
};

#endif
