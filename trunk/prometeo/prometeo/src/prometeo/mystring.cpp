/***************************************************************************
                                 mystring.cpp
                             -------------------
	revision             : $Id: mystring.cpp,v 1.3 2002-10-29 18:01:15 tellini Exp $
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
