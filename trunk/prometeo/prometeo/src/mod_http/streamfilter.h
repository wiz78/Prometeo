/***************************************************************************
                               streamfilter.h
                             -------------------
    revision             : $Id: streamfilter.h,v 1.1 2003-04-06 10:57:37 tellini Exp $
    copyright            : (C) 2003 by Simone Tellini
    email                : tellini@users.sourceforge.net

    description          : base class to filter a stream of data
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef STREAMFILTER_H
#define STREAMFILTER_H

#include "buffer.h"

class StreamFilter
{
public:
					StreamFilter();

	bool			Filter( const char *data, int len );
	void			Clean( void );

	int				GetSize( bool eof );
	const char		*GetData( void )      { return( Data.GetData() ); }

protected:
	Buffer			Data;
	int				Ready; // how many bytes can be returned

	virtual bool	Process( void ) = 0;

	// look whether the available data has a prefix of str at the
	// end (if we want to substitute str we can keep some data for
	// later processing and return the rest as Ready)
	int				FindPrefix( const char *str, int len );

	// returns the index of a str prefix, if found, or -1
	// search is assumed to be lowercase
	int				Replace( const char *search, const char *rep );
};

#endif
