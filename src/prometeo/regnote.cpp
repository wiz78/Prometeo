/***************************************************************************
                                 regnote.cpp
                             -------------------
	revision             : $Id: regnote.cpp,v 1.1 2002-10-10 10:22:59 tellini Exp $
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

#include <fstream>

#include "regnote.h"

//--------------------------------------------------------------------------
RegNote::RegNote() : RegString( "" )
{
	Type = REG_NOTE;
}
//--------------------------------------------------------------------------
void RegNote::Save( ofstream& fh, int indent )
{
	WriteTabs( fh, indent );

	fh << "<Note>" << Value << "</Note>" << endl;
}
//--------------------------------------------------------------------------
