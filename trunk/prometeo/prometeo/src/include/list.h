/***************************************************************************
                                   list.h
                             -------------------
	revision             : $Id: list.h,v 1.1.1.1 2002-10-10 09:59:12 tellini Exp $
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

#ifndef LIST_H
#define LIST_H


class List
{
public:
					List();
					~List();

	void 			*operator[]( int index ) const;

	void			*Get( int index ) const;
	void			Add( void *item );
	void			*Remove( int index );
	void			*Remove( void *item );
	unsigned int	Count( void ) const { return( ItemsCount ); }
	virtual void	Clear( void );

	void			SetAllocBy( unsigned int a ) { AllocBy = a; }

protected:
	void			**Items;
	unsigned int	ItemsCount;
	unsigned int	Allocated;
	unsigned int	AllocBy;
};

#endif
