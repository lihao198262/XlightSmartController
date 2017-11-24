/**
 * xlxPanel.cpp - Xlight SmartController Panel Class
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
 * FEATURE DESCRIPTION
 * 1. Click-rotation knob
 * 2. LED brightness indicator
 * 3. RGB status LED
 * 4. Click-knob actions:
 * 4.1 BUTTON_CLICKED: switch the lights on/off
 * 4.2 BUTTON_DOUBLE_CLICKED: switch between BCM (Brightness Control Mode) and CCT control mode temporarily,
 *      if there is no knob spin operation in CCT mode for a few seconds, it will resume to BCM automatically
 * 4.3 BUTTON_HELD / BUTTON_RELEASED, system operations
 *      if >= 30 seconds, then enter DFU mode
 *      if >= 15 seconds, the enter Wi-Fi setup mode
 *      if >= 10 seconds, then reset the controller
 *      if >= 5 seconds, enable base network
 *
**/
#include "xliPinMap.h"
#include "xliNodeConfig.h"
#include "xlxPanel.h"
#include "xlxLogger.h"
#include "xlSmartController.h"
#include "xlxRF24Server.h"
#include "xlxConfig.h"

#define PANEL_NUM_LEDS_RING   12
#define PANEL_NUM_LEDS_CCTIDX 8

// the one and only instance of Panel class
xlPanelClass thePanel;

uint8_t CCWTipLight595 [] = {0x00,0x00,0x01,0x00,0x03,0x00,0x07,0x00,0x0f,0x00,0x1f,0x00,0x3f,0x00,0x7f,0x00,0xff,0x00,0xff,0x01,0xff,0x03,0xff,0x07,0xff,0x0f};
uint8_t CWTipLight595 [] = {0x00,0x00,0x00,0x08,0x00,0x0c,0x00,0x0e,0x00,0x0f,0x80,0x0f,0xc0,0x0f,0xe0,0x0f,0xf0,0x0f,0xf8,0x0f,0xfc,0x0f,0xfe,0x0f,0xff,0x0f};
// LED#: 9, 10, 11, 12, 1, 2, 3, 4, 5
uint8_t CCTIdxLight595 [] = {0x00,0x01, 0x00,0x02, 0x00,0x04, 0x00,0x08, 0x01,0x00, 0x02,0x00, 0x04,0x00, 0x08,0x00, 0x10,0x00};

xlPanelClass::xlPanelClass()
{
  m_pEncoder = NULL;
  m_pHC595 = NULL;
  m_nDimmerValue = 0;
  m_nCCTValue = 0;
  m_bCCTFlag = false;
  m_stSwitch = false;
}

xlPanelClass::~xlPanelClass()
{
  if( m_pEncoder ) {
    delete m_pEncoder;
    m_pEncoder = NULL;
  }

  if( m_pHC595 ) {
    delete m_pHC595;
    m_pHC595 = NULL;
  }
}

void xlPanelClass::InitPanel()
{
  InitEncoder(PIN_KNOB_A_PHASE, PIN_KNOB_B_PHASE, PIN_KNOB_BUTTON);
  InitHC595(2, PIN_MOSI_LED, PIN_SCK_LED, PIN_LATCH_LED);
}

void xlPanelClass::InitEncoder(uint8_t A, uint8_t B, uint8_t buttonPin)
{
  if( !m_pEncoder ) {
    m_pEncoder = new ClickEncoder(A, B, buttonPin, 6);
  }
}

void xlPanelClass::InitHC595(uint8_t numberOfShiftRegisters, uint8_t serialDataPin, uint8_t clockPin, uint8_t latchPin)
{
  if( !m_pHC595 ) {
    // create shift register object (number of shift registers, data pin, clock pin, latch pin)
    m_pHC595 = new ShiftRegister74HC595(numberOfShiftRegisters, serialDataPin, clockPin, latchPin);
  }
}

bool xlPanelClass::EncoderAvailable()
{
  if( !m_pEncoder ) return false;
  m_pEncoder->service();
  return true;
}

bool xlPanelClass::HC595Available()
{
  return(m_pHC595);
}

