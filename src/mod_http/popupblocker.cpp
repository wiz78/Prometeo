/***************************************************************************
                              popupblocker.cpp
                             -------------------
    revision             : $Id: popupblocker.cpp,v 1.2 2003-04-07 13:44:30 tellini Exp $
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

#include "popupblocker.h"

//---------------------------------------------------------------------------
bool PopupBlocker::Process( void )
{
	int head_pre, open_pre;

	head_pre = Replace( "<head>",
						"<head>\n"
						"<script language=\"JavaScript\"><!--\n"
						"    function _prometeo_window_open()\n"
						"    {\n"
						"        if(( event.type == \"click\" ) ||\n"
						"           ( event.type == \"dblclick\" ) ||\n"
						"           ( event.type == \"mousedown\" ) ||\n"
						"           ( event.type == \"mouseup\" ) ||\n"
						"           ( event.type == \"keyup\" ) ||\n"
						"           ( event.type == \"keydown\" ) ||\n"
						"           ( event.type == \"keypress\" ) ||\n"
						"           ( event.type == \"error\" ) ||\n"
						"           ( event.type == \"submit\" )) {\n"
						"               func = \"window.\" + \"open(\";\n"
						"               for( i = 0; i < arguments.length; i++ ) {\n"
						"                  if( i > 0 )\n"
						"                     func += \",\";\n"
						"                  if( typeof( arguments[ i ] ) == \"string\" )\n"
						"                     func += \"\\\"\" + arguments[ i ] + \"\\\"\";\n"
						"                  else\n"
						"                     func += arguments[ i ];\n"
						"               }\n"
						"               return( eval( func + \");\" ));\n"
						"        }\n"
						"    }\n"
						"// --></script>\n" );

	open_pre = Replace( "window.open(", "_prometeo_window_open(" );

	if(( head_pre >= 0 ) && ( open_pre >= 0 ))
		Ready = ( head_pre < open_pre ) ? head_pre : open_pre;
	else if( head_pre >= 0 )
		Ready = head_pre;
	else if( open_pre >= 0 )
		Ready = open_pre;
	else
		Ready = Data.GetSize();
		
	return( true );
}
//---------------------------------------------------------------------------

