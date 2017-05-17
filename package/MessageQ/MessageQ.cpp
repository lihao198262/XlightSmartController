/**
 * MessageQ.cpp - Message queue is used to store messages for process,
 * supports message TTL(time-to-live) and retry times
 *
 * Created by Baoshi Sun <bs.sun@datatellit.com>
 * Copyright (C) 2015-2017 DTIT
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
 * Dependancy
 *
 * DESCRIPTION
 *
 * ToDo:
 *
**/

//#include "xliCommon.h"
#include "MessageQ.h"

////////////////////////////////////////////////////////////////
// Fast Message Queue
////////////////////////////////////////////////////////////////
CFastMessageNode::CFastMessageNode(uint8_t f_iSize)
 : m_iRepeatTimes(0)
{
	m_pData = new uint8_t[f_iSize];
	if( m_pData )
	{
		m_nSize = f_iSize;
		memset(m_pData, 0x00, m_nSize);
	}
	else
		m_nSize = 0;

	m_nLen = 0;
	m_pPrev = NULL;
	m_pNext = NULL;
	m_Tag = 0;
  m_iRepeatTimes = 0;
  m_tickLastRead = 0;
}

CFastMessageNode::~CFastMessageNode()
{
	if( m_pData )
		delete[] m_pData;
}

void CFastMessageNode::WriteMessage(const uint8_t *f_data, uint8_t f_len, uint8_t f_Tag)
{
	uint8_t lv_len = min(f_len, m_nSize);
	if( lv_len > 0 )
		memcpy( m_pData, f_data, lv_len );
	m_nLen = lv_len;
	m_Tag = f_Tag;
  m_iRepeatTimes = 0;
  m_tickLastRead = 0;
}

uint8_t CFastMessageNode::ReadMessage(uint8_t *f_data, uint8_t *f_repeat, uint8_t *f_Tag, uint8_t f_10ms)
{
  uint32_t ticknow = millis();
  if(ticknow - m_tickLastRead <= f_10ms * 10) return 0;

  m_iRepeatTimes++;
  m_tickLastRead = ticknow;
  if( f_repeat ) *f_repeat = m_iRepeatTimes;
  if( f_Tag )	*f_Tag = m_Tag;
  if( m_nLen > 0 )
    memcpy(f_data, m_pData, m_nLen);
	return m_nLen;
}

// return true if messages are identical
bool CFastMessageNode::CompareMessage(const uint8_t *f_data, uint8_t f_len)
{
  if(f_len != m_nLen) return false;
  return(memcmp(f_data, m_pData, m_nLen) == 0);
}

void CFastMessageNode::ClearMessage()
{
  m_nLen = 0;
}

CFastMessageQ::CFastMessageQ(uint8_t f_iMaxLen, uint8_t f_iNodeSize)
	: m_iQLength(0),
	  m_pQHead(NULL),
	  m_pQTail(NULL),
    m_bDupMsg(false),
    m_bLock(false)
{
	// Get maxium length
	m_iMaxQLength = f_iMaxLen;

	// Get node size
	m_iNodeSize = f_iNodeSize;

	// Create queue
	CFastMessageNode *lv_new, *lv_old = NULL;
	for( uint8_t lv_loop = 0; lv_loop < m_iMaxQLength; lv_loop++ )
	{
		lv_new = new CFastMessageNode(f_iNodeSize);
		if( lv_new == NULL )
		{
			m_iMaxQLength = lv_loop;
			break;
		}

		/// Link Nodes
		if( lv_old != NULL )
		{
			lv_old->m_pNext = lv_new;
			lv_new->m_pPrev = lv_old;
		}
		else
			m_pQHead = m_pQTail = lv_new; // First node
		lv_old = lv_new;
	}

	if( m_pQHead )
	{
		m_pQHead->m_pPrev = lv_old;
		lv_old->m_pNext = m_pQHead;
	}
}