bool xlPanelClass::ProcessEncoder()
{
  if( !m_pEncoder ) return false;

	// Read dimmer value
  int16_t _dimValue;
  int16_t _delta = m_pEncoder->getValue();
  if( GetCCTFlag() ) {
    _dimValue = GetCCTValue();
    _dimValue += _delta;
  	SetCCTValue(_dimValue);

    // CCT idle timeout: automatically clear CCT flag
    if( _delta > 0 ) {
      m_nCCTick = millis();
    } else if( (millis() - m_nCCTick) / 1000 > RTE_TM_MAX_CCT_IDLE ) {
      // Clear CCT flag
      SetCCTFlag(false);
    }
  } else {
  	_dimValue = GetDimmerValue();
    _dimValue += _delta;
  	SetDimmerValue(_dimValue);
  }

	// Read button input
	ButtonType b = m_pEncoder->getButton();
  if (b != BUTTON_OPEN) {
    switch (b) {
      case BUTTON_PRESSED:
        LOGD(LOGTAG_ACTION, "Button Pressed");
        break;
      case BUTTON_HELD:
        LOGD(LOGTAG_ACTION, "Button Held");
        break;
      case BUTTON_RELEASED:
        LOGD(LOGTAG_ACTION, "Button Released: %d", m_pEncoder->getHeldDuration());
        // Clear CCT flag
        SetCCTFlag(false);
        // Check held duration
        CheckHeldTimeout(m_pEncoder->getHeldDuration());
        break;
      case BUTTON_CLICKED:
        LOGD(LOGTAG_ACTION, "Button Clicked,keyobj=%d",theConfig.GetRelayKeyObj());
        if( theConfig.GetRelayKeyObj() == BTN_OBJ_SCAN_KEY_MAP ) {
          theSys.ToggleAllHardSwitchs();
        } else if( theConfig.GetRelayKeyObj() == BTN_OBJ_LOOP_KEY_MAP ) {
          theSys.ToggleLoopHardSwitch();
        } else {
          theSys.ToggleLampOnOff(CURRENT_DEVICE, CURRENT_SUBDEVICE);
        }
        // Clear CCT flag, but don't need to change HC595, cuz the toggle function will do it
        //SetCCTFlag(false);
        m_bCCTFlag = false;
        break;
      case BUTTON_DOUBLE_CLICKED:
        LOGD(LOGTAG_ACTION, "Button DoubleClicked");
        ReverseCCTFlag();
        //m_pEncoder->setAccelerationEnabled(!m_pEncoder->getAccelerationEnabled());
        //SERIAL_LN("  Acceleration is %s", m_pEncoder->getAccelerationEnabled() ? "enabled" : "disabled");
        break;
    }
  }

}

int16_t xlPanelClass::GetDimmerValue()
{
	return m_nDimmerValue;
}

void xlPanelClass::SetDimmerValue(int16_t _value)
{
	// Restrict range
  _value = constrain(_value, 0, 100);
  if( m_nDimmerValue != _value ) {
		m_nDimmerValue = _value;
    SetHC595();
    // Send Light Percentage message
    theSys.ChangeLampBrightness(CURRENT_DEVICE, _value, CURRENT_SUBDEVICE);
		LOGD(LOGTAG_EVENT, "Dimmer-BR changed to %d", _value);
	}
}

// Update Dimmer value according to confirmation
void xlPanelClass::UpdateDimmerValue(int16_t _value)
{
	// Restrict range
  _value = constrain(_value, 0, 100);
  m_nDimmerValue = _value;
}

int16_t xlPanelClass::GetCCTValue(const bool _percent)
{
	return(_percent ? m_nCCTValue : map(m_nCCTValue, 0, 100, CT_MIN_VALUE, CT_MAX_VALUE));
}

void xlPanelClass::SetCCTValue(int16_t _value)
{
	// Restrict range
  _value = constrain(_value, 0, 100);

	if( m_nCCTValue != _value ) {
		m_nCCTValue = _value;
    SetHC595();
    // Send CCT message
    US cctValue = map(_value, 0, 100, CT_MIN_VALUE, CT_MAX_VALUE);
    theSys.ChangeLampCCT(CURRENT_DEVICE, cctValue);
		LOGD(LOGTAG_EVENT, "Dimmer-CCT changed to %d", cctValue);
	}
}

void xlPanelClass::UpdateCCTValue(uint16_t _value)
{
  UC cct_dimmer = map(_value, CT_MIN_VALUE, CT_MAX_VALUE, 0, 100);
  m_nCCTValue = cct_dimmer;
}

uint8_t xlPanelClass::GetButtonStatus()
{
  if( !m_pEncoder ) return 0;
	return (uint8_t)m_pEncoder->getButton();
}

