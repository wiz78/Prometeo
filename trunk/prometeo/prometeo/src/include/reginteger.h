/***************************************************************************
                                reginteger.h
                             -------------------
	revision             : $Id: reginteger.h,v 1.1.1.1 2002-10-10 09:59:14 tellini Exp $
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

#ifndef REGINTEGER_H
#define REGINTEGER_H

#include "regnode.h"

class RegInteger : public RegNode
{
public:
					RegInteger( const char *name );

	int				GetValue( void ) { return( Value ); }
	void			SetValue( int val ) { Value = val; }

	virtual char	*GetTag( void ) { return( "Value" ); }

	virtual void	Save( ofstream& fh, int indent );

private:
	int				Value;
};

#endif
