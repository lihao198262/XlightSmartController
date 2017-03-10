#ifndef PCS_DATAQUEUE_INCLUDED_
#define PCS_DATAQUEUE_INCLUDED_

#include "application.h"
#include "xliCommon.h"

//	little class to make code more readable
class CManulSync;
class CManulSync
{
protected:
	bool		m_lock;

public:
	CManulSync();
	//~CManulSync();

	CManulSync(CManulSync& s);
	CManulSync& operator= (CManulSync& s);

	bool Enter(UL ms=0);
	void Leave();
};

class CDataQueue
{
public:
	CDataQueue();
	CDataQueue(US f_maxlen);
	~CDataQueue();

	US GetMaxLength();
	void SetMaxLength(US f_maxlen);
	US Append(const UC* data, US len);
	US Remove(US f_first = 0, UC* data = NULL);
	US Length();
	void ClearBuffer();
	UC *GetBuffer();
	void LockBuffer();
	void UnLockBuffer();

protected:
	UC		*m_pBuffer;
	CManulSync	m_sync;
	US		m_length;
	US		m_maxlen;

	void		CreateDataBuffer(US f_nlen);

private:
	US		m_pRead;
	US		m_pWrite;
};

#endif // PCS_DATAQUEUE_INCLUDED_

///////////////////////////////////////////////////////////////////////////////
// End of file
///////////////////////////////////////////////////////////////////////////////
