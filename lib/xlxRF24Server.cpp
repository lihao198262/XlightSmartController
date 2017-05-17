/**
 * xlxRF24Server.cpp - Xlight RF2.4 server library based on via RF2.4 module
 * for communication with instances of xlxRF24Client
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
 * Dependancy
 * 1. Particle RF24 library, refer to
 *    https://github.com/stewarthou/Particle-RF24
 *    http://tmrh20.github.io/RF24/annotated.html
 *
 * DESCRIPTION
 * 1. Mysensors protocol compatible and extended
 * 2. Address algorithm
 * 3. PipePool (0 AdminPipe, Read & Write; 1-5 pipe pool, Read)
 * 4. Session manager, optional address shifting
 * 5.
 *
 * ToDo:
 * 1. Two pipes collaboration for security: divide a message into two parts
 * 2. Apply security layer, e.g. AES, encryption, etc.
 *
**/

#include "xlxRF24Server.h"
#include "xliPinMap.h"
#include "xliNodeConfig.h"
#include "xlSmartController.h"
#include "xlxLogger.h"
#include "xlxPanel.h"
#include "xlxBLEInterface.h"

#include "MyParserSerial.h"

//------------------------------------------------------------------
// the one and only instance of RF24ServerClass
RF24ServerClass theRadio(PIN_RF24_CE, PIN_RF24_CS);
MyMessage msg;
UC *msgData = (UC *)&(msg.msg);

RF24ServerClass::RF24ServerClass(uint8_t ce, uint8_t cs, uint8_t paLevel)
	:	MyTransportNRF24(ce, cs, paLevel)
	, CDataQueue(MAX_MESSAGE_LENGTH * MQ_MAX_RF_RCVMSG)
	, CFastMessageQ(MQ_MAX_RF_SNDMSG, MAX_MESSAGE_LENGTH)
{
	_times = 0;
	_succ = 0;
	_received = 0;
}

bool RF24ServerClass::ServerBegin()
{
  // Initialize RF module
	if( !init() ) {
    LOGC(LOGTAG_MSG, "RF24 module is not valid!");
		return false;
	}

  // Set role to Controller or Gateway
	SetRole_Gateway();
  return true;
}

// Make NetworkID with the right 4 bytes of device MAC address
uint64_t RF24ServerClass::GetNetworkID(bool _full)
{
  uint64_t netID = 0;

  byte mac[6];
  WiFi.macAddress(mac);
	int i = (_full ? 0 : 2);
  for (; i<6; i++) {
    netID += mac[i];
		if( _full && i == 5 ) break;
    netID <<= 8;
  }

  return netID;
}

bool RF24ServerClass::ChangeNodeID(const uint8_t bNodeID)
{
	if( bNodeID == getAddress() ) {
			return true;
	}

	if( bNodeID == 0 ) {
		SetRole_Gateway();
	} else {
		setAddress(bNodeID, RF24_BASE_RADIO_ID);
	}

	return true;
}

void RF24ServerClass::SetRole_Gateway()
{
	uint64_t lv_networkID = GetNetworkID();
	setAddress(GATEWAY_ADDRESS, lv_networkID);
}

bool RF24ServerClass::ProcessMQ()
{
	ProcessSendMQ();
	ProcessReceiveMQ();
	return true;
}

