/***************************************************************************
                               popupblocker.h
                             -------------------
    revision             : $Id: popupblocker.h,v 1.1 2003-04-06 10:57:37 tellini Exp $
    copyright            : (C) 2003 by Simone Tellini
    email                : tellini@users.sourceforge.net

    description          : blocks those annoying popups, yeah!
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef POPUPBLOCKER_H
#define POPUPBLOCKER_H

#include "streamfilter.h"

class PopupBlocker : public StreamFilter
{
public:
		
protected:
	virtual bool	Process( void );
};

#endif
