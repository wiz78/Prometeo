/***************************************************************************
                                regnodee.cpp
                             -------------------
	revision             : $Id: regnode.cpp,v 1.3 2002-11-08 14:32:32 tellini Exp $
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
#include <fstream>

#include "regnode.h"
#include "regkey.h"

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

	if( Parent )
		Parent->RemoveChild( this );
}
//--------------------------------------------------------------------------
void RegNode::WriteTabs( ofstream& fh, int indent )
{
	while( indent-- > 0 )
		fh << "\t";
}
//--------------------------------------------------------------------------
