// Template class definition for linked list data structures
//
//    Copyright (C) 2007-2008  CapeSym, Inc.
//
#pragma once
//#ifndef listsH
//#define listsH
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// The template LIST class defines generic linked list operations.
/////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <class Element> class List {		// template for doubly-linked, ordered list of Elements
public:
	Element *first, *last;					// hidden pointers to head and tail of list
	List() { first = NULL; last = NULL; };	// constructor of empty list
	virtual ~List();						// destructor frees all elements
	List( List<Element> &l );				// copy constructor
	int Number(void);						// return number of items in list
	void Extract( Element *element );		// remove element from list (but don't delete it)
	void Add( Element *element ); 			// add element after last of the list
	void AddFirst( Element *element );		// add element before first of list
	void DeleteFirst( void );				// delete first element in list
	void Insert( Element *insert, Element *after );
	void Reverse( void );					// reverse order of entire list
};

// Methods for a simplified template class for doubly-linked lists of arbitrary Elements

template <class Element>
List<Element>::~List()						// delete the entire list and all elements
{
	Element *e;
	while ( last )
		{
		e = last;
		last = last->prev;
		delete e;							// delete the element at tail
		}
}

template <class Element>
List<Element>::List( List<Element> &l )		// copy constructor
{
	Element *el, *e;
	first = NULL; last = NULL;
											// copy list element by element
	el = l.first;
	while ( el )
		{
		e = new Element( *el );				// NOTE: <class Element> must have a copy constructor also
		this->Add( e );
		el = el->next;
		}
}

template <class Element>
int List<Element>::Number(void)				// count the number of list items
{
	Element *e;
	int count = 0;

	e = first;
	while ( e )
		{
		count++;
		e = e->next;
		}

	return count;
}

template <class Element>
void List<Element>::Extract( Element *element )	// remove element from list and return
{
	if ( element )
		{
		if ( element->prev )					// set pointers around element
			element->prev->next = element->next;
		else
			first = element->next;				// adjust head if no prev

		if ( element->next )
			element->next->prev = element->prev;
		else
			last = element->prev;				// adjust tail if no next

		element->prev = NULL;					// now it's disconnected!
		element->next = NULL;
		}
}

template <class Element>
void List<Element>::Add( Element *element )		// add element to tail of list
{
	if ( element )
		{
		element->prev = last;
		element->next = NULL;
		if ( last )								// if no tail, then no head either
			last->next = element;
		else
			first = element;
		last = element;
		}
}

template <class Element>
void List<Element>::AddFirst( Element *element )	// add element to head of list
{
	if ( element )
		{
		element->next = first;
		element->prev = NULL;
		if ( first )							// if no head, then no tail either
			first->prev = element;
		else
			last = element;
		first = element;
		}
}

template <class Element>
void List<Element>::DeleteFirst( void )			// remove first element from list and delete it
{
	Element *e;
	if ( first )
		{
		e = first;								// remember first element
		first = first->next;					// move head toward tail
		if ( first ) first->prev = NULL;		// if still have elements, clear prev ptr
		else last = NULL;						// otherwise clear tail ptr
		delete e;
		}
}

												// add *insert to list at position *after
template <class Element>
void List<Element>::Insert( Element *insert, Element *after )
{
	if ( insert )
	  if ( after )								// add to middle of list or tail
		{
		insert->next = after->next;
		insert->prev = after;
		if ( insert->next ) insert->next->prev = insert;
		else last = insert;
		after->next = insert;
		}
	  else AddFirst( insert );					// add at head of list
}

template <class Element>
void List<Element>::Reverse( void )				// reverse the order of entire list
{
	Element *tmp;

	first = last;								// head points to tail
	tmp = last;
	while ( tmp )
		{
		last = tmp;								// move tail if not NULL
		tmp = last->prev;						// swap prev, next pointers
		last->prev = last->next;
		last->next = tmp;
		}
}
//#endif