bool RF24ServerClass::ProcessSend(const UC _node, const UC _msgID, String &strPayl, MyMessage &my_msg, const UC _replyTo)
{
	bool sentOK = false;
	bool bMsgReady = false;
	uint8_t bytValue;
	int iValue;
	float fValue;
	char strBuffer[64];
	uint8_t payload[MAX_PAYLOAD];
	MyMessage lv_msg;
	int nPos;

	switch (_msgID)
	{
	case 0: // Free style
		iValue = min(strPayl.length(), 63);
		strncpy(strBuffer, strPayl.c_str(), iValue);
		strBuffer[iValue] = 0;
		// Serail format to MySensors message structure
		bMsgReady = serialMsgParser.parse(lv_msg, strBuffer);
		if (bMsgReady) {
			SERIAL("Now sending message...");
		}
		break;

	case 1:   // Request new node ID
		if (getAddress() == GATEWAY_ADDRESS) {
			if( _node == GATEWAY_ADDRESS ) {
				SERIAL_LN("Controller can not request node ID\n\r");
			} else {
				// Set specific NodeID to node
				UC newID = _node;
				if( strPayl.length() > 0 ) {
					newID = (UC)strPayl.toInt();
				}
				if( newID > 0 ) {
					lv_msg.build(getAddress(), _node, newID, C_INTERNAL, I_ID_RESPONSE, false, true);
					lv_msg.set(getMyNetworkID());
					bMsgReady = true;
					//theConfig.lstNodes.clearNodeId(_node);
					SERIAL("Now sending new id:%d to node:%d...", newID, _node);
				}
			}
		}
		else {
			UC deviceType = (getAddress() == 3 ? NODE_TYP_REMOTE : NODE_TYP_LAMP);
			ChangeNodeID(AUTO);
			lv_msg.build(AUTO, BASESERVICE_ADDRESS, deviceType, C_INTERNAL, I_ID_REQUEST, false);
			lv_msg.set(GetNetworkID(true));		// identity: could be MAC, serial id, etc
			bMsgReady = true;
			SERIAL("Now sending request node id message...");
		}
		break;

	case 2:   // Node Config
		{
			nPos = strPayl.indexOf(':');
			if (nPos > 0) {
				bytValue = (uint8_t)(strPayl.substring(0, nPos).toInt());
				iValue = strPayl.substring(nPos + 1).toInt();
				lv_msg.build(getAddress(), _node, bytValue, C_INTERNAL, I_CONFIG, true);
				lv_msg.set((unsigned int)iValue);
				bMsgReady = true;
				SERIAL("Now sending node:%d config:%d value:%d...", _node, bytValue, iValue);
			}
		}
		break;

	case 3:   // Temperature sensor present with sensor id 1, req no ack
		lv_msg.build(getAddress(), _node, _replyTo, C_PRESENTATION, S_TEMP, false);
		lv_msg.set("");
		bMsgReady = true;
		SERIAL("Now sending DHT11 present message...");
		break;

	case 4:   // Temperature set to 23.5, req no ack
		lv_msg.build(getAddress(), _node, _replyTo, C_SET, V_TEMP, false);
		fValue = 23.5;
		if( strPayl.length() > 0 ) {
			fValue = strPayl.toFloat();
		}
		lv_msg.set(fValue, 2);
		bMsgReady = true;
		SERIAL("Now sending set temperature message...");
		break;

	case 5:   // Humidity set to 45, req no ack
		lv_msg.build(getAddress(), _node, _replyTo, C_SET, V_HUM, false);
		iValue = 45;
		if( strPayl.length() > 0 ) {
			iValue = strPayl.toInt();
		}
		lv_msg.set(iValue);
		bMsgReady = true;
		SERIAL("Now sending set humidity message...");
		break;

	case 6:   // Get main lamp(ID:1) power(V_STATUS:2) on/off, ack
		lv_msg.build(getAddress(), _node, _replyTo, C_REQ, V_STATUS, true);
		bMsgReady = true;
		SERIAL("Now sending get V_STATUS message...");
		break;

	case 7:   // Set main lamp(ID:1) power(V_STATUS:2) on/off, ack
		lv_msg.build(getAddress(), _node, _replyTo, C_SET, V_STATUS, true);
		bytValue = constrain(strPayl.toInt(), DEVICE_SW_OFF, DEVICE_SW_TOGGLE);
		lv_msg.set(bytValue);
		bMsgReady = true;
		SERIAL("Now sending set V_STATUS %s message...", (bytValue ? "on" : "off"));
		break;

	case 8:   // Get main lamp(ID:1) dimmer (V_PERCENTAGE:3), ack
		lv_msg.build(getAddress(), _node, _replyTo, C_REQ, V_PERCENTAGE, true);
		bMsgReady = true;
		SERIAL("Now sending get V_PERCENTAGE message...");
		break;

	case 9:   // Set main lamp(ID:1) dimmer (V_PERCENTAGE:3), ack
		lv_msg.build(getAddress(), _node, _replyTo, C_SET, V_PERCENTAGE, true);
		bytValue = constrain(strPayl.toInt(), 0, 100);
		lv_msg.set((uint8_t)OPERATOR_SET, bytValue);
		bMsgReady = true;
		SERIAL("Now sending set V_PERCENTAGE:%d message...", bytValue);
		break;

	case 10:  // Get main lamp(ID:1) color temperature (V_LEVEL), ack
		lv_msg.build(getAddress(), _node, _replyTo, C_REQ, V_LEVEL, true);
		bMsgReady = true;
		SERIAL("Now sending get CCT V_LEVEL message...");
		break;

	case 11:  // Set main lamp(ID:1) color temperature (V_LEVEL), ack
		lv_msg.build(getAddress(), _node, _replyTo, C_SET, V_LEVEL, true);
		iValue = constrain(strPayl.toInt(), CT_MIN_VALUE, CT_MAX_VALUE);
		lv_msg.set((uint8_t)OPERATOR_SET, (unsigned int)iValue);
		bMsgReady = true;
		SERIAL("Now sending set CCT V_LEVEL %d message...", iValue);
		break;

	case 12:  // Request lamp status in one
		lv_msg.build(getAddress(), _node, _replyTo, C_REQ, V_RGBW, true);
		lv_msg.set((uint8_t)RING_ID_ALL);		// RING_ID_1 is also workable currently
		bMsgReady = true;
		SERIAL("Now sending get dev-status (V_RGBW) message...");
		break;

	case 13:  // Set main lamp(ID:1) status in one, ack
		lv_msg.build(getAddress(), _node, _replyTo, C_SET, V_RGBW, true);
		payload[0] = RING_ID_ALL;
		payload[1] = 1;
		payload[2] = 65;
		nPos = strPayl.indexOf(':');
		if (nPos > 0) {
			// Extract brightness, cct or WRGB
			bytValue = (uint8_t)(strPayl.substring(0, nPos).toInt());
			payload[2] = bytValue;
			iValue = strPayl.substring(nPos + 1).toInt();
			if( iValue < 256 ) {
				// WRGB
				payload[3] = iValue;	// W
				payload[4] = 0;	// R
				payload[5] = 0;	// G
				payload[6] = 0;	// B
				for( int cindex = 3; cindex < 7; cindex++ ) {
					strPayl = strPayl.substring(nPos + 1);
					nPos = strPayl.indexOf(':');
					if (nPos <= 0) {
						bytValue = (uint8_t)(strPayl.toInt());
						payload[cindex] = bytValue;
						break;
					}
					bytValue = (uint8_t)(strPayl.substring(0, nPos).toInt());
					payload[cindex] = bytValue;
				}
				lv_msg.set((void*)payload, 7);
				SERIAL("Now sending set BR=%d WRGB=(%d,%d,%d,%d)...",
						payload[2], payload[3], payload[4], payload[5], payload[6]);
			} else {
				// CCT
				iValue = constrain(iValue, CT_MIN_VALUE, CT_MAX_VALUE);
				payload[3] = iValue % 256;
			  payload[4] = iValue / 256;
				lv_msg.set((void*)payload, 5);
				SERIAL("Now sending set BR=%d CCT=%d...", bytValue, iValue);
			}
		} else {
			iValue = 3000;
			payload[3] = iValue % 256;
		  payload[4] = iValue / 256;
			lv_msg.set((void*)payload, 5);
			SERIAL("Now sending set BR=%d CCT=%d...", bytValue, iValue);
		}
		bMsgReady = true;
		break;

	case 14:	// Reserved for query command
	case 16:	// Reserved for query command
		break;

	case 15:	// Set Device Scenerio
		bytValue = (UC)(strPayl.toInt());
		theSys.ChangeLampScenario(_node, bytValue, _replyTo);
		break;

	case 17:	// Set special effect
		lv_msg.build(getAddress(), _node, _replyTo, C_SET, V_VAR1, true);
		bytValue = (UC)(strPayl.toInt());
		lv_msg.set(bytValue);
		bMsgReady = true;
		SERIAL("Now setting special effect %d...", bytValue);
		break;
	}

	if (bMsgReady) {
		SERIAL("to %d...", lv_msg.getDestination());
		sentOK = ProcessSend(&lv_msg);
		my_msg = lv_msg;
		SERIAL_LN(sentOK ? "OK" : "failed");
	}

	return sentOK;
}

