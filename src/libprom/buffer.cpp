/***************************************************************************
                                 buffer.cpp
                             -------------------
	revision             : $Id: buffer.cpp,v 1.1.1.1 2002-10-10 09:59:19 tellini Exp $
    copyright            : (C) 2002 by Simone Tellini
    email                : tellini@users.sourceforge.net
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <stdlib.h>
#include <string.h>

#include "buffer.h"

//--------------------------------------------------------------------------
Buffer::Buffer()
{
	Data = NULL;
	Size = 0;
}
//--------------------------------------------------------------------------
Buffer::Buffer( const void *data, int len )
{
	Data = NULL;
	Size = 0;

	if( len > 0 ) {

		Resize( len );

		memcpy( Data, data, len );
	}
}
//--------------------------------------------------------------------------
Buffer::~Buffer()
{
	Clear();
}
//--------------------------------------------------------------------------
char Buffer::operator[]( int index )
{
	if((unsigned int)index >= Size )
		throw "Buffer index out of bounds!";

	return( Data[ index ] );
}
//--------------------------------------------------------------------------
void Buffer::Append( const char *data, int len )
{
	if( len > 0 ) {
		int old = Size;

		Resize( Size + len );

		memcpy( &Data[ old ], data, len );
	}
}
//--------------------------------------------------------------------------
void Buffer::Clear( void )
{
	if( Data ) {
		free( Data );
		Data = NULL;
	}

	Size = 0;
}
//--------------------------------------------------------------------------
void Buffer::SetContent( const char *data, int len )
{
	Clear();
	Append( data, len );
}
//--------------------------------------------------------------------------
void Buffer::Resize( int size )
{
	Size = size;
	Data = (char *)realloc( Data, size );

	if( !Data )
		throw "Buffer::Resize() - out of memory!";
}
//--------------------------------------------------------------------------
