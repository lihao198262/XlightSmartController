/**
 * xlxBLEInterface.cpp - Xlight Bluetooth interface via Serial Port
 * This module works with Bluetooth module and exchange data via serial port.
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
 * 1. Tested on HC-06
 * 2.
 *
 * ToDo:
 * 1.
 *
**/

#include "xlxBLEInterface.h"
#include "xlSmartController.h"
#include "xlxConfig.h"
#include "xlxLogger.h"
#include "MyParserSerial.h"
#include "xlxRF24Server.h"
#include "xlxSerialConsole.h"

#ifndef DISABLE_BLE

#define BLE_CHIP_HC05             5
#define BLE_CHIP_HC06             6
#define BLE_CHIP_TYPE             BLE_CHIP_HC06

#define BLE_CMD_TIMEOUT           1000

#if BLE_CHIP_TYPE == BLE_CHIP_HC05
#define XLIGHT_BLE_BAUD           "38400,0,0"
#define XLIGHT_BLE_ROLE		        '0'		          // 0=>Slave role; 1=>Master role; 2=>Loopback
#else
#define XLIGHT_BLE_BAUD           '4'             // 1=>1200 baudios, 2=>2400, 3=>4800, 4=>9600 (default), 5=>19200, 6=>38400, 7=>57600, 8=>115200
#define XLIGHT_BLE_ROLE		        'S'		          // S=>Slave role; M=>Master role
#endif

#define BLE_BUFER_LENGTH          48

#ifdef PIN_BTN_EXT_1
#undef PIN_BTN_EXT_1
#endif

#ifdef PIN_BTN_EXT_2
#undef PIN_BTN_EXT_2
#endif

#ifdef PIN_BTN_EXT_3
#undef PIN_BTN_EXT_3
#endif

#ifdef PIN_BTN_EXT_4
#undef PIN_BTN_EXT_4
#endif

// AT commands for HC-05, ending with \r\n
// Input              Output
// AT                 OK
// AT+RESET           OK
// AT+VERSION?        +VERSION:version
//                    OK
// AT+ADDR?           +ADDR:address (NAP:UAP:LAP)
//                    OK
// AT+NAME?           +NAME:name
//                    OK or FAIL
// AT+NAME=name       OK
// AT+PSWD?           +PSWD:password
//                    OK
// AT+PSWD=password   OK
// AT+UART?           +UART=baud,stopbits,parity
//                    OK
// AT+UART=38400,0,0  OK
// AT+ROLE:           +ROLE:0|1|2
//                    OK
// AT+ROLE=0|1|2      OK
// AT+CLASS?          +CLASS:clas
//                    OK or FAIL
// AT+CLASS=class     OK

// AT commands for HC-06
// Input              Output
// AT                 OK
// AT+BAUD1-8         OK1200, OK2400...
// AT+NAMEname        OKsetname
// AT+PINpin          OKsetPIN
// AT+ROLE=S|M        OK+ROLE:S|M

//------------------------------------------------------------------
// the one and only instance of BLEInterfaceClass
BLEInterfaceClass theBLE;

char bleBuf[BLE_BUFER_LENGTH];
UC bleBufPos = 0;

//------------------------------------------------------------------
// Xlight SerialConsole Class
//------------------------------------------------------------------
BLEInterfaceClass::BLEInterfaceClass()
{
  m_token = random(65535); // Random number
  m_bGood = false;
#if BLE_CHIP_TYPE == BLE_CHIP_HC05
  m_speed = SERIALPORT_SPEED_HC05; // 38400
#else
  m_speed = SERIALPORT_SPEED_LOW; // 9600
#endif

  m_bATMode = false;
  m_lastCmd = "";
}

void BLEInterfaceClass::Init(UC _stpin, UC _enpin)
{
  // Open BLE Port
  m_pin_state = _stpin;
  m_pin_en = _enpin;
#if BLE_CHIP_TYPE == BLE_CHIP_HC05
  pinMode(m_pin_state, INPUT);
  pinMode(m_pin_en, OUTPUT);
#endif
  BLEPort.begin(m_speed);

  // Config BLE module
  m_name = theConfig.GetBLEName();
  m_pin = theConfig.GetBLEPin();
  config();
}

