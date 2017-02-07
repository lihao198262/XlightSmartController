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
#include "ParticleSoftSerial.h"
#include "xliPinMap.h"

#define ASR_TXCMD_PREFIX          0xbb
#define ASR_RXCMD_PREFIX          0xaa
#define ASR_CMD_LEN               3

//------------------------------------------------------------------
// the one and only instance of ASRInterfaceClass
ASRInterfaceClass theASR;

#define PROTOCOL                SERIAL_8N1

#ifndef ASRPort
#define ASRPort                 SoftSer
#endif

// RX must be interrupt enabled (on Photon/Electron D0/A5 are not)
#define PSS_RX                  D1
#define PSS_TX                  P1S3
ParticleSoftSerial SoftSer(PSS_RX, PSS_TX);

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
  // Open ASR Port
  m_speed = _speed;
  ASRPort.begin(_speed, PROTOCOL);
}

bool ASRInterfaceClass::processCommand()
{
  static bool bWaitCmd = false;
  static bool bWaitCheck = false;
  bool rc = false;
  int incomingByte;

  while( ASRPort.available() > 0 ) {
    incomingByte = ASRPort.read();
    if( incomingByte < 0 ) break;
    SERIAL("0x%x", incomingByte);
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
      ASRPort.end();
      executeCmd(m_revCmd);
      ASRPort.begin(m_speed);
    }
  }

  return rc;
}

bool ASRInterfaceClass::sendCommand(UC _cmd)
{
  UC buf[ASR_CMD_LEN];
  buf[0] = ASR_TXCMD_PREFIX;
  buf[1] = _cmd;
  buf[2] = ~_cmd;
  if( ASRPort.write(buf, ASR_CMD_LEN) < ASR_CMD_LEN ) return false;
  m_sndCmd = _cmd;
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
  SERIAL_LN("\n\rexecute ASR cmd: 0x%x", _cmd);
  UC _br;
  US _cct;

  switch( _cmd ) {
  /*
  case 0x01:    // Brightness++
    _br = theConfig.GetDevBrightness();
    _br += BTN_STEP_SHORT_BR;
    theSys.ChangeLampBrightness(NODEID_MAINDEVICE, _br);
    break;

  case 0x02:    // Brightness--
    _br = theConfig.GetDevBrightness();
    _br -= BTN_STEP_SHORT_BR;
    theSys.ChangeLampBrightness(NODEID_MAINDEVICE, _br);
    break;

  case 0x03:    // CCT++
    _cct = theConfig.GetDevCCT();
    _cct += BTN_STEP_SHORT_CCT;
    theSys.ChangeLampCCT(NODEID_MAINDEVICE, _br);
    break;

  case 0x04:    // CCT--
    _cct = theConfig.GetDevCCT();
    _cct -= BTN_STEP_SHORT_CCT;
    theSys.ChangeLampCCT(NODEID_MAINDEVICE, _br);
    break;

  case 0x05:    // Scenario
    _br = 25;
    _cct = 3000;
    theSys.ChangeBR_CCT(NODEID_MAINDEVICE, _br, _cct);
    break;

  case 0x06:    // Scenario
    _br = 85;
    _cct = 5000;
    theSys.ChangeBR_CCT(_br, _cct, NODEID_MAINDEVICE);
    break;
    */

  case 0x07:    // lights on
    theSys.DevSoftSwitch(true, NODEID_MAINDEVICE);
    break;

  case 0x08:    // lights off
    theSys.DevSoftSwitch(false, NODEID_MAINDEVICE);
    break;
  }
}
