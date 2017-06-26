/**
 * xlxASRInterface.cpp - Xlight Automatic Speech Recognition interface via Serial Port
 * This module works with the 3rd party ASR module and takes voice commands via serial port.
 * The module also supports sending playback commands to ASR module.
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
 * 1. Tested on WTK6900B01
 * 2.
 *
 * ToDo:
 * 1.
 *
**/

#include "xlxASRInterface.h"
#include "xlSmartController.h"
#include "xlxConfig.h"
#include "xlxLogger.h"
#include "ParticleSoftSerial.h"
#include "xliPinMap.h"

#define ASR_TXCMD_PREFIX          0xbb
#define ASR_RXCMD_PREFIX          0xaa
#define ASR_CMD_LEN               3

#define BTN_STEP_SHORT_BR         10
#define BTN_STEP_SHORT_CCT        300
#define BTN_STEP_LONG_BR          25
#define BTN_STEP_LONG_CCT         800

//------------------------------------------------------------------
// the one and only instance of ASRInterfaceClass
ASRInterfaceClass theASR;

#define PROTOCOL                SERIAL_8N1

#ifndef DISABLE_ASR

#ifndef ASRPort
#define ASRPort                 SoftSer
#endif

// RX must be interrupt enabled (on Photon/Electron D0/A5 are not)
#define PSS_RX                  A6      // D1
#define PSS_TX                  A7      // P1S3
ParticleSoftSerial SoftSer(PSS_RX, PSS_TX);

#endif

//------------------------------------------------------------------
// Xlight SerialConsole Class
//------------------------------------------------------------------
ASRInterfaceClass::ASRInterfaceClass()
{
  m_speed = SERIALPORT_SPEED_LOW; // 9600
  m_revCmd = 0;
  m_sndCmd = 0;
  m_delayCmdTimer = 0;
}

void ASRInterfaceClass::Init(US _speed)
{
#ifndef DISABLE_ASR
  // Open ASR Port
  m_speed = _speed;
  ASRPort.begin(_speed, PROTOCOL);
#endif
}

bool ASRInterfaceClass::processCommand()
{
  static bool bWaitCmd = false;
  static bool bWaitCheck = false;
  bool rc = false;
  int incomingByte;

#ifndef DISABLE_ASR

  // Send command
  if( m_sndCmd > 0 ) {
    sendCommand(m_sndCmd, true);
    SERIAL("asr cmd %d rc=", m_sndCmd);
    m_sndCmd = 0;
  }

  while( ASRPort.available() > 0 ) {
    incomingByte = ASRPort.read();
    if( incomingByte < 0 ) break;
    SERIAL("0x%02x ", incomingByte);
    if( (UC)incomingByte == ASR_RXCMD_PREFIX ) {
      bWaitCmd = true;
      rc = false;   // start to receive new command
    } else if( bWaitCmd ) {
      m_revCmd = (UC)incomingByte;
      bWaitCmd = false;
      bWaitCheck = true;
    } else if( bWaitCheck ) {
      if( (UC)incomingByte == (UC)~m_revCmd ) {
        // Got a valid command
        /// Notes: delay to execute after voice playing due to lack of current while playing
        m_delayCmdTimer = (theConfig.IsSpeakerEnabled() ? 30 : 2);   // * RTE_DELAY_SELFCHECK ms
        //executeCmd(m_revCmd);
        rc = true;
      }
      bWaitCheck = false;
    }
  }

  // Delay Execution
  if( m_delayCmdTimer > 0 ) {
    if( --m_delayCmdTimer == 0 ) {
      //ASRPort.end();
      executeCmd(m_revCmd);
      //ASRPort.begin(m_speed, PROTOCOL);
    }
  }

#endif

  return rc;
}

bool ASRInterfaceClass::sendCommand(UC _cmd, bool now)
{
#ifndef DISABLE_ASR

  if( now ) {
    UC buf[ASR_CMD_LEN];
    buf[0] = ASR_TXCMD_PREFIX;
    buf[1] = _cmd;
    buf[2] = ~_cmd;
    //UC _len = ASRPort.write(buf, ASR_CMD_LEN);
    //if( _len < ASR_CMD_LEN ) return false;
    //SERIAL_LN("len: %d - 0x%02x 0x%02x 0x%02x", _len, buf[0], buf[1], buf[2]);
    if( ASRPort.write(buf, ASR_CMD_LEN) < ASR_CMD_LEN ) return false;
  } else {
    m_sndCmd = _cmd;
  }

#endif

  return true;
}

UC ASRInterfaceClass::getLastReceivedCmd()
{
  return m_revCmd;
}

UC ASRInterfaceClass::getLastSentCmd()
{
  return m_sndCmd;
}

void ASRInterfaceClass::executeCmd(UC _cmd)
{
#ifndef DISABLE_ASR

  LOGD(LOGTAG_MSG, "execute ASR cmd: 0x%x", _cmd);
  UC _snt, _br;
  US _cct;

  _snt = theConfig.GetASR_SNT(_cmd);
  if( _snt > 0 && _snt < 255 ) {
    theSys.ChangeLampScenario(CURRENT_DEVICE, _snt);
  } else {
    switch( _cmd ) {
    case 0x01:    // lights on
      theSys.DevSoftSwitch(true, CURRENT_DEVICE);
      break;

    case 0x02:    // lights off
      theSys.DevSoftSwitch(false, CURRENT_DEVICE);
      break;

    case 0x03:    // Brightness++
      _br = theSys.GetDevBrightness(CURRENT_DEVICE);
      _br += BTN_STEP_SHORT_BR;
      theSys.ChangeLampBrightness(CURRENT_DEVICE, _br, CURRENT_SUBDEVICE);
      break;

    case 0x04:    // Brightness--
      _br = theSys.GetDevBrightness(CURRENT_DEVICE);
      _br -= BTN_STEP_SHORT_BR;
      theSys.ChangeLampBrightness(CURRENT_DEVICE, _br, CURRENT_SUBDEVICE);
      break;

    case 0x05:    // CCT++
      _cct = theSys.GetDevCCT(CURRENT_DEVICE);
      _cct += BTN_STEP_SHORT_CCT;
      theSys.ChangeLampCCT(CURRENT_DEVICE, _br);
      break;

    case 0x06:    // CCT--
      _cct = theSys.GetDevCCT(CURRENT_DEVICE);
      _cct -= BTN_STEP_SHORT_CCT;
      theSys.ChangeLampCCT(CURRENT_DEVICE, _br);
      break;
    }
  }

#endif
}