void MyDelay1S()
{
#if BLE_CHIP_TYPE == BLE_CHIP_HC05
  delay(250);
  Particle.process();
#else
  delay(250);
  Particle.process();
  delay(250);
  Particle.process();
  delay(250);
  Particle.process();
  delay(250);
#endif
}

void BLEInterfaceClass::config()
{
  // Module configuration:
  m_bGood = FALSE;
#if BLE_CHIP_TYPE == BLE_CHIP_HC05

  // Starts configuration
  digitalWrite(m_pin_en, HIGH);

  // Starts configuration
  BLEPort.println("AT"); MyDelay1S();

  // Adjusting bluetooth name
  BLEPort.printlnf("AT+NAME=%s", m_name.c_str()); MyDelay1S();

  // Baud rate adjust
  BLEPort.printlnf("AT+UART=%s", XLIGHT_BLE_BAUD); MyDelay1S();

  // Password adjust
  BLEPort.printlnf("AT+PSWD=%s", m_pin.c_str()); MyDelay1S();

  // CoD adjust
  BLEPort.printlnf("AT+CLASS=%x", XLIGHT_BLE_CLASS); // MyDelay1S();

  // Role adjust
  //BLEPort.printlnf("AT+ROLE=%c", XLIGHT_BLE_ROLE); MyDelay1S();

  // Stop configuration
  digitalWrite(m_pin_en, LOW);

#else

  // Starts configuration
  BLEPort.print("AT"); MyDelay1S();

  // Adjusting bluetooth name
  BLEPort.print("AT+NAME"); BLEPort.print(m_name); MyDelay1S();

  // Baud rate adjust
  BLEPort.print("AT+BAUD"); BLEPort.print(XLIGHT_BLE_BAUD); MyDelay1S();

  // Password adjust
  BLEPort.print("AT+PIN"); BLEPort.print(m_pin); MyDelay1S();

  // Role adjust
  BLEPort.print("AT+ROLE"); BLEPort.print(XLIGHT_BLE_ROLE); //MyDelay1S();
#endif
}

BOOL BLEInterfaceClass::isGood()
{
  return m_bGood;
}

BOOL BLEInterfaceClass::getState()
{
  return digitalRead(m_pin_state);
}

String BLEInterfaceClass::getName()
{
  return m_name;
}

BOOL BLEInterfaceClass::setName(String _name)
{
  if( _name != m_name) {
    m_name = _name;

    // Module configuration:
#if BLE_CHIP_TYPE == BLE_CHIP_HC05
    // Starts configuration
    digitalWrite(m_pin_en, HIGH);
    // Adjusting bluetooth name
    BLEPort.printlnf("AT+NAME=%s", m_name);
    // Stop configuration
    digitalWrite(m_pin_en, LOW);
#else
    // Adjusting bluetooth name
    BLEPort.print("AT+NAME"); BLEPort.print(m_name);
#endif

    return TRUE;
  }
  return FALSE;
}

String BLEInterfaceClass::getPin()
{
  return m_pin;
}

BOOL BLEInterfaceClass::setPin(String _pin)
{
  if( _pin != m_pin) {
    m_pin = _pin;

    // Module configuration:
#if BLE_CHIP_TYPE == BLE_CHIP_HC05
    // Starts configuration
    digitalWrite(m_pin_en, HIGH);
    // Password adjust
    BLEPort.printlnf("AT+PSWD=%s", m_pin);
    // Stop configuration
    digitalWrite(m_pin_en, LOW);
#else
    // Password adjust
    BLEPort.print("AT+PIN"); BLEPort.print(m_pin);
#endif

    return TRUE;
  }
  return FALSE;
}

UL BLEInterfaceClass::getSpeed()
{
  return m_speed;
}

BOOL BLEInterfaceClass::isATMode()
{
  return m_bATMode;
}

