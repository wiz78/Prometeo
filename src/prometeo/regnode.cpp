/***************************************************************************
                                regnodee.cpp
                             -------------------
	revision             : $Id: regnode.cpp,v 1.1 2002-10-10 10:22:59 tellini Exp $
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

#include <stdlib.h>
#include <string.h>
#include <fstream>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "regnode.h"

//--------------------------------------------------------------------------
RegNode::RegNode( const char *name, RegType type )
{
	Name   = strdup( name );
	Type   = type;
	Parent = NULL;
}
//--------------------------------------------------------------------------
RegNode::~RegNode()
{
	free( Name );
}
//--------------------------------------------------------------------------
void RegNode::WriteTabs( ofstream& fh, int indent )
{
	while( indent-- > 0 )
		fh << "\t";
}
//--------------------------------------------------------------------------
