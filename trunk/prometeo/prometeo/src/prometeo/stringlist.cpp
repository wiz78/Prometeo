/***************************************************************************
                               stringlist.cpp
                             -------------------
    revision             : $Id: stringlist.cpp,v 1.4 2002-11-15 16:26:46 tellini Exp $
    copyright            : (C) 2002 by Simone Tellini
    email                : tellini@users.sourceforge.net

    description          : a simple string list
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

#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include "stringlist.h"

//---------------------------------------------------------------------------
StringList::~StringList()
{
	Clear();
}
//---------------------------------------------------------------------------
void StringList::Add( const string& str )
{
	List::Add( strdup( str.c_str() ));
}
//---------------------------------------------------------------------------
void StringList::Add( const char *fmt, ... )
{
	va_list	ap;
	char	buf[1024];

	va_start( ap, fmt );

	vsnprintf( buf, sizeof( buf ), fmt, ap );

	va_end( ap );

	List::Add( strdup( buf ));
}
//---------------------------------------------------------------------------
void StringList::Set( int index, const char *str )
{
	if(( index < 0 ) || ( index > Count() ))
		throw "StringList::Set() - index out of bounds!";

	if( index == Count() )
		Add( "%s", str );
	else {

		free( List::Get( index ));

		List::Set( index, strdup( str ));
	}
}
//---------------------------------------------------------------------------
void StringList::Clear( void )
{
	for( int i = Count() - 1; i >= 0; i-- )
		free( List::Get( i ));

	List::Clear();
}
//---------------------------------------------------------------------------
void StringList::Explode( const string& str, const char *separator )
{
	const char	*cur = str.c_str(), *start = cur, *ptr;
	int			len = strlen( separator );

	while( ptr = strstr( cur, separator )) {

		Add( "%s", str.substr( cur - start, ptr - cur ).c_str() );

		cur = ptr + len;
	}

	Add( "%s", cur );
}
//---------------------------------------------------------------------------
string StringList::Implode( const char *separator )
{
	string	ret;

	for( int i = 0; i < Count(); i++ ) {

		if( i )
			ret += separator;

		ret += Get( i );
	}

	return( ret );
}
//--------------------------------------------------------------------------
