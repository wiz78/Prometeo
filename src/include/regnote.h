/***************************************************************************
                                  regnote.h
                             -------------------
	revision             : $Id: regnote.h,v 1.1.1.1 2002-10-10 09:59:15 tellini Exp $
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

#ifndef REGNOTE_H
#define REGNOTE_H

#include "regstring.h"

class RegNote : public RegString
{
public:
					RegNote();

	virtual char	*GetTag( void ) { return( "Note" ); }

	virtual void	Save( ofstream& fh, int indent );
};

#endif