bool RF24ServerClass::ProcessSend(String &strMsg, MyMessage &my_msg, const UC _replyTo)
{
	int nPos = strMsg.indexOf(':');
	int nPos2;
	uint8_t lv_nNodeID;
	uint8_t lv_nMsgID;
	String lv_sPayload = "";
	if (nPos > 0) {
		// Extract NodeID & MessageID
		lv_nNodeID = (uint8_t)(strMsg.substring(0, nPos).toInt());
		lv_nMsgID = (uint8_t)(strMsg.substring(nPos + 1).toInt());
		nPos2 = strMsg.indexOf(':', nPos + 1);
		if (nPos2 > 0) {
			lv_nMsgID = (uint8_t)(strMsg.substring(nPos + 1, nPos2).toInt());
			lv_sPayload = strMsg.substring(nPos2 + 1);
		} else {
			lv_nMsgID = (uint8_t)(strMsg.substring(nPos + 1).toInt());
		}
	}
	else {
		// Parse serial message
		lv_nMsgID = 0;
		lv_sPayload = strMsg;
	}

	return ProcessSend(lv_nNodeID, lv_nMsgID, lv_sPayload, my_msg, _replyTo);
}

bool RF24ServerClass::ProcessSend(String &strMsg, const UC _replyTo)
{
	MyMessage tempMsg;
	return ProcessSend(strMsg, tempMsg, _replyTo);
}

