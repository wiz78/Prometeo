/***************************************************************************
                               stringlist.cpp
                             -------------------
    revision             : $Id: stringlist.cpp,v 1.2 2002-10-13 15:40:12 tellini Exp $
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
void StringList::Clear( void )
{
	for( int i = Count() - 1; i >= 0; i-- )
		free( List::Get( i ));

	List::Clear();
}
//---------------------------------------------------------------------------
