/***************************************************************************
                               reginteger.cpp
                             -------------------
	revision             : $Id: reginteger.cpp,v 1.2 2002-10-13 15:40:12 tellini Exp $
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

#include <fstream>

#include "reginteger.h"

//--------------------------------------------------------------------------
RegInteger::RegInteger( const char *name ) : RegNode( name, REG_INTEGER )
{
}
//--------------------------------------------------------------------------
void RegInteger::Save( ofstream& fh, int indent )
{
	WriteTabs( fh, indent );
	
	fh << "<Value type=\"integer\" name=\"" << Name << "\">";
	fh << Value;
	fh << "</Value>" << endl;
}
//--------------------------------------------------------------------------
