/***************************************************************************
                               resizeblocker.h
                             -------------------
    revision             : $Id: resizeblocker.h,v 1.1 2003-04-06 10:57:37 tellini Exp $
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

#ifndef RESIZEBLOCKER_H
#define RESIZEBLOCKER_H

#include "streamfilter.h"

class ResizeBlocker : public StreamFilter
{
public:
		
protected:
	virtual bool	Process( void );
};

#endif
