/***************************************************************************
                                  buffer.h
                             -------------------
	revision             : $Id: buffer.h,v 1.1.1.1 2002-10-10 09:59:09 tellini Exp $
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

#ifndef BUFFER_H
#define BUFFER_H


class Buffer
{
public:
					Buffer();
					Buffer::Buffer( const void *data, int len );
					~Buffer();

	char 			operator[]( int index );

	char			*GetData( void ) { return( Data ); }
	unsigned int	GetSize( void ) const { return( Size ); }

	void			Clear( void );
	virtual void	Append( const char *data, int len );
	virtual void	SetContent( const char *data, int len );

	void			Resize( int size );

protected:
	char			*Data;
	unsigned int	Size;
};

#endif
