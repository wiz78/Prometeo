/***************************************************************************
                               linkedlist.cpp
                             -------------------
	revision             : $Id: linkedlist.cpp,v 1.2 2002-10-22 14:31:27 tellini Exp $
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

#include <stdlib.h>

#include "linkedlist.h"

//---------------------------------------------------------------------------
LinkedList::LinkedList()
{
	Succ = this;
	Pred = this;
}
//---------------------------------------------------------------------------
LinkedList::~LinkedList()
{
	while( !IsEmpty() ) {
		LinkedListNode	*node = Succ;

		node->Unlink();	// this may change Succ/Pred

		delete node;
	}
}
//---------------------------------------------------------------------------
void LinkedList::AddHead( LinkedListNode *node )
{
	node->Unlink();
		
	Succ->SetPred( node );
	node->SetPred( this );
	node->SetSucc( Succ );
	Succ = node;

	node->SetInList();
}
//---------------------------------------------------------------------------
void LinkedList::AddTail( LinkedListNode *node )
{
	node->Unlink();
		
	Pred->SetSucc( node );
	node->SetSucc( this );
	node->SetPred( Pred );
	Pred = node;

	node->SetInList();
}
//---------------------------------------------------------------------------
LinkedListNode *LinkedList::RemHead( void )
{
	LinkedListNode *node = NULL;

	if( !IsEmpty() ) {

		node = GetHead();

		node->Unlink();
	}

	return( node );
}
//---------------------------------------------------------------------------
LinkedListNode *LinkedList::RemTail( void )
{
	LinkedListNode *node = NULL;

	if( !IsEmpty() ) {

		node = GetTail();

		node->Unlink();
	}

	return( node );
}
//---------------------------------------------------------------------------
unsigned int LinkedList::Count( void )
{
	LinkedListNode	*node = GetHead();
	unsigned int	num = 0;

	while( IsNode( node )) {
		node = node->GetSucc();
		num++;
	}

	return( num );
}
//---------------------------------------------------------------------------
