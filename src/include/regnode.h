/***************************************************************************
                                  regnode.h
                             -------------------
	revision             : $Id: regnode.h,v 1.2 2002-10-13 15:40:12 tellini Exp $
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

#ifndef REGNODE_H
#define REGNODE_H

#include <fstream>

class RegKey;

typedef enum { REG_KEY, REG_INTEGER, REG_STRING, REG_NOTE } RegType;

class RegNode
{
public:
					RegNode( const char *name, RegType type );
					~RegNode();

	void			SetParent( RegKey *par ) { Parent = par; }
	RegKey			*GetParent( void ) { return( Parent ); }

	RegType			GetType( void ) { return( Type ); }
	char			*GetName( void ) { return( Name ); }
	virtual char	*GetTag( void ) = 0;

	virtual void	Save( ofstream& fh, int indent ) = 0;
	void			WriteTabs( ofstream& fh, int indent );

protected:
	char			*Name;
	RegType			Type;
	RegKey			*Parent;
};

#endif