void BLEInterfaceClass::setATMode(BOOL on)
{
#if BLE_CHIP_TYPE == BLE_CHIP_HC05
  digitalWrite(m_pin_state, on ? HIGH : LOW);
#else
  if( on ) sendCommand("AT");    // wait for OK
#endif
  m_bATMode = on;
}

void BLEInterfaceClass::processCommand()
{
  static char preChar = 0;
  static char preChar2 = 0;
  static char preChar3 = 0;
  char myChar;

  while( BLEPort.available() > 0 ) {
    myChar = BLEPort.read();
#ifdef SERIAL_DEBUG
    TheSerial.print(myChar);
#endif
    if( preChar == 'O' && myChar == 'K') {
      m_bGood = TRUE;
    } else if( bleBufPos == 0 && preChar >= '0' &&  preChar <= '9' && myChar == ';' ) {
      m_bGood = TRUE;
      // Start of new message
      if( preChar2 >= '0' &&  preChar2 <= '9' ) {
        if( preChar3 >= '0' &&  preChar3 <= '9' ) bleBuf[bleBufPos++] = preChar3;
        bleBuf[bleBufPos++] = preChar2;
      }
      bleBuf[bleBufPos++] = preChar;
      bleBuf[bleBufPos++] = ';';
    } else if( bleBufPos > 0) {
      if( myChar == '\n' || myChar == '\r' ) {
        // Received an entire message
        bleBuf[bleBufPos] = 0;
        exectueCommand(bleBuf);
        bleBufPos = 0;
      } else {
        bleBuf[bleBufPos++] = myChar;
        if( bleBufPos >= BLE_BUFER_LENGTH ) bleBufPos = 0;
      }
    }
    preChar3 = preChar2;
    preChar2 = preChar;
    preChar = myChar;
  }
}

BOOL BLEInterfaceClass::sendCommand(String _cmd)
{
  UC len = _cmd.length();
  if( BLEPort.write((UC *)_cmd.c_str(), len) < len ) return false;
  m_lastCmd = _cmd;
  return true;
}

BOOL BLEInterfaceClass::sendNotification(const UC _id, String _data)
{
  if( !m_bGood ) return false;
  String lv_msg;
  lv_msg = String::format("%d;0;0;0;%d;%s\n", NODEID_SMARTPHONE, _id, _data.c_str());
  UC len = lv_msg.length();
  return( BLEPort.write((UC *)lv_msg.c_str(), len) == len );
}

