/***************************************************************************
                                  bitfield.h
                             -------------------
	revision             : $Id: bitfield.h,v 1.1.1.1 2002-10-10 09:59:09 tellini Exp $
    copyright            : (C) 2002 by Simone Tellini
    email                : tellini@users.sourceforge.net

	description          : utility class to manipulate flags
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef PROM_BITFIELD_H
#define PROM_BITFIELD_H

class BitField
{
public:

					BitField() : Flags(0) {};

	void			Set( unsigned int f )
					{
						Flags |= f;
					}

	void			Set( unsigned int f, bool set )
					{
						if( set )
							Set( f );
						else
							Clear( f );
					}

	void			Clear( unsigned int f )
					{
						Flags &= ~f;
					}

	void			Clear( void )
					{
						Flags = 0;
					}

	bool			IsSet( unsigned int f ) const
					{
						return(( Flags & f ) ? true : false );
					}

private:
	unsigned int	Flags;
};

#endif