// ToDo: add message to queue instead of sending out directly
bool RF24ServerClass::ProcessSend(MyMessage *pMsg)
{
	if( !pMsg ) { pMsg = &msg; }

	// Convent message if necessary
	bool _bConvert = false;
	if( pMsg->getDestination() == BROADCAST_ADDRESS || IS_GROUP_NODEID(pMsg->getDestination()) ) {
		if( theConfig.GetBcMsgRptTimes() > 0 ) {
			_bConvert = true;
		}
	} else if( theConfig.GetNdMsgRptTimes() > 0 ) {
		_bConvert = true;
	}
	if( _bConvert ) {
		ConvertRepeatMsg(pMsg);
	}

	// Add message to sending MQ. Right now tag has no actual purpose (just for debug)
	if( AddMessage((UC *)&(pMsg->msg), MAX_MESSAGE_LENGTH, GetMQLength()) > 0 ) {
		_times++;
		//LOGD(LOGTAG_MSG, "Add sendMQ len:%d", GetMQLength());
		return true;
	}

	LOGW(LOGTAG_MSG, "Failed to add sendMQ");
	return false;
}

void RF24ServerClass::ConvertRepeatMsg(MyMessage *pMsg)
{
	// Note: change relative value to absolute value
	if( pMsg->getCommand() == C_SET ) {
		uint8_t *payload = (uint8_t *)pMsg->getCustom();
		uint8_t bytValue = payload[0];
		if( pMsg->getType() == V_STATUS && bytValue == DEVICE_SW_TOGGLE ) {
			bytValue = 1 - theSys.GetDevOnOff(CURRENT_DEVICE);
			pMsg->set(bytValue);
		} else if( pMsg->getType() == V_PERCENTAGE && pMsg->getLength() == 2 ) {
			if( bytValue != OPERATOR_SET ) {
				payload[0] = OPERATOR_SET;
				if( bytValue == OPERATOR_ADD ) {
					payload[1] += theSys.GetDevBrightness(CURRENT_DEVICE);
					if( payload[1] > 100 ) payload[1] = 100;
				} else if( bytValue == OPERATOR_SUB ) {
					if( theSys.GetDevBrightness(CURRENT_DEVICE) > payload[1] + BR_MIN_VALUE ) {
						payload[1] = theSys.GetDevBrightness(CURRENT_DEVICE) - payload[1];
					} else {
						payload[1] = BR_MIN_VALUE;
					}
				}
			}
		} else if( pMsg->getType() == V_LEVEL && pMsg->getLength() == 3 ) {
			if( bytValue != OPERATOR_SET ) {
				uint16_t _CCTValue = theSys.GetDevCCT(CURRENT_DEVICE);
				uint16_t _deltaValue = payload[2] * 256 + payload[1];
				payload[0] = OPERATOR_SET;
				if( bytValue == OPERATOR_ADD ) {
					_CCTValue += _deltaValue;
					if( _CCTValue > CT_MAX_VALUE ) _CCTValue = CT_MAX_VALUE;
				} else if( bytValue == OPERATOR_SUB ) {
					if( _CCTValue > _deltaValue + CT_MIN_VALUE ) {
						_CCTValue -= _deltaValue;
					} else {
						_CCTValue = CT_MIN_VALUE;
					}
				}
				payload[1] = _CCTValue % 256;
				payload[2] = _CCTValue / 256;
			}
		}
	}
}

