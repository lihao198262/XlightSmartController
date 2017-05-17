//  xlxRF24Server.h - Xlight RF2.4 server library

#ifndef xlxRF24Server_h
#define xlxRF24Server_h

#include "DataQueue.h"
#include "MessageQ.h"
#include "MyTransportNRF24.h"

// RF24 Server class
class RF24ServerClass : public MyTransportNRF24, public CDataQueue, public CFastMessageQ
{
public:
  RF24ServerClass(uint8_t ce=RF24_CE_PIN, uint8_t cs=RF24_CS_PIN, uint8_t paLevel=RF24_PA_LEVEL_GW);

  bool ServerBegin();
  uint64_t GetNetworkID(bool _full = false);
  void SetRole_Gateway();
  bool ChangeNodeID(const uint8_t bNodeID);
  bool ProcessSend(const UC _node, const UC _msgID, String &strPayl, MyMessage &my_msg, const UC _replyTo);
  bool ProcessSend(String &strMsg, MyMessage &my_msg, const UC _replyTo = 0);
  bool ProcessSend(String &strMsg, const UC _replyTo = 0); //overloaded
  bool ProcessSend(MyMessage *pMsg = NULL);
  bool SendNodeConfig(UC _node, UC _ncf, unsigned int _value);

  bool ProcessMQ();
  bool ProcessSendMQ();
  bool ProcessReceiveMQ();

  bool PeekMessage();

  unsigned long _times;
  unsigned long _succ;
  unsigned long _received;

private:
  void ConvertRepeatMsg(MyMessage *pMsg);
};

//------------------------------------------------------------------
// Function & Class Helper
//------------------------------------------------------------------
extern RF24ServerClass theRadio;

#endif /* xlxRF24Server_h */
