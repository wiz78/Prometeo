/***************************************************************************
                                 regstring.h
                             -------------------
	revision             : $Id: regstring.h,v 1.2 2003-02-07 14:10:58 tellini Exp $
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

#ifndef REGSTRING_H
#define REGSTRING_H

#include "regnode.h"

class RegString : public RegNode
{
public:
					RegString( const char *name );
	virtual			~RegString();

	char			*GetValue( void ) { return( Value ); }
	void			SetValue( const char *val );

	virtual char	*GetTag( void ) { return( "Value" ); }

	virtual void	Save( ofstream& fh, int indent );

protected:
	char			*Value;
};

#endif