bool RF24ServerClass::SendNodeConfig(UC _node, UC _ncf, unsigned int _value)
{
	// Notify Remote Node
	MyMessage lv_msg;
	lv_msg.build(NODEID_GATEWAY, _node, _ncf, C_INTERNAL, I_CONFIG, true);
	lv_msg.set(_value);
	return ProcessSend(&lv_msg);
}

// Get messages from RF buffer and store them in MQ
bool RF24ServerClass::PeekMessage()
{
	if( !isValid() ) return false;

	UC to = 0;
  UC pipe;
	UC len;
	MyMessage lv_msg;
	UC *lv_pData = (UC *)&(lv_msg.msg);

	while (available(&to, &pipe)) {
		len = receive(lv_pData);

		// rough check
	  if( len < HEADER_SIZE )
	  {
	    LOGW(LOGTAG_MSG, "got corrupt dynamic payload!");
	    return false;
	  } else if( len > MAX_MESSAGE_LENGTH )
	  {
	    LOGW(LOGTAG_MSG, "message length exceeded: %d", len);
	    return false;
	  }

	  _received++;
	  LOGD(LOGTAG_MSG, "Received from pipe %d msg-len=%d, from:%d to:%d dest:%d cmd:%d type:%d sensor:%d payl-len:%d",
	        pipe, len, lv_msg.getSender(), to, lv_msg.getDestination(), lv_msg.getCommand(),
	        lv_msg.getType(), lv_msg.getSensor(), lv_msg.getLength());
		if( Append(lv_pData, len) <= 0 ) return false;
	}
	return true;
}

