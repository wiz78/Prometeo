/***************************************************************************
                              popupblocker.cpp
                             -------------------
    revision             : $Id: popupblocker.cpp,v 1.3 2003-04-08 17:44:41 tellini Exp $
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
	int head_pre, open_pre, html_pre;

	head_pre = Replace( "<head>",
						"<head>\n"
						"<script language=\"JavaScript\"><!--\n"
						"    var _prometeo_loaded = false;\n"
						"    function _prometeo_window_open()\n"
						"    {\n"
						"        do_it = ( event == null ) && _prometeo_loaded;\n\n"
						"        if( !do_it && ( event != null ))\n"
						"           do_it = ( event.type == \"click\" ) ||\n"
						"                   ( event.type == \"dblclick\" ) ||\n"
						"                   ( event.type == \"mousedown\" ) ||\n"
						"                   ( event.type == \"mouseup\" ) ||\n"
						"                   ( event.type == \"keyup\" ) ||\n"
						"                   ( event.type == \"keydown\" ) ||\n"
						"                   ( event.type == \"keypress\" ) ||\n"
						"                   ( event.type == \"error\" ) ||\n"
						"                   ( event.type == \"submit\" );\n\n"
						"        if( do_it ) {\n"
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
	
	html_pre = Replace( "</html>",
						"<script language=\"JavaScript\"><!--\n"
						" _prometeo_loaded = true;\n"
						"//--></script></html>" );

	if(( head_pre >= 0 ) && ( open_pre >= 0 ))
		Ready = ( head_pre < open_pre ) ? head_pre : open_pre;
	else if( head_pre >= 0 )
		Ready = head_pre;
	else if( open_pre >= 0 )
		Ready = open_pre;
	else
		Ready = Data.GetSize();

	if(( html_pre >= 0 ) && ( html_pre < Ready ))
		Ready = html_pre;
		
	return( true );
}
//---------------------------------------------------------------------------

