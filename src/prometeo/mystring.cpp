/***************************************************************************
                                 mystring.cpp
                             -------------------
	revision             : $Id: mystring.cpp,v 1.2 2002-10-13 15:40:12 tellini Exp $
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

#include "main.h"

#include <stdlib.h>
#include <string.h>

#include "mystring.h"

//--------------------------------------------------------------------------
int MyString::ToInt( void ) const
{
	return( atoi( c_str() ));
}
//--------------------------------------------------------------------------
void MyString::Explode( const char *separator, StringList& list )
{
	const char	*cur = c_str(), *start = cur, *ptr;
	int			len = strlen( separator );

	while( ptr = strstr( cur, separator )) {

		list.Add( "%s", substr( cur - start, ptr - cur ).c_str() );

		cur = ptr + len;
	}

	list.Add( "%s", cur );
}
//--------------------------------------------------------------------------
