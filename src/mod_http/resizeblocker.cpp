/***************************************************************************
                              resizeblocker.cpp
                             -------------------
    revision             : $Id: resizeblocker.cpp,v 1.1 2003-04-06 10:57:37 tellini Exp $
    copyright            : (C) 2003 by Simone Tellini
    email                : tellini@users.sourceforge.net

    description          : blocks the resizeTo method
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "resizeblocker.h"

//---------------------------------------------------------------------------
bool ResizeBlocker::Process( void )
{
	int res_pre, mov_pre;
			
	res_pre = Replace( ".resizeto(", ".scrollTo(" );
	mov_pre = Replace( ".moveto(", ".scrollTo(" );

	if(( res_pre >= 0 ) && ( mov_pre >= 0 ))
		Ready = ( res_pre < mov_pre ) ? res_pre : mov_pre;
	else if( res_pre >= 0 )
		Ready = res_pre;
	else if( mov_pre >= 0 )
		Ready = mov_pre;
	else
		Ready = Data.GetSize();
	
	return( true );
}
//---------------------------------------------------------------------------

