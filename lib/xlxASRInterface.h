//  xlxASRInterface.h - Xlight Automatic Speech Recognition interface via Serial Port

#ifndef xlxASRInterface_h
#define xlxASRInterface_h

#include "xliCommon.h"

#ifndef DISABLE_ASR

class ASRInterfaceClass
{
protected:
  UC m_revCmd;
  UC m_sndCmd;
  UL m_delayCmdTimer;

public:
  ASRInterfaceClass();

  void Init(US _speed = SERIALPORT_SPEED_LOW);
  bool processCommand();
  bool sendCommand(UC _cmd, bool now = false);
  UC getLastReceivedCmd();
  UC getLastSentCmd();

private:
  US m_speed;

  void executeCmd(UC _cmd);
};

//------------------------------------------------------------------
// Function & Class Helper
//------------------------------------------------------------------
extern ASRInterfaceClass theASR;

#endif // DISABLE_ASR

#endif /* xlxASRInterface_h */
