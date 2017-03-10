#include "DataQueue.h"

CManulSync::CManulSync()
{
	//m_lock = CreateMutex (NULL, false, NULL);
	m_lock = false;
}

/*
CManulSync::~CManulSync()
{
	if (m_lock != NULL)
	{
		CloseHandle (m_lock);
		m_lock = NULL;
	}
}
*/

bool CManulSync::Enter(UL ms)
{
	//WaitForSingleObject (m_sync, INFINITE);
	UL start = millis();
	while( millis() - start  <= ms || ms == 0) {
		if(!m_lock) break;
	}

	return !m_lock;
}

void CManulSync::Leave()
{
	//ReleaseMutex (m_sync);
	m_lock = false;
}

CDataQueue::CDataQueue()
{
	m_pBuffer = NULL;
	m_length = 0;
	m_maxlen = 0;
	m_pRead = 0;
	m_pWrite = 0;
}

CDataQueue::CDataQueue(US f_maxlen)
{
	CDataQueue();
	CreateDataBuffer(f_maxlen);
}

CDataQueue::~CDataQueue()
{
	ClearBuffer();
}

void CDataQueue::CreateDataBuffer(US f_nlen)
{
	if( f_nlen > 0 )
	{
		m_sync.Enter();
		if( m_pBuffer )
			delete[] m_pBuffer;
		m_pBuffer = new UC[f_nlen];
		m_length = 0;
		m_maxlen = f_nlen;
		m_pRead = 0;
		m_pWrite = 0;
		m_sync.Leave();
	}
}

US CDataQueue::GetMaxLength()
{
	return m_maxlen;
}

void CDataQueue::SetMaxLength(US f_maxlen)
{
	CreateDataBuffer(f_maxlen);
}

US CDataQueue::Append(const UC* data, US len)
{
	if (data == NULL || len <= 0 ) return 0;

	US lv_retval;

	m_sync.Enter();
	if( m_length + len > m_maxlen )
		lv_retval = -1;
	else
	{
		if( m_pWrite + len > m_maxlen ) {
			US lv_first = m_maxlen - m_pWrite;
			US lv_rest = len - lv_first;
			memcpy(m_pBuffer + m_pWrite, data, lv_first);
			memcpy(m_pBuffer, data + lv_first, lv_rest);
			m_pWrite = lv_rest;
		} else {
			memcpy(m_pBuffer + m_pWrite, data, len);
			m_pWrite += len;
		}

		m_length += len;
		lv_retval = len;
	}
	m_sync.Leave();

	return lv_retval;
}

US CDataQueue::Remove(US f_first, UC* data)
{
	US copylen;

	m_sync.Enter();

	copylen = ( f_first > 0 ? min(f_first, m_length) : m_length );
	if( copylen > 0 )
	{
		if( m_pRead + copylen > m_maxlen )
		{
			US lv_first = m_maxlen - m_pRead;
			US lv_rest = copylen - lv_first;
			memcpy(data, m_pBuffer + m_pRead, lv_first);
			memcpy(data + lv_first, m_pBuffer, lv_rest);
			m_pRead = lv_rest;
		} else {
			memcpy(data, m_pBuffer + m_pRead, copylen);
			m_pRead += copylen;
		}
		m_length -= copylen;
	}
	m_sync.Leave();

	return copylen;
}

US CDataQueue::Length()
{
	US length;

	m_sync.Enter();
	length = m_length;
	m_sync.Leave();

	return length;
}

void CDataQueue::ClearBuffer()
{
	m_sync.Enter();
	if( m_pBuffer )
	{
		delete[] m_pBuffer;
		m_pBuffer = NULL;
	}
	m_length = 0;
	m_maxlen = 0;
	m_pRead = 0;
	m_pWrite = 0;
	m_sync.Leave();
}

UC *CDataQueue::GetBuffer()
{
	return m_pBuffer;
}

void CDataQueue::LockBuffer()
{
	m_sync.Enter();
}

void CDataQueue::UnLockBuffer()
{
	m_sync.Leave();
}
