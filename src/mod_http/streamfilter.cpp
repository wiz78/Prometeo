/***************************************************************************
                              streamfilter.cpp
                             -------------------
    revision             : $Id: streamfilter.cpp,v 1.1 2003-04-06 10:57:37 tellini Exp $
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

#include "main.h"

using namespace std;

#include <ctype.h>
#include <string>

#include "streamfilter.h"

//---------------------------------------------------------------------------
StreamFilter::StreamFilter()
{
	Ready = 0;
}
//---------------------------------------------------------------------------
bool StreamFilter::Filter( const char *data, int len )
{
	bool ret = true;

	if( len > 0 ) {
		
		Data.Append( data, len );

		ret = Process();
	}		
	
	return( ret );
}
//---------------------------------------------------------------------------
void StreamFilter::Clean( void )
{
	if( Ready > 0 ) {
		int left = Data.GetSize() - Ready;

		if( left > 0 ) {

			memcpy( Data.GetData(), Data.GetData() + Ready, left );
			Data.Resize( left );
			
		} else
			Data.Clear();

		Ready = 0;
	}
}
//---------------------------------------------------------------------------
int StreamFilter::FindPrefix( const char *str, int len )
{
	int ret = -1;

	if( Data.GetSize() ) {
		int start, left;
		
		start = Data.GetSize() - len + 1;
			
		if( start < 0 )
			start = 0;

		left = Data.GetSize() - start;

		while(( ret < 0 ) && ( left > 0 )) {
			const char *p = (char *)Data.GetData() + start;
			int			i = 0;
			
			while(( i++ < left ) && ( tolower( p[ i ] ) == tolower( str[ i ])));

			// all the available chars matched, prefix found
			if( i >= left )
				ret = p - Data.GetData();

			left--;
		}
	}
	
	return( ret );
}
//---------------------------------------------------------------------------
int StreamFilter::Replace( const char *search, const char *rep )
{
	char	*tmp, *start;
	int		slen, rlen, off = 0;
	string	work;
	bool	changed = false;

	slen = strlen( search );
	rlen = strlen( rep );
	tmp  = new char[ Data.GetSize() + 1 ];
	
	memcpy( tmp, Data.GetData(), Data.GetSize() );

	tmp[ Data.GetSize() ] = '\0';

	work = tmp;

	for( char *ptr = tmp; *ptr; ptr++ )
		*ptr = tolower( *ptr );

	start = tmp;

	while( char *str = strstr( start, search )) {
		
		work.replace( str - tmp + off, slen, rep );

		start    = str + slen;
		off     += rlen - slen;
		changed  = true;
	}

	delete[] tmp;
	
	if( changed )
		Data.SetContent( work.c_str(), work.length() );
	
	return( FindPrefix( search, slen ));
}
//---------------------------------------------------------------------------
int StreamFilter::GetSize( bool eof )
{
	if( eof )
		Ready = Data.GetSize();

	return( Ready );
}
//---------------------------------------------------------------------------