CFastMessageQ::~CFastMessageQ()
{
	CFastMessageNode *lv_pNode;
	for( uint8_t lv_loop = 0; lv_loop < m_iMaxQLength; lv_loop++ )
	{
		lv_pNode = m_pQHead;
		m_pQHead = m_pQHead->m_pNext;
		delete(lv_pNode);
	}
}


uint8_t CFastMessageQ::GetMQLength()
{
	return m_iQLength;
}


uint8_t CFastMessageQ::GetMQMaxLength()
{
	return m_iMaxQLength;
}

// Add message at the end of queue
uint8_t CFastMessageQ::AddMessage(const uint8_t *f_data, uint8_t f_len, uint8_t f_Tag)
{
  if( GetLock(20) ) return 0;

  uint8_t lv_retVal = 0;

  // Lock Queue
	LockQueue();

  if( !m_bDupMsg ) {
    // Skip duplicated message
    CFastMessageNode *lv_pNode = m_pQHead;
    while( lv_pNode != NULL && lv_pNode != m_pQTail )
    {
      if( lv_pNode->CompareMessage(f_data, f_len) )
      {
        // Same message
        lv_retVal = m_iQLength;
        break;
      }
      lv_pNode = lv_pNode->m_pNext;
    }
  }

	if( m_iQLength < m_iMaxQLength && lv_retVal == 0 )
	{
		// Set Data
		m_pQTail->WriteMessage(f_data, f_len, f_Tag);
		m_pQTail = m_pQTail->m_pNext;
		m_iQLength++;
		lv_retVal = m_iQLength;
    //SERIAL_LN("Messgae Queue len:%d", m_iQLength);
	}

	// Unlock
	UnlockQueue();

	return lv_retVal;
}

// Read member
CFastMessageNode *CFastMessageQ::GetMessage(CFastMessageNode *pNode)
{
  if( GetLock(10) ) return NULL;

	// Lock Queue
	LockQueue();

  if( pNode == NULL ) pNode = m_pQHead;
  else if( pNode == m_pQTail || m_iQLength == 0 ) pNode = NULL;

	// Unlock
	UnlockQueue();

	return pNode;
}

// Remove member
bool CFastMessageQ::RemoveMessage(CFastMessageNode *pNode)
{
  if( GetLock(10) ) return false;

	bool lv_retVal = false;

	// Lock Queue
	LockQueue();

	if( pNode == NULL ) pNode = m_pQHead;
  if( pNode != m_pQTail && m_iQLength > 0 ) {
    pNode->ClearMessage();
    // Rearrange node chain
    pNode->m_pPrev->m_pNext = pNode->m_pNext;
    pNode->m_pNext->m_pPrev = pNode->m_pPrev;
    pNode->m_pNext = m_pQTail->m_pNext;
    pNode->m_pPrev = m_pQTail;
    m_pQTail->m_pNext->m_pPrev = pNode;
    m_pQTail->m_pNext = pNode;
		m_iQLength--;
  }

	// Unlock
	UnlockQueue();

	return lv_retVal;
}

// Remove all messages from queue
void CFastMessageQ::RemoveAllMessage()
{
	// Lock Queue
	LockQueue();
	m_iQLength = 0;
	m_pQHead = m_pQTail;
	// Unlock
	UnlockQueue();
}

bool CFastMessageQ::GetDuplicateMsg()
{
  return m_bDupMsg;
}

void CFastMessageQ::SetDuplicateMsg(const bool f_sw)
{
  m_bDupMsg = f_sw;
}

bool CFastMessageQ::GetLock(uint8_t f_10ms)
{
  while(f_10ms-- > 0 && m_bLock) {
    delay(10);
  }
  return m_bLock;
}

void CFastMessageQ::LockQueue()
{
	m_bLock = true;
}

void CFastMessageQ::UnlockQueue()
{
	m_bLock = false;
}
