//  xlxBLEInterface.h - Xlight Automatic Speech Recognition interface via Serial Port

#ifndef xlxBLEInterface_h
#define xlxBLEInterface_h

#include "xliCommon.h"

class BLEInterfaceClass
{
public:
  BLEInterfaceClass();

  void Init(UC _stpin, UC _enpin);
  void config();
  BOOL isGood();
  UL getSpeed();
  BOOL getState();

  String getName();
  BOOL setName(String _name);
  String getPin();
  BOOL setPin(String _pin);

  BOOL isATMode();
  void setATMode(BOOL on);
  void processCommand();
  BOOL exectueCommand(char *inputString);
  BOOL sendCommand(String _cmd);
  BOOL sendNotification(const UC _id, String _data);

private:
  UC m_pin_state;
  UC m_pin_en;
  UL m_speed;
  US m_token;
  BOOL m_bGood;
  BOOL m_bATMode;
  String m_name;
  String m_pin;
  String m_lastCmd;
};

//------------------------------------------------------------------
// Function & Class Helper
//------------------------------------------------------------------
extern BLEInterfaceClass theBLE;

#endif /* xlxBLEInterface_h */