// Parse and execute command
BOOL BLEInterfaceClass::exectueCommand(char *inputString)
{
  MyMessage lv_msg;
  if( serialMsgParser.parse(lv_msg, inputString) ) {
    UC _sensor, _sender, _msgType, _cmd, _nodeID;
  	bool _bIsAck, _needAck;
  	char *payload;
    String strCmd;

    _cmd = lv_msg.getCommand();
    _nodeID = lv_msg.getDestination();
    _sender = lv_msg.getSender();
    _sensor = lv_msg.getSensor();
		_msgType = lv_msg.getType();
		_bIsAck = lv_msg.isAck();
		_needAck = lv_msg.isReqAck();
		payload = (char *)lv_msg.getCustom();
    char strDisplay[SENSORDATA_JSON_SIZE];

    LOGD(LOGTAG_MSG, "BLECmd dest:%d cmd:%d type:%d sender:%d sensor:%d ack:%d rack:%d",
          _nodeID, _cmd, _msgType, _sender, _sensor, _bIsAck, _needAck);

    switch( _cmd ) {
      case C_INTERNAL:
      if( _msgType == I_ID_REQUEST && _needAck ) {
          // Login
          if( _sender == NODEID_PROJECTOR || _sender == NODEID_SMARTPHONE ) {
            // Check Access code
            lv_msg.build(NODEID_GATEWAY, _sender, _sensor, C_INTERNAL, I_ID_RESPONSE, false, true);
            if( theConfig.CheckPPTAccessCode(payload) ) {
              m_token = random(65535); // Random number
              lv_msg.set((unsigned int)m_token);
              LOGD(LOGTAG_MSG, "%s-%d login OK", _sender == NODEID_PROJECTOR ? "PPTCtrl" : "APP", _sensor);
            } else {
              lv_msg.set((unsigned int)0);
              LOGI(LOGTAG_MSG, "%s-%d login failed", _sender == NODEID_PROJECTOR ? "PPTCtrl" : "APP", _sensor);
            }
						// Convert to serial format
						memset(strDisplay, 0x00, sizeof(strDisplay));
						lv_msg.getSerialString(strDisplay);
						sendCommand(strDisplay);
          }
      } else if( _msgType == I_CONFIG && _needAck ) {
        // Config Controller
        if( _sender == NODEID_SMARTPHONE ) {
          if( payload[0] == '0' ) {
            // Wi-Fi(0):SSID:Password:Auth:Cipher
            // Auth: WEP=1, WPA=2, WPA2=3
            // Cipher: WLAN_CIPHER_AES=1, WLAN_CIPHER_TKIP=2, WLAN_CIPHER_AES_TKIP=3
            gintWiFi_Auth = 0;
            gintWiFi_Cipher = 0;
            char *token, *last;
            token = strtok_r(payload, ":", &last);
            if( token ) {
              token = strtok_r(NULL, ":", &last);
              if( token ) {
                gstrWiFi_SSID = token;
                gstrWiFi_Password = "";
                token = strtok_r(NULL, ":", &last);
                if( token ) {
                  gstrWiFi_Password = token;
                  token = strtok_r(NULL, ":", &last);
                  if( token ) {
                    gintWiFi_Auth = atoi(token);
                    if( gintWiFi_Auth > 3 || gintWiFi_Auth < 0 ) {
                      gintWiFi_Auth = 0;
                    } else {
                      token = strtok_r(NULL, ":", &last);
                      if( token ) {
                        gintWiFi_Cipher = atoi(token);
                        if( gintWiFi_Cipher > 3 || gintWiFi_Cipher < 0 ) {
                          gintWiFi_Cipher = 0;
                        }
                      }
                    }
                  }
                }

                theConsole.UpdateWiFiCredential();

                strCmd = String::format("%d;0;3;2;6;1:0:%s\n", NODEID_SMARTPHONE, theSys.GetSysID().c_str());
                sendCommand(strCmd);

                WiFi.listen(false);
                theSys.connectWiFi();
              }
            } else {
              strCmd = String::format("%d;0;3;2;6;0:0\n", NODEID_SMARTPHONE);
              sendCommand(strCmd);
            }
          } else if( payload[0] == '1' ) {
            // Query CoreID
            strCmd = String::format("%d;0;3;2;6;1:1:%s\n", NODEID_SMARTPHONE, theSys.GetSysID().c_str());
            sendCommand(strCmd);
          }else {
            // serial set command
            strCmd = String::format("set %s", payload);
            theConsole.ExecuteCloudCommand(strCmd.c_str());

            strCmd = String::format("%d;0;3;2;6;1:%s\n", NODEID_SMARTPHONE, payload);
            sendCommand(strCmd);
          }
        }
      } else if( _msgType == I_REBOOT ) {
        if( _sender == NODEID_SMARTPHONE ) {
          strCmd = String::format("%d;0;3;2;13;1:%s\n", NODEID_SMARTPHONE, payload);
          sendCommand(strCmd);

          strCmd = String::format("sys %s", payload);
          theConsole.ExecuteCloudCommand(strCmd.c_str());
        }
      }
      break;

      case C_SET:
      if( _bIsAck ) {
        // Reply from PPTCtrl or Smartphone
        LOGD(LOGTAG_MSG, "Got C_SET:%d ack to %d from %d-%d:%s", _msgType, _nodeID, _sender, _sensor, payload);
      } else if( _sender == NODEID_SMARTPHONE ) {
        // Request from Smartphone
        String lv_sPayload = payload;
        theRadio.ProcessSend(_nodeID, _msgType, lv_sPayload, lv_msg, _sender, _sensor);
      }
      break;
    }
    return true;
  } else {
    LOGI(LOGTAG_MSG, "Failed to parse BLE msg %s", inputString);
  }
  return false;
}

#endif // DISABLE_BLE
