/***************************************************************************
                                  list.cpp
                             -------------------
	revision             : $Id: list.cpp,v 1.1 2002-10-10 10:22:59 tellini Exp $
    copyright            : (C) 2002 by Simone Tellini
    email                : tellini@users.sourceforge.net

	description          ; a list class similar to Borland's TList
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

#ifdef HAVE_STRING_H
#include <string.h>
#else
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#endif

#include "list.h"

//--------------------------------------------------------------------------
List::List()
{
	Items      = NULL;
	ItemsCount = 0;
	Allocated  = 0;
	AllocBy    = 10;
}
//--------------------------------------------------------------------------
List::~List()
{
	Clear();
}
//--------------------------------------------------------------------------
void *List::operator[]( int index ) const
{
	return( Get( index ));
}
//--------------------------------------------------------------------------
void *List::Get( int index ) const
{
	if((unsigned int)index >= ItemsCount )
		throw "List index out of bounds!";

	return( Items[ index ] );
}
//--------------------------------------------------------------------------
void List::Add( void *item )
{
	if( ItemsCount >= Allocated ) {
		Allocated += AllocBy;
		Items      = (void **)realloc( Items, sizeof( void * ) * Allocated );
	}

	if( !Items )
		throw "List::Add() - Out of memory!";

	Items[ ItemsCount++ ] = item;
}
//--------------------------------------------------------------------------
void *List::Remove( int index )
{
	void	*item;
	int		succ;

	if((unsigned int)index >= ItemsCount )
		throw "List index out of bounds!";

	item = Items[ index ];
	succ = --ItemsCount - index;

	if( succ )
		memcpy( &Items[ index ], &Items[ index + 1 ], sizeof( Items[0] ) * succ );

	// shrink when there are too many empty slots
	if(( Allocated - ItemsCount ) > ( AllocBy + ( AllocBy / 2 ))) {
		Allocated -= AllocBy;
		Items      = (void **)realloc( Items, sizeof( void * ) * Allocated );
	}

	return( item );
}
//--------------------------------------------------------------------------
void *List::Remove( void *item )
{
	void	*it = NULL;

	for( int i = 0; !it && ( i < ItemsCount ); i++ )
		if( Items[ i ] == item )
			it = Remove( i );

	return( it );
}
//--------------------------------------------------------------------------
void List::Clear( void )
{
	free( Items );

	Items      = NULL;
	ItemsCount = 0;
	Allocated  = 0;
}
//--------------------------------------------------------------------------
