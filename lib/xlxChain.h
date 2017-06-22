/**
* xlxChain.h - Xlight Chain Object Library - This library creates the linkedlists in working memory (Rules, Schedule, Scenerio etc)
*
* Created by Baoshi Sun <bs.sun@datatellit.com>
* Copyright (C) 2015-2016 DTIT
* Full contributor list:
*
* Documentation:
* Support Forum:
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* version 2 as published by the Free Software Foundation.
*
*******************************
*
* REVISION HISTORY
* Version 1.0 - Created by Baoshi Sun <bs.sun@datatellit.com>
*
* DESCRIPTION
*
* ToDo:
* 1.
**/

#include "xliCommon.h"
#include "LinkedList.h"

//------------------------------------------------------------------
// Chain Class, inherited from Arduino LinkedList base class
// Keep all member functions inside of this header file
//------------------------------------------------------------------
template <typename T>
class ChainClass : public LinkedList<T>
{
private:
	UC max_chain_length;

public:
	ChainClass(UC max);

	//child functions
	ListNode<T>* search(uint8_t uid);	//returns node pointer, given the uid
	int search_uid(uint8_t uid);		//returns index, given the uid
	bool delete_one_outdated_row();		//deletes the single most outdated row from the chain passed in, returns false if no such row exists
	bool isFull();						//checks if the max chain length has been reached (return true), and if a row can be deleted (return false)

	//accessor functions
	ListNode<T>* getRoot();
	ListNode<T>* getLast();

	//overload all "add" functions to first check if linkedlist length is greater than MAX_TABLE_SIZE
	virtual bool add(int index, T);
	virtual bool add(T);
	virtual bool unshift(T);
};

//------------------------------------------------------------------
// Constructors
//------------------------------------------------------------------
template<typename T>
ChainClass<T>::ChainClass(UC max)
 : LinkedList<T>()
{
	max_chain_length = max;
}

//------------------------------------------------------------------
// Child Functions
//------------------------------------------------------------------
template<typename T>
ListNode<T>* ChainClass<T>::search(uint8_t uid)
{
	ListNode<T> *tmp = LinkedList<T>::root;
	while (tmp != NULL)
	{
		if (tmp->data.uid == uid) {
			return tmp;
		}
		tmp = tmp->next;
	}
	return NULL;
}

template<typename T>
int ChainClass<T>::search_uid(uint8_t uid)
{
	int index = 0;
	ListNode<T> *tmp = LinkedList<T>::root;
	while (tmp != NULL)
	{
		if (tmp->data.uid == uid)
		{
			return index;
		}
		index++;
		tmp = tmp->next;
	}
	return -1;
}

template<typename T>
bool ChainClass<T>::delete_one_outdated_row()
{
	int index = 0;
	ListNode<T> *tmp = LinkedList<T>::root;
	while (tmp != NULL)
	{
		if (tmp->data.flash_flag == SAVED && tmp->data.run_flag == EXECUTED)
		{
			LinkedList<T>::remove(index);
			return true;
		}
		index++;
		tmp = tmp->next;
	}
	return false;
}

template<typename T>
bool ChainClass<T>::isFull()
{
	return (max_chain_length > 0 && LinkedList<T>::size() >= max_chain_length);
}

template<typename T>
bool ChainClass<T>::add(int index, T _t)
{
	if (isFull())
		return false;

	return LinkedList<T>::add(index, _t);
}

//------------------------------------------------------------------
// Accessor Functions
//------------------------------------------------------------------
template<typename T>
ListNode<T>* ChainClass<T>::getRoot()
{
	return LinkedList<T>::root;
}

template<typename T>
ListNode<T>* ChainClass<T>::getLast()
{
	return LinkedList<T>::last;
}

//------------------------------------------------------------------
// Overloaded Functions
//------------------------------------------------------------------
template<typename T>
bool ChainClass<T>::add(T _t)
{
	if (isFull())
		return false;

	return LinkedList<T>::add(_t);
}

template<typename T>
bool ChainClass<T>::unshift(T _t)
{
	if (isFull())
		return false;

	return LinkedList<T>::unshift(_t);
}
