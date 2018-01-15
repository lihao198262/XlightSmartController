//  MessageQ.h - Message queue is used to store messages for process

#ifndef DTIT_FASTMESSAGEQ_INCLUDED_
#define DTIT_FASTMESSAGEQ_INCLUDED_

#include "application.h"

class CFastMessageNode
{
public:
  CFastMessageNode(uint8_t f_iSize);
  ~CFastMessageNode();

  CFastMessageNode *m_pNext;
  CFastMessageNode *m_pPrev;
  uint8_t m_Tag;
  uint32_t m_iFlag;           // Message flag

  void WriteMessage(const uint8_t *f_data, uint8_t f_len, uint8_t f_Tag = 0,  uint32_t f_flag = 0);
  uint8_t ReadMessage(uint8_t *f_data, uint8_t *f_repeat, uint8_t *f_Tag = NULL, uint32_t *f_flag = NULL, uint8_t f_10ms = 0);
  uint8_t CompareMessage(const uint8_t *f_data, uint8_t f_len, uint32_t f_flag = 0);
  void ClearMessage();

private:
  uint8_t *m_pData;					  // Message Data
  uint8_t m_nLen;							// Message Length
  uint8_t m_nSize;						// Buffer size
  uint8_t m_iRepeatTimes;
  uint32_t m_tickLastRead;
};

class CFastMessageQ
{
public:
	void RemoveAllMessage();
	bool RemoveMessage(CFastMessageNode *pNode = NULL);
	CFastMessageNode *GetMessage(CFastMessageNode *pNode = NULL);
	uint8_t AddMessage(const uint8_t *f_data, uint8_t f_len, uint8_t f_Tag = 0,  uint32_t f_flag = 0);
	uint8_t GetMQLength();
	uint8_t GetMQMaxLength();

	CFastMessageQ(uint8_t f_iMaxLen = 8, uint8_t f_iNodeSize = 32);
	virtual ~CFastMessageQ();

  uint8_t GetRepeatInterval();

	void LockQueue();
	void UnlockQueue();
  bool GetLock(uint8_t f_10ms = 0);

  bool GetDuplicateMsg();
  void SetDuplicateMsg(const bool f_sw = true);

protected:
	CFastMessageNode *m_pQHead;
	CFastMessageNode *m_pQTail;
	uint8_t m_iQLength;
	uint8_t m_iMaxQLength;
	uint8_t m_iNodeSize;

private:
	bool m_bLock;
  bool m_bDupMsg;
};

#endif