// Change LED ring
bool xlPanelClass::SetHC595()
{
  //SERIAL_LN("SetHC595 ... ");
  if( !m_pHC595 ) return false;
  if(theConfig.GetDisableLamp()) return false;
  uint8_t pos;
  if( GetCCTFlag() ) {
    // Change dimmer value to LED ring position
    pos = map(m_nCCTValue, 0, 100, 0, PANEL_NUM_LEDS_CCTIDX);
  } else {
    // Change dimmer value to LED ring position
    pos = map(m_nDimmerValue, 0, 100, 0, PANEL_NUM_LEDS_RING);
  }
  SetRingPos(pos);
  return true;
}

void xlPanelClass::SetRingPos(uint8_t _pos)
{
  if( !m_pHC595 ) return;
  //SERIAL_LN("pos:%d, b0:0x%x, b1:0x%x", _pos, CCWTipLight595[_pos * 2], CCWTipLight595[_pos * 2+1]);
  if( GetCCTFlag() ) {
    m_pHC595->setAll(CCTIdxLight595 + _pos * 2);
  } else {
    m_pHC595->setAll(CCWTipLight595 + _pos * 2);
  }
}

bool xlPanelClass::GetRingOnOff()
{
  return m_stSwitch;
}

void xlPanelClass::SetRingOnOff(bool _switch)
{
  //LOGD(LOGTAG_EVENT, "SetRingOnOff to %d", _switch);
  if( !m_pHC595 ) return;
  uint8_t pos;
  if( GetCCTFlag() ) {
    pos = (_switch ? map(m_nCCTValue, 0, 100, 0, PANEL_NUM_LEDS_CCTIDX) : 0);
  } else {
    pos = (_switch ? map(m_nDimmerValue, 0, 100, 0, PANEL_NUM_LEDS_RING) : 0);
  }
  m_stSwitch = _switch;
  if(theConfig.GetDisableLamp())
  {
    pos = theConfig.GetKeyOnNum()*12/MAX_KEY_MAP_ITEMS;
  }
  SetRingPos(pos);
}

bool xlPanelClass::CheckLEDRing(uint8_t _testno)
{
  if( !m_pHC595 ) return false;

  uint8_t i;
  //uint8_t length = m_pHC595->getNumberOfShiftRegisters() * 8;
  switch(_testno) {
  case 0:
    SERIAL("all low...");
    m_pHC595->setAllLow();  // set all pins LOW
    break;
  case 1:
    SERIAL("all high...");
    m_pHC595->setAllHigh();  // set all pins HIGH
    break;
  case 2:
    for (i = 0; i <= PANEL_NUM_LEDS_RING; i++) {
      m_pHC595->set(i, HIGH); // set single pin HIGH
      delay(200);
    }
    break;
  case 3:
    for (i = 0; i <= PANEL_NUM_LEDS_RING; i++) {
      m_pHC595->set(i, LOW); // set single pin LOW
      delay(200);
    }
    break;
  case 4:
    for (i = 0; i <= PANEL_NUM_LEDS_RING; i++) {
      m_pHC595->setAll(CWTipLight595 + i*2);
      delay(200);
    }
    break;
  case 5:
    for (i = 0; i <= PANEL_NUM_LEDS_RING; i++) {
      m_pHC595->setAll(CCWTipLight595 + i*2);
      delay(200);
    }
    break;
  }

  return true;
}

bool xlPanelClass::GetCCTFlag()
{
  return m_bCCTFlag;
}

void xlPanelClass::SetCCTFlag(bool _flag)
{
  if( m_bCCTFlag != _flag ) {
    if( _flag ) m_nCCTick = millis();
    m_bCCTFlag = _flag;
    SetHC595();
  }
}

void xlPanelClass::ReverseCCTFlag()
{
  SetCCTFlag(!m_bCCTFlag);
}

void xlPanelClass::CheckHeldTimeout(const uint8_t nHeldDur)
{
  if( nHeldDur >= RTE_TM_HELD_TO_DFU ) {
    LOGE(LOGTAG_ACTION, "System is about to enter DFU mode");
    System.dfu();
  } else if( nHeldDur >= RTE_TM_HELD_TO_WIFI ) {
    LOGW(LOGTAG_ACTION, "System is about to enter safe mode");
    System.enterSafeMode();
  } else if( nHeldDur >= RTE_TM_HELD_TO_RESET ) {
    LOGW(LOGTAG_ACTION, "System is about to reset");
    theSys.Restart();
  } else if( nHeldDur >= RTE_TM_HELD_TO_BASENW ) {
    theRadio.enableBaseNetwork(true);
    LOGN(LOGTAG_ACTION, "Base network is enabled.");
  }
}
