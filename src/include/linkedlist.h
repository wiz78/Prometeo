/***************************************************************************
                                linkedlist.h
                             -------------------
	revision             : $Id: linkedlist.h,v 1.1.1.1 2002-10-10 09:59:11 tellini Exp $
    copyright            : (C) 2002 by Simone Tellini
    email                : tellini@users.sourceforge.net

	description          : a simple doubly-linked list class
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef PROM_LINKEDLIST_H
#define PROM_LINKEDLIST_H

class LinkedListNode
{
public:
					LinkedListNode()
					{
						Succ = Pred = NULL;
						InList      = false;
					}

					~LinkedListNode()
					{
						Unlink();
					}

	void			Unlink( void )
					{
						if( InList ) {
							Succ->Pred = Pred;
							Pred->Succ = Succ;
							InList     = false;
						}
					}

	LinkedListNode	*GetSucc( void ) { return( Succ ); }
	LinkedListNode	*GetPred( void ) { return( Pred ); }

	void			SetSucc( LinkedListNode *node ) { Succ = node; }
	void			SetPred( LinkedListNode *node ) { Pred = node; }

	void			SetInList( void ) { InList = true; }
	bool			IsInList( void ) { return( InList ); }

protected:
	LinkedListNode	*Succ;
	LinkedListNode	*Pred;
	bool			InList;
};

class LinkedList : private LinkedListNode
{
public:
					LinkedList();
					~LinkedList();

	void			AddHead( LinkedListNode *node );
	void			AddTail( LinkedListNode *node );

	LinkedListNode	*RemHead( void );
	LinkedListNode	*RemTail( void );

	bool			IsEmpty( void ) { return( Succ == this ); }
	bool			IsNode( LinkedListNode *node ) { return( node && ( node != this )); }

	LinkedListNode	*GetHead( void ) { return( Succ ); }
	LinkedListNode	*GetTail( void ) { return( Pred ); }

	unsigned int	Count( void );
};

#endif