// Parse and process message in MQ
bool RF24ServerClass::ProcessReceiveMQ()
{
	bool msgReady;
	UC len, payl_len;
	UC replyTo, _sensor, msgType, transTo;
	bool _bIsAck, _needAck;
	UC *payload;
	UC _bValue;
	US _iValue;
	char strDisplay[SENSORDATA_JSON_SIZE];

  while (Length() > 0) {

		msgReady = false;
	  len = Remove(MAX_MESSAGE_LENGTH, msgData);
		payl_len = msg.getLength();
		_sensor = msg.getSensor();
		msgType = msg.getType();
		replyTo = msg.getSender();
		_bIsAck = msg.isAck();
		_needAck = msg.isReqAck();
		payload = (uint8_t *)msg.getCustom();

		/*
	  memset(strDisplay, 0x00, sizeof(strDisplay));
	  msg.getJsonString(strDisplay);
	  SERIAL_LN("  JSON: %s, len: %d", strDisplay, strlen(strDisplay));
	  memset(strDisplay, 0x00, sizeof(strDisplay));
	  SERIAL_LN("  Serial: %s, len: %d", msg.getSerialString(strDisplay), strlen(strDisplay));
		*/
		LOGD(LOGTAG_MSG, "Will process cmd:%d from:%d type:%d sensor:%d",
					msg.getCommand(), replyTo, msgType, _sensor);

	  switch( msg.getCommand() )
	  {
	    case C_INTERNAL:
	      if( msgType == I_ID_REQUEST ) {
	        // On ID Request message:
	        /// Get new ID
					char cNodeType = (char)_sensor;
					uint64_t nIdentity = msg.getUInt64();
	        UC newID = theConfig.lstNodes.requestNodeID(replyTo, cNodeType, nIdentity);

	        /// Send response message
	        msg.build(getAddress(), replyTo, newID, C_INTERNAL, I_ID_RESPONSE, false, true);
					if( newID > 0 ) {
		        msg.set(getMyNetworkID());
		        LOGN(LOGTAG_EVENT, "Allocated NodeID:%d type:%c to %s", newID, cNodeType, PrintUint64(strDisplay, nIdentity));
					} else {
						LOGW(LOGTAG_MSG, "Failed to allocate NodeID type:%c to %s", cNodeType, PrintUint64(strDisplay, nIdentity));
					}
					msgReady = true;
	      } else if( msgType == I_CONFIG ) {
					if( _bIsAck ) {
						if( _sensor == NCF_QUERY ) {
							theSys.GotNodeConfigAck(replyTo, payload);
						}
					}
				}
	      break;

			case C_PRESENTATION:
				if( _sensor == S_LIGHT || _sensor == S_DIMMER ) {
					US token;
					if( _needAck ) {
						// Presentation message: appear of Smart Lamp
						// Verify credential, return token if true, and change device status
						UC lv_nNodeID = msg.getSender();
						UC lv_assoDev;
						uint64_t nIdentity = msg.getUInt64();
						if( IS_GROUP_NODEID(lv_nNodeID) ) {
							token = 6666;
						} else {
							token = theSys.VerifyDevicePresence(&lv_assoDev, lv_nNodeID, msgType, nIdentity);
						}
						if( token ) {
							// return token
							// Notes: lampType & S_LIGHT (msgType) are not necessary, use for associated device
			        msg.build(getAddress(), replyTo, _sensor, C_PRESENTATION, lv_assoDev, false, true);
							msg.set((unsigned int)token);
							msgReady = true;
							// ToDo: send status req to this lamp
						} else {
							LOGW(LOGTAG_MSG, "Unqualitied device connect attemp received");
						}
					}
				} else {
					ListNode<DevStatusRow_t> *DevStatusRowPtr = theSys.SearchDevStatus(replyTo);
					if (DevStatusRowPtr) theSys.ConfirmLampPresent(DevStatusRowPtr, true);
					if( _sensor == S_IR ) {
						if( msgType == V_STATUS) { // PIR
							theSys.UpdateMotion(replyTo, msg.getByte()!=DEVICE_SW_OFF);
						}
					} else if( _sensor == S_LIGHT_LEVEL ) {
						if( msgType == V_LIGHT_LEVEL) { // ALS
							theSys.UpdateBrightness(replyTo, msg.getByte());
						}
					} else if( _sensor == S_SOUND ) {
						if( msgType == V_STATUS ) { // MIC
							theSys.UpdateSound(replyTo, payload[0]);
						} else if( msgType == V_LEVEL ) {
							_iValue = payload[1] * 256 + payload[0];
							theSys.UpdateNoise(replyTo, _iValue);
						}
					} else if( _sensor == S_DUST || _sensor == S_AIR_QUALITY || _sensor == S_SMOKE ) {
						if( msgType == V_LIGHT_LEVEL) { // Dust, Gas or Smoke
							_iValue = payload[1] * 256 + payload[0];
							if( _sensor == S_DUST ) {
								theSys.UpdateDust(replyTo, _iValue);
							} else if( _sensor == S_AIR_QUALITY ) {
								theSys.UpdateGas(replyTo, _iValue);
							} else if( _sensor == S_SMOKE ) {
								theSys.UpdateSmoke(replyTo, _iValue);
							}
						}
					}
				}
				break;

			case C_REQ:
				if( msgType == V_STATUS || msgType == V_PERCENTAGE || msgType == V_LEVEL
					  || msgType == V_RGBW || msgType == V_DISTANCE || msgType == V_VAR1 ) {
					transTo = (msg.getDestination() == getAddress() ? _sensor : msg.getDestination());
					BOOL bDataChanged = false;
					if( _bIsAck ) {
						//SERIAL_LN("REQ ack:%d to: %d 0x%x-0x%x-0x%x-0x%x-0x%x-0x%x-0x%x", msgType, transTo, payload[0],payload[1], payload[2], payload[3], payload[4],payload[5],payload[6]);
						if( msgType == V_STATUS ||  msgType == V_PERCENTAGE ) {
							bDataChanged |= theSys.ConfirmLampBrightness(replyTo, payload[0], payload[1]);
						} else if( msgType == V_LEVEL ) {
							bDataChanged |= theSys.ConfirmLampCCT(replyTo, (US)msg.getUInt());
						} else if( msgType == V_RGBW ) {
							if( payload[0] ) {	// Succeed or not
								static bool bFirstRGBW = true;		// Make sure the first message will be sent anyway
								UC _devType = payload[1];	// payload[2] is present status
								UC _ringID = payload[3];
								if( IS_SUNNY(_devType) ) {
									// Sunny
									US _CCTValue = payload[7] * 256 + payload[6];
									bDataChanged |= theSys.ConfirmLampCCT(replyTo, _CCTValue, _ringID);
									bDataChanged |= theSys.ConfirmLampBrightness(replyTo, payload[4], payload[5], _ringID);
									bDataChanged |= bFirstRGBW;
									bFirstRGBW = false;
								} else if( IS_RAINBOW(_devType) || IS_MIRAGE(_devType) ) {
									// Rainbow or Mirage, set RBGW
									bDataChanged |= theSys.ConfirmLampHue(replyTo, payload[6], payload[7], payload[8], payload[9], _ringID);
									bDataChanged |= theSys.ConfirmLampBrightness(replyTo, payload[4], payload[5], _ringID);
									bDataChanged |= bFirstRGBW;
									bFirstRGBW = false;
								}
							}
						} else if( msgType == V_VAR1 ) { // Change special effect ack
							bDataChanged |= theSys.ConfirmLampFilter(replyTo, payload[0]);
						} else if( msgType == V_DISTANCE && payload[0] ) {
							UC _devType = payload[1];	// payload[2] is present status
							if( IS_MIRAGE(_devType) ) {
								bDataChanged |= theSys.ConfirmLampTop(replyTo, payload, payl_len);
							}
						}

						// If data changed, new status must broadcast to all end points
						if( bDataChanged ) {
							transTo = BROADCAST_ADDRESS;
						}
					} /* else { // Request
						// ToDo: verify token
					} */

					// ToDo: if lamp is not present, return error
					if( transTo > 0 ) {
						// Transfer message
						msg.build(getAddress(), transTo, replyTo, C_REQ, msgType, _needAck, _bIsAck, true);
						// Keep payload unchanged
						msgReady = true;
					}
				}
				break;

			case C_SET:
				// ToDo: verify token
				// ToDo: if lamp is not present, return error
				if( _sensor == NODEID_PROJECTOR ) {
					// PPT control
					if( msgType == V_STATUS ) {
						// Keep payload unchanged
						msg.build(NODEID_PROJECTOR, NODEID_PROJECTOR, replyTo, C_SET, msgType, _needAck, _bIsAck, true);
						// Convert to serial format
						memset(strDisplay, 0x00, sizeof(strDisplay));
						msg.getSerialString(strDisplay);
						if( theBLE.isGood() ) theBLE.sendCommand(strDisplay);
					}
				} else {
					transTo = (msg.getDestination() == getAddress() ? _sensor : msg.getDestination());
					if( transTo > 0 ) {
						if( msgType == V_SCENE_ON ) {
							theSys.ChangeLampScenario(transTo, payload[0]);
						}	else {
							// Transfer message
							msg.build(getAddress(), transTo, replyTo, C_SET, msgType, _needAck, _bIsAck, true);
							// Keep payload unchanged
							msgReady = true;
						}
					}
				}
				break;

	    default:
	      break;
	  }

		// Send reply message
		if( msgReady ) {
			ProcessSend(&msg);
		}
	}

  return true;
}

