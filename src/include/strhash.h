/***************************************************************************
                                  strhash.h
                             -------------------
	revision             : $Id: strhash.h,v 1.2 2002-11-18 17:46:31 tellini Exp $
    copyright            : (C) 2002 by Simone Tellini
    email                : tellini@users.sourceforge.net

	description          : an hash table for string-keyed items
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef PROM_STRHASH_H
#define PROM_STRHASH_H

#include "linkedlist.h"

class HashItem : public LinkedListNode
{
public:
					HashItem();
					~HashItem();

	char			*String;
	void			*Data;
};

class StrHash
{
public:
					StrHash( unsigned int size );
					~StrHash();

	void			Clear( void );

	unsigned int 	ComputeHash( const char *str ) const;

	bool			Add( const char *str, void *data );
	void			Replace( const char *str, void *data );

	void			Remove( const char *str );

	HashItem 		*Find( const char *str ) const;
	void			*FindData( const char *str ) const; // returns the Data field

	void			*GetData( int index ) const;

	unsigned int	GetSize( void ) const { return( Size ); }
	unsigned int	Count( void ) const { return( NumItems ); }

private:
	unsigned int	Size;
	LinkedList		*Items;
	unsigned int	NumItems;

	void 			Remove( HashItem *item );
};

#endif
