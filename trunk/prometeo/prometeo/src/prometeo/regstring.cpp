/***************************************************************************
                                regstring.cpp
                             -------------------
	revision             : $Id: regstring.cpp,v 1.1 2002-10-10 10:22:59 tellini Exp $
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

#include <string.h>
#include <stdlib.h>
#include <fstream.h>

#include "regstring.h"

//--------------------------------------------------------------------------
RegString::RegString( const char *name ) : RegNode( name, REG_STRING )
{
	Value = NULL;
}
//--------------------------------------------------------------------------
RegString::~RegString()
{
	free( Value );
}
//--------------------------------------------------------------------------
void RegString::SetValue( const char *val )
{
	free( Value );

	Value = strdup( val );
}
//--------------------------------------------------------------------------
void RegString::Save( ofstream& fh, int indent )
{
	WriteTabs( fh, indent );
	
	fh << "<Value type=\"string\" name=\"" << Name << "\">";
	fh << Value;
	fh << "</Value>" << endl;
}
//--------------------------------------------------------------------------