// Scan sendMQ and send messages, repeat if necessary
bool RF24ServerClass::ProcessSendMQ()
{
	MyMessage lv_msg;
	UC *pData = (UC *)&(lv_msg.msg);
	CFastMessageNode *pNode = NULL, *pOld;
	UC pipe, _repeat, _tag;
	bool _remove;

	if( GetMQLength() > 0 ) {
		while( pNode = GetMessage(pNode) ) {
			pOld = pNode;
			// Get message data
			if( pNode->ReadMessage(pData, &_repeat, &_tag, 15) > 0 )
			{
				// Determine pipe
				if( lv_msg.getCommand() == C_INTERNAL && lv_msg.getType() == I_ID_RESPONSE ) {
					pipe = CURRENT_NODE_PIPE;
				} else {
					pipe = PRIVATE_NET_PIPE;
				}

				// Send message
				_remove = send(lv_msg.getDestination(), lv_msg, pipe);
				LOGD(LOGTAG_MSG, "RF-send msg %d-%d tag %d to %d pipe %d tried %d %s", lv_msg.getCommand(), lv_msg.getType(), _tag, lv_msg.getDestination(), pipe, _repeat, _remove ? "OK" : "Failed");

				// Determine whether requires retry
				if( lv_msg.getDestination() == BROADCAST_ADDRESS || IS_GROUP_NODEID(lv_msg.getDestination()) ) {
					if( _remove && _repeat == 1 ) _succ++;
					_remove = (_repeat > theConfig.GetBcMsgRptTimes());
				} else {
					if( _remove ) _succ++;
					if( _repeat > theConfig.GetNdMsgRptTimes() ) 	_remove = true;
				}

				// Remove message if succeeded or retried enough times
				if( _remove ) {
					RemoveMessage(pNode);
				}
			}

			// Next node
			pNode = pOld->m_pNext;
		}
	}

	return true;
}
