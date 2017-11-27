//  xlxPanel.h - Xlight SmartController Panel Class
#ifndef xlxPanel_h
#define xlxPanel_h

#include "ClickEncoder.h"
#include "ShiftRegister74HC595.h"

class xlPanelClass
{
private:
  ClickEncoder *m_pEncoder;
  ShiftRegister74HC595 *m_pHC595;
  int16_t m_nDimmerValue;
  int16_t m_nCCTValue;
  bool m_stSwitch;
  bool m_bCCTFlag;
  uint32_t m_nCCTick;
  uint32_t m_nLastOpPast;

protected:
  bool SetHC595();
  void CheckHeldTimeout(const uint8_t nHeldDur);

public:
  xlPanelClass();
  virtual ~xlPanelClass();
  void InitPanel();
  void InitEncoder(uint8_t A, uint8_t B, uint8_t buttonPin);
  void InitHC595(uint8_t numberOfShiftRegisters, uint8_t serialDataPin, uint8_t clockPin, uint8_t latchPin);

  bool EncoderAvailable();
  bool HC595Available();
  bool ProcessEncoder();
  bool CheckLEDRing(uint8_t _testno = 0);
  void SetRingPos(uint8_t _pos);
  bool GetRingOnOff();
  void SetRingOnOff(bool _switch);

  int16_t GetDimmerValue();
  void SetDimmerValue(int16_t _value);
  void UpdateDimmerValue(int16_t _value);
  int16_t GetCCTValue(const bool _percent = true);
  void SetCCTValue(int16_t _value);
  void UpdateCCTValue(uint16_t _value);
  uint8_t GetButtonStatus();

  bool GetCCTFlag();
  void SetCCTFlag(bool _flag);
  void ReverseCCTFlag();
};

//------------------------------------------------------------------
// Function & Class Helper
//------------------------------------------------------------------
extern xlPanelClass thePanel;

#endif /* xlxPanel_h */
