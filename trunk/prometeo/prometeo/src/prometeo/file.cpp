/***************************************************************************
                                  file.cpp
                             -------------------
	revision             : $Id: file.cpp,v 1.1 2002-10-10 10:22:59 tellini Exp $
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

#include "main.h"

#include <sys/stat.h>
#include <fcntl.h>

#include "file.h"
#include "iodispatcher.h"

//---------------------------------------------------------------------------
File::File()
{
	AsyncCallback = NULL;
	AsyncUserData = NULL;
}
//---------------------------------------------------------------------------
File::File( const char *name, int flags )
{
	AsyncCallback = NULL;
	AsyncUserData = NULL;

	Open( name, flags );
}
//---------------------------------------------------------------------------
File::~File()
{
	FlushWriteBuffers( false );

	Close();
}
//---------------------------------------------------------------------------
void File::FlushWriteBuffers( bool keep )
{
	if( keep )
		SetNonBlocking( false );

	Flags.Set( PROM_FILEF_FLUSH );

	while( FileBuf *buf = (FileBuf *)WriteBuffers.RemTail() ) {

		if( keep )
			write( FD, buf->GetData(), buf->GetSize() );

		delete buf;
	}

	Flags.Clear( PROM_FILEF_FLUSH );
}
//---------------------------------------------------------------------------
void File::SetAsyncCallback( Prom_FileCallback callback, void *userdata )
{
	AsyncCallback = callback;
	AsyncUserData = userdata;
}
//---------------------------------------------------------------------------
bool File::Open( const char *name, int flags )
{
	int	fl = 0;

	Flags.Set( flags );

	if( flags & PROM_FILEF_APPEND ) {
		flags |= PROM_FILEF_WRITE | PROM_FILEF_READ;
		fl    |= O_CREAT;
	} else if( flags & PROM_FILEF_WRITE )
		fl |= O_TRUNC | O_CREAT;

	if(( flags & PROM_FILEF_WRITE ) && !( flags & PROM_FILEF_READ ))
		fl |= O_WRONLY;
	else if(!( flags & PROM_FILEF_WRITE ) && ( flags & PROM_FILEF_READ ))
		fl |= O_RDONLY;
	else
		fl |= O_RDWR;

	if( flags & PROM_FILEF_NONBLOCK )
		fl |= O_NONBLOCK;

	FD = open( name, fl, 0666 );

	if( IsValid() && ( flags & PROM_FILEF_APPEND ))
		Seek( 0, PROM_FILE_SEEK_END );

	return( IsValid() );
}
//---------------------------------------------------------------------------
bool File::Close( void )
{
	bool ok = WriteBuffers.IsEmpty();

	if( ok ) {

		if( Dispatcher )
			Dispatcher->RemFD( this, PROM_IOF_READ | PROM_IOF_WRITE | PROM_IOF_EXCEPT );

		if( IsValid() ) {
			close( FD );
			FD = -1;
		}

		Flags.Clear();

	} else {

		Flags.Set( PROM_FILEF_FLUSH );

		if( !Flags.IsSet( PROM_FILEF_WRITING ))
			HandleWrite();
	}

	return( ok );
}
//---------------------------------------------------------------------------
int File::Read( void *buf, int len )
{
	int	nread;

	nread = read( FD, buf, len );

	if( nread >= 0 ) {

		Callback( PROM_FILE_READ, nread );
			
	} else {

		if(( errno == EAGAIN ) || ( errno == EWOULDBLOCK )) {

			ReadBuf    = buf;
			ReadBufLen = len;

			if( Dispatcher )
				Dispatcher->AddFD( this, PROM_IOF_READ );

		} else
			Callback( PROM_FILE_ERROR, errno );
	}

	return( nread );
}
//---------------------------------------------------------------------------
bool File::Write( const void *buf, int len )
{
	bool 	ret = true;
	bool	full;

	full = EnqueueData( buf, len );

	// only write when a buffer is full
	// skip if we're already waiting to write
	if( full && !Flags.IsSet( PROM_FILEF_WRITING ))
		HandleWrite();

	return( ret );
}
//---------------------------------------------------------------------------
bool File::Write( const string& str )
{
	unsigned int len = str.length();

	return( Write( &len, sizeof( len )) && Write( str.c_str(), len ));
}
//---------------------------------------------------------------------------
bool File::Read( string& str )
{
	unsigned int len;
	bool		 ok = false;

	if( Read( &len, sizeof( len )) == sizeof( len )) {
		char *tmp = new char[ len + 1 ];

		ok = Read( tmp, len ) == len;

		tmp[ len ] = '\0';

		str = tmp;

		delete[] tmp;
	}

	return( ok );
}
//---------------------------------------------------------------------------
bool File::EnqueueData( const void *buf, int len )
{
	bool	full = false;

	while( len > 0 ) {
		FileBuf	*fb = (FileBuf *)WriteBuffers.GetTail();
		int		done = WriteBuffers.IsNode( fb ) ? fb->Append( buf, len ) : 0;

		if( done ) {

			len -= done;
			buf  = (void *)((char *)buf + done );

		} else {

			if( WriteBuffers.IsNode( fb ))
				full = true;

			// no room left, create another buffer
			WriteBuffers.AddTail( new FileBuf() );
		}
	}

	return( full );
}
//---------------------------------------------------------------------------
bool File::Seek( int offset, int whence )
{
	return( lseek( FD, offset, whence ) != -1 );
}
//---------------------------------------------------------------------------
unsigned int File::GetSize( void )
{
	unsigned int	size = 0;
	struct stat		buf;

	if( !fstat( FD, &buf ))
		size = buf.st_size;

	return( size );
}
//---------------------------------------------------------------------------
void File::HandleRead( void )
{
	int	nr = read( FD, ReadBuf, ReadBufLen );

	if( Dispatcher )
		Dispatcher->RemFD( this, PROM_IOF_READ );

	if( nr == 0 )
		Callback( PROM_FILE_READ, 0 );

	else if( nr > 0 ) {

		ReadBuf     = (void *)((char *)ReadBuf + nr );
		ReadBufLen -= nr;

		if( ReadBufLen && Dispatcher )
			Dispatcher->AddFD( this, PROM_IOF_READ );
		else if( !ReadBufLen )
			Callback( PROM_FILE_READ, nr );

	} else if(( errno == EAGAIN ) || ( errno == EWOULDBLOCK )) {

		if( Dispatcher )
			Dispatcher->AddFD( this, PROM_IOF_READ );

	} else
		Callback( PROM_FILE_ERROR, errno );
}
//---------------------------------------------------------------------------
void File::HandleWrite( void )
{
	FileBuf	*buf = (FileBuf *)WriteBuffers.GetHead();
	bool	err = false;

	Flags.Clear( PROM_FILEF_WRITING );

	if( Dispatcher )
		Dispatcher->RemFD( this, PROM_IOF_WRITE );

	while( WriteBuffers.IsNode( buf ) &&
	       ( Flags.IsSet( PROM_FILEF_FLUSH ) || buf->IsFull() )) {
		int written;

		written = write( FD, buf->GetData(), buf->GetSize() );

		if( written > 0 ) {

			if( buf->Processed( written )) {

				delete buf;

				buf = (FileBuf *)WriteBuffers.GetHead();
			}

		} else {

			buf = NULL;
			err = true;
		}
	}

	if( err ) {

		if(( errno == EAGAIN ) || ( errno == EWOULDBLOCK )) {

			if( Dispatcher )
				Dispatcher->AddFD( this, PROM_IOF_WRITE );

			Flags.Set( PROM_FILEF_WRITING );

		} else
			Callback( PROM_FILE_ERROR, errno );

	} else if( !WriteBuffers.IsEmpty() ) {

		buf = (FileBuf *)WriteBuffers.GetHead();

		if( buf->IsFull() || Flags.IsSet( PROM_FILEF_FLUSH )) {

			if( Dispatcher )
				Dispatcher->AddFD( this, PROM_IOF_WRITE );

			Flags.Set( PROM_FILEF_WRITING );
		}
	}
}
//---------------------------------------------------------------------------
void File::ResumeWrite( void )
{
	if( !WriteBuffers.IsEmpty() )
		HandleWrite();
}
//---------------------------------------------------------------------------
void File::Callback( Prom_FC_Reason reason, int data )
{
	if( AsyncCallback )
		( *AsyncCallback )( this, reason, data, AsyncUserData );
}
//---------------------------------------------------------------------------
int FileBuf::Append( const void *buf, int len )
{
	int	left = sizeof( Data ) - Size;

	if( left < len )
		len = left;

	if( len ) {

		memcpy( &Data[ Size ], buf, len );

		Size += len;
	}

	return( len );
}
//---------------------------------------------------------------------------

