/***************************************************************************
                                  regkey.h
                             -------------------
	revision             : $Id: regkey.h,v 1.2 2003-02-07 14:10:58 tellini Exp $
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

#ifndef REGKEY_H
#define REGKEY_H

#include "regnode.h"
#include "list.h"

class RegKey : public RegNode
{
public:
					RegKey( const char *name );
	virtual			~RegKey();

	void			AddChild( RegNode *key );
	void			RemoveChild( RegNode *key );

	RegKey			*FindKey( const char *path );
	RegKey			*GetKey( int index );

	RegNode			*FindValue( const char *name, RegType type );
	RegNode			*GetValue( int index );

	virtual char	*GetTag( void ) { return( "Key" ); }

	virtual void	Save( ofstream& fh, int indent );

private:
	List			Children;
};

#endif
