/***************************************************************************
                                   list.h
                             -------------------
	revision             : $Id: list.h,v 1.3 2003-12-29 22:24:41 tellini Exp $
    copyright            : (C) 2002-2004 by Simone Tellini
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
	void			Set( int index, void *item );
	
	void			Add( void *item );
	
	void			*Remove( int index );
	void			*Remove( void *item );
	void			*RemoveLast( void );
	
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
