/**
 * xlSmartController.cpp - contains the major implementation of SmartController.
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
 * 1.
 *
 * ToDo:
**/
#include "xlSmartController.h"
#include "xliPinMap.h"
#include "xliNodeConfig.h"
#include "xlxConfig.h"
#include "xlxLogger.h"
#include "xlxPanel.h"
#include "xlxRF24Server.h"
#include "xlxSerialConsole.h"
#include "xlxASRInterface.h"
#include "xlxBLEInterface.h"

#include "Adafruit_DHT.h"
#include "ArduinoJson.h"
//#include "LightSensor.h"
//#include "MotionSensor.h"
#include "TimeAlarms.h"

//------------------------------------------------------------------
// Global Data Structures & Variables
//------------------------------------------------------------------
// make an instance for main program
SmartControllerClass theSys;

DHT senDHT(PIN_SEN_DHT, SEN_TYPE_DHT);
//LightSensor senLight(PIN_SEN_LIGHT);
//MotionSensor senMotion(PIN_SEN_PIR);

//------------------------------------------------------------------
// Alarm Triggered Actions
//------------------------------------------------------------------
void AlarmTimerTriggered(uint32_t tag)
{
	if( tag == 255 ) return;
	uint8_t rule_uid = (uint8_t)tag;
	SERIAL_LN("Rule %u Alarm Triggered", rule_uid);

	//search Rule table for matching UID
	ListNode<RuleRow_t> *RuleRowptr = theSys.Rule_table.search(rule_uid);
	if (RuleRowptr == NULL)
	{
		LOGE(LOGTAG_MSG, "Error, could not locate Rule Row corresponding to triggered Alarm ID");
		return;
	}

	// Execute the rule with init-flag
	theSys.Execute_Rule(RuleRowptr, true);
}

//------------------------------------------------------------------
// Smart Controller Class
//------------------------------------------------------------------
SmartControllerClass::SmartControllerClass()
{
	m_isRF = false;
	m_isLAN = false;
	m_isWAN = false;
}

// Primitive initialization before loading configuration
void SmartControllerClass::Init()
{
  // Open Serial Port
  TheSerial.begin(SERIALPORT_SPEED_DEFAULT);

#ifdef SYS_SERIAL_DEBUG
	// Wait Serial connection so that we can see the starting information
	while(!TheSerial.available()) {
		if( Particle.connected() == true ) { Particle.process(); }
	}
	SERIAL_LN("SmartController is starting...");
#endif

	// Get System ID
	m_SysID = System.deviceID();
	m_SysVersion = System.version();
	m_SysStatus = STATUS_INIT;

	// Initialize Logger
	theLog.Init(m_SysID);
	theLog.InitFlash(MEM_OFFLINE_DATA_OFFSET, MEM_OFFLINE_DATA_LEN);

	// Open ASR Interface
	theASR.Init(SERIALPORT_SPEED_LOW);

	LOGN(LOGTAG_MSG, "SmartController is starting...SysID=%s", m_SysID.c_str());
}

// Second level initialization after loading configuration
/// check RF2.4 & BLE
void SmartControllerClass::InitRadio()
{
	// Set NetworkID
	while(theRadio.GetNetworkID() == 0) delay(50);

	// Check RF2.4
	if( CheckRF() ) {
  	if (IsRFGood())
  	{
  		LOGN(LOGTAG_MSG, "RF2.4 is working.");
  		SetStatus(STATUS_BMW);
  	}
  }

	// Open BLE Interface
  theBLE.Init(PIN_BLE_STATE, PIN_BLE_EN);
}

// Third level initialization after loading configuration
/// check LAN & WAN
void SmartControllerClass::InitNetwork()
{
	BOOL oldWAN = IsWANGood();
	BOOL oldLAN = IsLANGood();

	// Check WAN and LAN
	CheckNetwork();

	if (IsWANGood())
	{
		if( !oldWAN ) {	// Only log when status changed
			LOGN(LOGTAG_EVENT, "WAN is working.");
			SetStatus(STATUS_NWS);
		}

		// Initialize Logger: syslog & cloud log
		// ToDo: substitude network parameters
		//theLog.InitSysLog();
		//theLog.InitCloud();
	}
	else if (IsLANGood())
	{
		if( !oldLAN ) {	// Only log when status changed
			LOGN(LOGTAG_EVENT, "LAN is working.");
			SetStatus(STATUS_DIS);
		}
	}
	else if (IsRFGood()) {
		SetStatus(STATUS_BMW);
	}
	else {
		SetStatus(STATUS_ERR);
	}
}

// Initialize Pins: check the routine with PCB
void SmartControllerClass::InitPins()
{
	// Set Panel pin mode
#ifdef MCU_TYPE_P1
	pinMode(PIN_BTN_SETUP, INPUT);
	pinMode(PIN_BTN_RESET, INPUT);
	pinMode(PIN_LED_RED, OUTPUT);
	pinMode(PIN_LED_GREEN, OUTPUT);
	pinMode(PIN_LED_BLUE, OUTPUT);
#endif

	// Workaround for Paricle Analog Pin mode problem
#ifndef MCU_TYPE_Particle
	pinMode(PIN_BTN_UP, INPUT);
	pinMode(PIN_BTN_OK, INPUT);

	// Set Sensors pin Mode
	//pinModes are already defined in the ::begin() method of each sensor library, may need to be ommitted from here
	pinMode(PIN_SEN_DHT, INPUT);
//	pinMode(PIN_SEN_LIGHT, INPUT);
//	pinMode(PIN_SEN_MIC, INPUT);
//	pinMode(PIN_SEN_PIR, INPUT);
#endif

	// Brightness level indicator to LS138
	//pinMode(PIN_LED_LEVEL_B0, OUTPUT);
	//pinMode(PIN_LED_LEVEL_B1, OUTPUT);
	//pinMode(PIN_LED_LEVEL_B2, OUTPUT);

	// Set communication pin mode
	pinMode(PIN_BLE_RX, INPUT);
	pinMode(PIN_BLE_TX, OUTPUT);

	// Init Panel components
	thePanel.InitPanel();
	// Change Panel LED ring to indicate panel is working
	thePanel.CheckLEDRing(2);
	thePanel.CheckLEDRing(3);
}

// Initialize Sensors
void SmartControllerClass::InitSensors()
{
	// DHT
	if (theConfig.IsSensorEnabled(sensorDHT)) {
		senDHT.begin();
		LOGD(LOGTAG_MSG, "DHT sensor works.");
	}

	// Light
	/*if (theConfig.IsSensorEnabled(sensorALS)) {
		senLight.begin(SEN_LIGHT_MAX, SEN_LIGHT_MIN);	// Reversed threshold
		LOGD(LOGTAG_MSG, "Light sensor works.");
	}*/

	// Brightness indicator

	// PIR
	/*if (theConfig.IsSensorEnabled(sensorPIR)) {
		senMotion.begin();
		LOGD(LOGTAG_MSG, "Motion sensor works.");
	}*/


	// ToDo:
	//...
}

void SmartControllerClass::InitCloudObj()
{
	// Set cloud variable initial value
	m_tzString = theConfig.GetTimeZoneJSON();

	if( theConfig.GetUseCloud() != CLOUD_DISABLE ) {
		CloudObjClass::InitCloudObj();
		LOGN(LOGTAG_MSG, "Cloud Objects registered.");
	}
}

// Get the controller started
BOOL SmartControllerClass::Start()
{
	// ToDo:bring controller up along with all modules (RF, wifi, BLE)
	FindCurrentDevice();

	LOGI(LOGTAG_MSG, "SmartController started.");
	LOGI(LOGTAG_MSG, "Product Info: %s-%s-%d",
			theConfig.GetOrganization().c_str(), theConfig.GetProductName().c_str(), theConfig.GetVersion());
	LOGI(LOGTAG_MSG, "System Info: %s-%s",
			GetSysID().c_str(), GetSysVersion().c_str());

#ifndef SYS_SERIAL_DEBUG
	ResetSerialPort();
#endif

	// Change Panel LED ring to the recent level
	thePanel.SetDimmerValue(theConfig.GetBrightIndicator());

	// Sync panel with dev-st
	if( m_pMainDev ) {
		// Set panel ring on or off
		thePanel.SetRingOnOff(m_pMainDev->data.ring[0].State);
		DevSoftSwitch(m_pMainDev->data.ring[0].State, CURRENT_DEVICE);
		// Set CCT or RGBW
		if( IS_SUNNY(m_pMainDev->data.type) ) {
			// Set CCT
			thePanel.UpdateCCTValue(m_pMainDev->data.ring[0].CCT);
			ChangeLampCCT(CURRENT_DEVICE, m_pMainDev->data.ring[0].CCT);
		} else if( IS_RAINBOW(m_pMainDev->data.type) || IS_MIRAGE(m_pMainDev->data.type) ) {
			// Rainbow or Mirage
			// ToDo: set RGBW
		}
	}

	// Publish local the main device status
	if( Particle.connected() == true ) {
		UL tm = 0x4ff;	// Delay 1.5s in order to publish
		while(tm-- > 0){ Particle.process(); }
	}
	QueryDeviceStatus(CURRENT_DEVICE);

	// Request the main device to report status
	RequestDeviceStatus(CURRENT_DEVICE);

	// Acts on the Rules rules newly loaded from flash
	ReadNewRules(true);

	return true;
}

void SmartControllerClass::Restart()
{
	theConfig.SaveConfig();
	SetStatus(STATUS_RST);
	delay(1000);
	System.reset();
}

UC SmartControllerClass::GetStatus()
{
	return (UC)m_SysStatus;
}

BOOL SmartControllerClass::SetStatus(UC st)
{
	if( st > STATUS_ERR ) return false;
	LOGN(LOGTAG_STATUS, "System status changed from %d to %d", m_SysStatus, st);
	if ((UC)m_SysStatus != st) {
		m_SysStatus = st;
	}
	return true;
}

// Connect to the Cloud
BOOL SmartControllerClass::connectCloud()
{
	BOOL retVal = Particle.connected();
	if( !retVal ) {
		SERIAL("Cloud connecting...");
	  Particle.connect();
	  waitFor(Particle.connected, RTE_CLOUD_CONN_TIMEOUT);
	  retVal = Particle.connected();
		SERIAL_LN("%s", retVal ? "OK" : "Failed");
	}
  return retVal;
}

// Connect Wi-Fi
BOOL SmartControllerClass::connectWiFi()
{
	BOOL retVal = WiFi.ready();
	if( !retVal ) {
		SERIAL("Wi-Fi connecting...");
	  WiFi.connect();
	  waitFor(WiFi.ready, RTE_WIFI_CONN_TIMEOUT);
	  retVal = WiFi.ready();
		SERIAL_LN("%s", retVal ? "OK" : "Failed");
	}
  theConfig.SetWiFiStatus(retVal);
  return retVal;
}

// Close and reopen serial port to avoid buffer overrun
void SmartControllerClass::ResetSerialPort()
{
	TheSerial.end();
	TheSerial.begin(SERIALPORT_SPEED_DEFAULT);
}

BOOL SmartControllerClass::CheckRF()
{
	// RF Server begins
	m_isRF = theRadio.ServerBegin();
	if( m_isRF ) {
		// Change it if setting is not default value
		theRadio.setPALevel(theConfig.GetRFPowerLevel());
		m_isRF = theRadio.CheckConfig();
	}
	return m_isRF;
}

// Check Wi-Fi module and connection
BOOL SmartControllerClass::CheckWiFi()
{
	if( WiFi.RSSI() > 0 ) {
		m_isWAN = false;
		m_isLAN = false;
		LOGE(LOGTAG_MSG, "Wi-Fi chip error!");
		return false;
	}

	if( !WiFi.ready() ) {
		m_isWAN = false;
		m_isLAN = false;
		LOGE(LOGTAG_EVENT, "Wi-Fi module error!");
		return false;
	}
	return true;
}

BOOL SmartControllerClass::CheckNetwork()
{
	// Check Wi-Fi module
	if( !CheckWiFi() ) {
		return false;
	}

	// Check WAN
	m_isWAN = WiFi.resolve("www.google.com");

	// Check LAN if WAN is not OK
	if( !m_isWAN ) {
		m_isLAN = (WiFi.ping(WiFi.gatewayIP(), 3) < 3);
		if( !m_isLAN ) {
			LOGW(LOGTAG_MSG, "Cannot reach local gateway!");
			m_isLAN = (WiFi.ping(WiFi.localIP(), 3) < 3);
			if( !m_isLAN ) {
				LOGE(LOGTAG_MSG, "Cannot reach itself!");
			}
		}
	} else {
		m_isLAN = true;
	}

	return true;
}

BOOL SmartControllerClass::SelfCheck(US ms)
{
	static US tickSaveConfig = 0;				// must be static
	static US tickCheckRadio = 0;				// must be static
	static UC tickAcitveCheck = 0;
	static UC tickWiFiOff = 0;

	// Check all alarms. This triggers them.
	Alarm.delay(ms);

	// Save config if it was changed
	if (++tickSaveConfig > 30000 / ms) {	// once per 30 seconds
		tickSaveConfig = 0;
		theConfig.SaveConfig();
	}

  // Slow Checking: once per 60 seconds
  if (++tickCheckRadio > 60000 / ms) {
		// Check RF module
		++tickAcitveCheck;
		tickCheckRadio = 0;
    if( !IsRFGood() || !theRadio.CheckConfig() ) {
      if( CheckRF() ) {
				theRadio.switch2BaseNetwork();
				delay(10);
				theRadio.switch2MyNetwork();
        LOGN(LOGTAG_MSG, "RF24 moudle recovered.");
      }
    }

		// Check Network
		if( theConfig.GetWiFiStatus() ) {
			if( !IsWANGood() || tickAcitveCheck % 5 == 0 || GetStatus() == STATUS_DIS ) {
				InitNetwork();
			}

			if( IsWANGood() ) { // WLAN is good
				tickWiFiOff = 0;
				if( !Particle.connected() ) {
					// Cloud disconnected, try to recover
					if( theConfig.GetUseCloud() != CLOUD_DISABLE ) {
						connectCloud();
					}
				} else {
					if( theConfig.GetUseCloud() == CLOUD_DISABLE ) {
						Particle.disconnect();
					}
				}
			} else { // WLAN is wrong
				if( ++tickWiFiOff > 5 ) {
					theConfig.SetWiFiStatus(false);
					if( theConfig.GetUseCloud() == CLOUD_MUST_CONNECT ) {
						SERIAL_LN("System is about to reset due to lost of network...");
						Restart();
					} else {
						// Avoid keeping trying
						LOGE(LOGTAG_MSG, "Turn off WiFi!");
						WiFi.disconnect();
						WiFi.off();	// In order to resume Wi-Fi, restart the application
					}
				}
			}
		} else if( WiFi.ready() ) {
			theConfig.SetWiFiStatus(true);
		}

		// Daily Cloud Synchronization
		/// TimeSync
		theConfig.CloudTimeSync(false);
  }

	// Check System Status
	if( GetStatus() == STATUS_ERR ) {
		LOGE(LOGTAG_MSG, "System is about to reset due to STATUS_ERR...");
		Restart();
	}

	// ToDo:add any other potential problems to check
	//...

	return true;
}

BOOL SmartControllerClass::CheckRFBaseNetEnableDur()
{
	if( theConfig.GetMaxBaseNetworkDur() > 0 ) {
		if( theRadio.getBaseNetworkDuration() > theConfig.GetMaxBaseNetworkDur() ) {
			theRadio.enableBaseNetwork(false);
			LOGI(LOGTAG_MSG, "RF base network enable timeout, disable it automatically");
			return true;
		}
	}

	return false;
}

BOOL SmartControllerClass::IsRFGood()
{
	return (m_isRF && theRadio.isValid());
}

BOOL SmartControllerClass::IsBLEGood()
{
	return theBLE.isGood();
}

BOOL SmartControllerClass::IsLANGood()
{
	return m_isLAN;
}

BOOL SmartControllerClass::IsWANGood()
{
	return m_isWAN;
}

// Process all kinds of commands
void SmartControllerClass::ProcessCommands()
{
	// Check RF Message
	theRadio.PeekMessage();

	// Process RF2.4 messages
	theRadio.ProcessReceive();

	// Process Console Command
  theConsole.processCommand();

	// Process ASR Command
	theASR.processCommand();

	// Process BLE commands
  theBLE.processCommand();

	// Process Cloud Commands
	ProcessCloudCommands();

	// ToDo: process commands from other sources (Wifi)
	// ToDo: Potentially move ReadNewRules here
}

// Process Cloud Commands
void SmartControllerClass::ProcessCloudCommands()
{
	String _cmd;
	while( m_cmdList.size() ) {
		_cmd = m_cmdList.shift();
		ExeJSONCommand(_cmd);
	}
	while( m_configList.size() ) {
		_cmd = m_configList.shift();
		ExeJSONConfig(_cmd);
	}
}

// Collect data from all enabled sensors
/// use tick to control the collection speed of each sensor,
/// and avoid reading too many data in one loop
void SmartControllerClass::CollectData(UC tick)
{
	BOOL blnReadDHT = false;
	BOOL blnReadALS = false;
	BOOL blnReadPIR = false;

	switch (GetStatus()) {
	case STATUS_DIS:
	case STATUS_NWS:    // Normal speed
		if (theConfig.IsSensorEnabled(sensorDHT)) {
			if (tick % SEN_DHT_SPEED_NORMAL == 0)
				blnReadDHT = true;
		}
		if (theConfig.IsSensorEnabled(sensorALS)) {
			if (tick % SEN_DHT_SPEED_NORMAL == 0)
				blnReadALS = true;
		}
		if (theConfig.IsSensorEnabled(sensorPIR)) {
			if (tick % SEN_DHT_SPEED_NORMAL == 0)
				blnReadPIR = true;
		}
		break;

	case STATUS_SLP:    // Lower speed in sleep mode
		if (theConfig.IsSensorEnabled(sensorDHT)) {
			if (tick % SEN_DHT_SPEED_LOW == 0)
				blnReadDHT = true;
		}
		if (theConfig.IsSensorEnabled(sensorALS)) {
			if (tick % SEN_ALS_SPEED_LOW == 0)
				blnReadALS = true;
		}
		if (theConfig.IsSensorEnabled(sensorPIR)) {
			if (tick % SEN_PIR_SPEED_LOW == 0)
				blnReadPIR = true;
		}
		break;

	default:
		return;
	}

	// Read from DHT
	if (blnReadDHT) {
		float t = senDHT.getTempCelcius();
		float h = senDHT.getHumidity();

		if (!isnan(t)) {
			UpdateTemperature(t);
		}
		if (!isnan(h)) {
			UpdateHumidity(h);
		}
	}

	// Read from ALS
	/*if (blnReadALS) {
		UpdateBrightness(NODEID_GATEWAY, senLight.getLevel());
	}*/

	// Motion detection
	/*if (blnReadPIR) {
		UpdateMotion(NODEID_GATEWAY, senMotion.getMotion());
	}*/

	// Update json data and publish on to the cloud
	if (blnReadDHT || blnReadALS || blnReadPIR) {
		UpdateJSONData();
	}

	// ToDo: Proximity detection
	// from all channels including Wi-Fi, BLE, etc. for MAC addresses and distance to device
}

// Process panel operations, such as key press, knob rotation, etc.
bool SmartControllerClass::ProcessPanel()
{
	// Process Panel Encoder
	thePanel.ProcessEncoder();

	return true;
}

//------------------------------------------------------------------
// Device Control Functions
//------------------------------------------------------------------
// Turn the switch of specific device and all devices on or off
/// Input parameters:
///   sw: true = on; false = off
///   dev: device id or 0 (all devices under this controller)
int SmartControllerClass::DevSoftSwitch(UC sw, UC dev)
{
	//String strCmd = String::format("%d;%d;%d;%d;%d;%d", dev, S_DIMMER, C_SET, 1, V_STATUS, (sw ? 1:0));
	//ExecuteLightCommand(strCmd);

	//ToDo: if dev = 0, go through list of devices
	// ToDo:
	//SetStatus();
	String strCmd = String::format("%d:7:%d", dev, sw);
	return theRadio.ProcessSend(strCmd);
}

// High speed system timer process
void SmartControllerClass::FastProcess()
{
	// Refresh Encoder
	thePanel.EncoderAvailable();

	// ToDo:
}

//------------------------------------------------------------------
// Cloud interface implementation
//------------------------------------------------------------------
// Format: {"id":90, "offset":-300, "dst":1}
int SmartControllerClass::CldSetTimeZone(String tzStr)
{
	// Parse JSON string
	StaticJsonBuffer<COMMAND_JSON_SIZE * 8> lv_jBuf;
	JsonObject& root = lv_jBuf.parseObject(const_cast<char*>(tzStr.c_str()));
	if (!root.success())
		return -1;
	if (root.size() != 3)  // Expected 3 KVPs
		return -1;
	if(!root.containsKey("id")) return 1;
	if(!root.containsKey("offset")) return 2;
	if(!root.containsKey("dst")) return 3;

	// Set timezone id
	if (!theConfig.SetTimeZoneID((US)root["id"].as<int>()))
		return 1;

	// Set timezone offset
	if (!theConfig.SetTimeZoneOffset((SHORT)root["offset"].as<int>()))
		return 2;

	// Set timezone dst
	if (!theConfig.SetDaylightSaving((UC)root["dst"].as<int>()))
		return 3;

	LOGI(LOGTAG_EVENT, "setTimeZone to %d;%d;%d", root["id"].as<int>(), root["offset"].as<int>(), root["dst"].as<int>());
	return 0;
}

// Format:
/// 1. Date: YYYY-MM-DD
/// 2. Time: hh:mm:ss
/// 3. Date and time: YYYY-MM-DD hh:mm:ss
/// 4. "" or "sync": synchronize with could
int SmartControllerClass::CldSetCurrentTime(String tmStr)
{
	// synchronize with cloud
	tmStr.trim();
	tmStr.toLowerCase();
	if(tmStr.length() == 0 || tmStr == "sync") {
		theConfig.CloudTimeSync();
		return 0;
	}

	US _YYYY = Time.year();
	UC _MM = Time.month();
	UC _DD = Time.day();
	UC _hh = Time.hour();
	UC _mm = Time.minute();
	UC _ss = Time.second();

	int nPos, nPos2, nPos3;
	String strTime = "";
	nPos = tmStr.indexOf('-');
	if( nPos > 0 ) {
		_YYYY = (US)(tmStr.substring(0, nPos).toInt());
		if( _YYYY < 1970 || _YYYY > 2100 ) return 1;
		nPos2 = tmStr.indexOf('-', nPos + 1);
		if( nPos2 < nPos + 1 ) return 2;
		_MM = (UC)(tmStr.substring(nPos + 1, nPos2).toInt());
		if( _MM <= 0 || _MM > 12 ) return 2;
		nPos3 = tmStr.indexOf(' ', nPos2 + 1);
		if( nPos3 < nPos2 + 1 ) {
			_DD = (UC)(tmStr.substring(nPos2 + 1).toInt());
		} else {
			_DD = (UC)(tmStr.substring(nPos2 + 1, nPos3).toInt());
			strTime = tmStr.substring(nPos3 + 1);
		}
		if( _DD <= 0 || _DD > 31 ) return 3;
	} else {
		strTime = tmStr;
	}

	strTime.trim();
	nPos = strTime.indexOf(':');
	if( nPos > 0 ) {
		_hh = (UC)(strTime.substring(0, nPos).toInt());
		if( _hh > 24 ) return 4;
		nPos2 = strTime.indexOf(':', nPos + 1);
		if( nPos2 < nPos + 1 ) return 5;
		_mm = (UC)(strTime.substring(nPos + 1, nPos2).toInt());
		if( _mm > 59 ) return 5;
		_ss = (UC)(strTime.substring(nPos2 + 1).toInt());
		if( _ss > 59 ) return 6;
	}

	time_t myTime = tmConvert_t(_YYYY, _MM, _DD, _hh, _mm, _ss);
	myTime -= theConfig.GetTimeZoneDSTOffset() * 60;	// Convert to local time
	Time.setTime(myTime);
	LOGI(LOGTAG_EVENT, "setTime to %d-%d-%d %d:%d:%d", _YYYY, _MM, _DD, _hh, _mm, _ss);
	return 0;
}

// Format: "dev:sw", where
/// dev = node id, or 0 for all devices, default is 1 (the main lamp)
/// sw = 0/off or 1/on
int SmartControllerClass::CldPowerSwitch(String swStr)
{
	//fast, simple control
	UC bytDev = 1;		// Default value
	UC blnOn;
	swStr.toLowerCase();
	int nPos = swStr.indexOf(':');
	if( nPos > 0 ) {
		bytDev = (uint8_t)(swStr.substring(0, nPos).toInt());
		if( bytDev > NODEID_MAX_DEVCIE ) return 1;
	}
	String strOn = swStr.substring(nPos + 1);
	if(strOn == "0" || strOn == "off") {
		blnOn = DEVICE_SW_OFF;
	} else if(strOn == "1" || strOn == "on") {
		blnOn = DEVICE_SW_ON;
	} else {
		blnOn = DEVICE_SW_TOGGLE;
	}
	return DevSoftSwitch(blnOn, bytDev);
}

// Execute Operations, including SerialConsole commands
/// Format: {cmd: '', data: ''}
int SmartControllerClass::ExeJSONCommand(String jsonCmd)
{
	SERIAL_LN("Execute JSON cmd: %s", jsonCmd.c_str());

	int rc = ProcessJSONString(jsonCmd);
	if (rc < 0) {
		// Error input
		LOGE(LOGTAG_MSG, "Error parsing json cmd: %s", jsonCmd.c_str());
		return 0;
	} else if (rc > 0) {
		// Wait for more...
		return 1;
	}

	if (!(*m_jpCldCmd).containsKey("cmd"))
  {
		LOGE(LOGTAG_MSG, "Error json cmd format: %s", jsonCmd.c_str());
		return 0;
  }

	const COMMAND _cmd = (COMMAND)(*m_jpCldCmd)["cmd"].as<int>();

	//COMMAND 0: Use Serial Interface
	if( _cmd == CMD_SERIAL) {
		// Execute serial port command, and reflect results on cloud variable
		if (!(*m_jpCldCmd).containsKey("data")) {
			LOGE(LOGTAG_MSG, "Error json cmd format: %s", jsonCmd.c_str());
			return 0;
		}

		if( !theConfig.IsCloudSerialEnabled() ) {
			LOGN(LOGTAG_MSG, "Cloud serial command is not allowed. Check system config.");
			return 0;
		}
		const char* strData = (*m_jpCldCmd)["data"];
		theConsole.ExecuteCloudCommand(strData);
	}

	//COMMAND 1: Toggle light switch
	if (_cmd == CMD_POWER) {
		if (!(*m_jpCldCmd).containsKey("node_id") || !(*m_jpCldCmd).containsKey("state")) {
			LOGE(LOGTAG_MSG, "Error json cmd format: %s", jsonCmd.c_str());
			return 0;
		}
		const int node_id = (*m_jpCldCmd)["node_id"].as<int>();
		const int state = (*m_jpCldCmd)["state"].as<int>();

		//char buf[64];
		//sprintf(buf, "%d;%d;%d;%d;%d;%d", node_id, S_DIMMER, C_SET, 1, V_STATUS, state);
		//String strCmd(buf);
		//ExecuteLightCommand(strCmd);
		String strCmd = String::format("%d:7:%d", node_id, state);
		theRadio.ProcessSend(strCmd);
	}

	//COMMAND 2: Change light color
	if (_cmd == CMD_COLOR) {
		if (!(*m_jpCldCmd).containsKey("node_id") || !(*m_jpCldCmd).containsKey("ring")) {
			LOGE(LOGTAG_MSG, "Error json cmd format: %s", jsonCmd.c_str());
			return 0;
		}
		const int node_id = (*m_jpCldCmd)["node_id"].as<int>();
		const uint8_t ring = (*m_jpCldCmd)["ring"][0].as<uint8_t>();
		const uint8_t State = (*m_jpCldCmd)["ring"][1].as<uint8_t>();
		const uint8_t BR = (*m_jpCldCmd)["ring"][2].as<uint8_t>();
		const uint8_t W = (*m_jpCldCmd)["ring"][3].as<uint8_t>();
		const uint8_t R = (*m_jpCldCmd)["ring"][4].as<uint8_t>();
		const uint8_t G = (*m_jpCldCmd)["ring"][5].as<uint8_t>();
		const uint8_t B = (*m_jpCldCmd)["ring"][6].as<uint8_t>();

		MyMessage tmpMsg;
		UC payl_buf[MAX_PAYLOAD];
		UC payl_len;

		payl_len = CreateColorPayload(payl_buf, ring, State, BR, W, R, G, B);
		tmpMsg.build(theRadio.getAddress(), node_id, NODEID_DUMMY, C_SET, V_RGBW, true);
		tmpMsg.set((void *)payl_buf, payl_len);
		theRadio.ProcessSend(&tmpMsg);
	}

	//COMMAND 3: Change brightness
	//COMMAND 5: Change CCT
	if (_cmd == CMD_BRIGHTNESS || _cmd == CMD_CCT) {
		if (!(*m_jpCldCmd).containsKey("node_id") || !(*m_jpCldCmd).containsKey("value")) {
			LOGE(LOGTAG_MSG, "Error json cmd format: %s", jsonCmd.c_str());
			return 0;
		}
		const int node_id = (*m_jpCldCmd)["node_id"].as<int>();
		const int value = (*m_jpCldCmd)["value"].as<int>();

		//char buf[64];
		//sprintf(buf, "%d;%d;%d;%d;%d;%d", node_id, S_DIMMER, C_SET, 1, V_DIMMER, value);
		//String strCmd(buf);
		//ExecuteLightCommand(strCmd);
		// Use shortcut instead
		String strCmd = String::format("%d:%d:%d", node_id, (_cmd == CMD_BRIGHTNESS ? 9 : 11), value);
		theRadio.ProcessSend(strCmd);
	}

	//COMMAND 4: Change color with scenario input
	if (_cmd == CMD_SCENARIO) {
		if (!(*m_jpCldCmd).containsKey("node_id") || !(*m_jpCldCmd).containsKey("SNT_id")) {
			LOGE(LOGTAG_MSG, "Error json cmd format: %s", jsonCmd.c_str());
			return 0;
		}
		const int node_id = (*m_jpCldCmd)["node_id"].as<int>();
		const int SNT_uid = (*m_jpCldCmd)["SNT_id"].as<int>();

		ChangeLampScenario((UC)node_id, (UC)SNT_uid);
	}

	//COMMAND 6: Query Device Status
	if (_cmd == CMD_QUERY) {
		if (!(*m_jpCldCmd).containsKey("node_id")) {
			LOGE(LOGTAG_MSG, "Error json cmd format: %s", jsonCmd.c_str());
			return 0;
		}
		const int node_id = (*m_jpCldCmd)["node_id"].as<int>();
		UC ring_id = RING_ID_ALL;
		if( (*m_jpCldCmd).containsKey("Ring") ) {
			ring_id = (UC)(*m_jpCldCmd)["Ring"].as<int>();
			if( ring_id > MAX_RING_NUM ) ring_id = RING_ID_ALL;
		}
		QueryDeviceStatus((UC)node_id, ring_id);
	}

	return 1;
}

int SmartControllerClass::ExeJSONConfig(String jsonData) //future actions
{
  //based on the input (ie whether it is a rule, scenario, or schedule), send the json string(s) to appropriate function.
  //These functions are responsible for adding the item to the respective, appropriate Chain. If multiple json strings coming through,
  //handle each for each respective Chain until end of incoming string

	SERIAL_LN("Execute JSON config message: %s", jsonData.c_str());

  int numRows = 0;
  bool bRowsKey = true;

	int rc = ProcessJSONString(jsonData);
	if (rc < 0) {
		// Error input
		LOGE(LOGTAG_MSG, "Error parsing json config message: %s", jsonData.c_str());
		return 0;
	} else if (rc > 0) {
		// Wait for more...
		return 1;
	}

  if (!(*m_jpCldCmd).containsKey("rows"))
  {
    numRows = 1;
    bRowsKey = false;
  }
  else
  {
    numRows = (*m_jpCldCmd)["rows"].as<int>();
  }

  int successCount = 0;
  for (int j = 0; j < numRows; j++)
  {
	  if (!bRowsKey)
	  {
		  if (ParseCmdRow(*m_jpCldCmd))
		  {
			  successCount++;
		  }
	  }
	  else
	  {
		  JsonObject& data = (*m_jpCldCmd)["data"][j];
		  if (ParseCmdRow(data))
		  {
			  successCount++;
		  }
	  }
  }
  LOGI(LOGTAG_MSG, "%d of %d rows were parsed correctly", successCount, numRows);
  return successCount;
}

bool SmartControllerClass::ParseCmdRow(JsonObject& data)
{
	bool isSuccess = true;
	OP_FLAG op_flag = (OP_FLAG)data["op"].as<int>();
	FLASH_FLAG flash_flag = (FLASH_FLAG)data["fl"].as<int>();
	RUN_FLAG run_flag = (RUN_FLAG)data["run"].as<int>();

	if (op_flag < GET || op_flag > DELETE)
	{
		LOGE(LOGTAG_MSG, "UID:%s Invalid HTTP command: %d", data["uid"], op_flag);
		return 0;
	}

	if (op_flag != GET)
	{
		if (flash_flag != UNSAVED)
		{
			LOGE(LOGTAG_MSG, "UID:%s Invalid FLASH_FLAG", data["uid"]);
			return 0;
		}

		if (run_flag != UNEXECUTED)
		{
			LOGE(LOGTAG_MSG, "UID:%s Invalid RUN_FLAG", data["uid"]);
			return 0;
		}
	}

	//grab first part of uid and store it in uidKey, and convert rest of uid string into int uidNum:
	const char* uidWhole = data["uid"];
	char uidKey = tolower(uidWhole[0]);
	uint8_t uidNum = atoi(&uidWhole[1]);
	UC _cond;
	String sTemp;

	switch(uidKey)
	{
		case CLS_RULE:				//rule
		{
			RuleRow_t row;
			row.op_flag = op_flag;
			row.flash_flag = flash_flag;
			row.run_flag = run_flag;
			row.uid = uidNum;
			row.node_id = data["node_uid"];
			if( data.containsKey("SCT_uid") )
				row.SCT_uid = data["SCT_uid"];
			else row.SCT_uid = 255;
			if( data.containsKey("SNT_uid") )
				row.SNT_uid = data["SNT_uid"];
			else row.SNT_uid = 255;
			if( data.containsKey("notif_uid") )
				row.notif_uid = data["notif_uid"];
			else row.notif_uid = 255;
			if( data.containsKey("tmr_int") )
				row.tmr_int = data["tmr_int"];
			else row.tmr_int = 0;
			if( data.containsKey("tmr_span") )
				row.tmr_span = data["tmr_span"];
			else row.tmr_span = 0;
			row.tmr_started = 0;

			// Get conditions
			for( _cond = 0; _cond < MAX_CONDITION_PER_RULE; _cond++ ) {
				sTemp = String::format("cond%d", _cond);
				if( data.containsKey(sTemp) ) {
					row.actCond[_cond].enabled = data[sTemp][0];
					row.actCond[_cond].sr_scope = data[sTemp][1];
					row.actCond[_cond].symbol = data[sTemp][2];
					row.actCond[_cond].connector = data[sTemp][3];
					row.actCond[_cond].sr_id = data[sTemp][4];
					row.actCond[_cond].sr_value1 = data[sTemp][5];
					row.actCond[_cond].sr_value2 = data[sTemp][6];
				} else {
					row.actCond[_cond].enabled = false;
				}
			}

			isSuccess = Change_Rule(row);
			if (!isSuccess)
			{
				LOGE(LOGTAG_MSG, "UID:%s Unable to write row to Rule_t", uidWhole);
				return 0;
			}
			else
			{
				LOGI(LOGTAG_MSG, "UID:%s write row to Rule_t OK", uidWhole);
				return 1;
			}
			break;
		}
		case CLS_SCHEDULE: 		//schedule
		{
			ScheduleRow_t row;
			row.op_flag = op_flag;
			row.flash_flag = flash_flag;
			row.run_flag = run_flag;
			row.uid = uidNum;

			if (data["isRepeat"] == 1)
			{
				if (data["weekdays"] > 7)		// [0..7]
				{
					LOGE(LOGTAG_MSG, "UID:%s Invalid 'weekdays' must between 0 and 7", uidWhole);
					return 0;
				}
			}
			else if (data["isRepeat"] == 0)
			{
				if (data["weekdays"] < 1 || data["weekdays"] > 7)		// [1..7]
				{
					LOGE(LOGTAG_MSG, "UID:%s Invalid 'weekdays' must between 1 and 7", uidWhole);
					return 0;
				}
			}
			else
			{
				LOGE(LOGTAG_MSG, "UID:%s Invalid 'isRepeat' must 0 or 1", uidWhole);
				return 0;
			}

			if (data["hour"] < 0 || data["hour"] > 23)
			{
				LOGE(LOGTAG_MSG, "UID:%s Invalid 'hour' must between 0 and 23", uidWhole);
				return 0;
			}

			if (data["min"] < 0 || data["min"] > 59)
			{
				LOGE(LOGTAG_MSG, "UID:%s Invalid 'min' must between 0 and 59", uidWhole);
				return 0;
			}

			row.weekdays = data["weekdays"];
			row.isRepeat = data["isRepeat"];
			row.hour = data["hour"];
			row.minute = data["min"];
			row.alarm_id = dtINVALID_ALARM_ID;

			isSuccess = Change_Schedule(row);
			if (!isSuccess)
			{
				LOGE(LOGTAG_MSG, "UID:%s Unable to write row to Schedule_t", uidWhole);
				return 0;
			}
			else
			{
				LOGI(LOGTAG_MSG, "UID:%s write row to Schedule_t OK", uidWhole);
				return 1;
			}
			break;
		}
		case CLS_SCENARIO: 	//scenario
		{
			ScenarioRow_t row;
			row.op_flag = op_flag;
			row.flash_flag = flash_flag;
			row.run_flag = run_flag;
			row.uid = uidNum;

			// Power Switch
			if( data.containsKey("sw") ) {
				row.sw = data["sw"];
			} else {
				row.sw = DEVICE_SW_DUMMY;
				// Copy JSON array to Hue
				if( data.containsKey("ring0") ) {
					Array2Hue(data["ring0"], row.ring[0]);
					Array2Hue(data["ring0"], row.ring[1]);
					Array2Hue(data["ring0"], row.ring[2]);
				} else {
					if( data.containsKey("ring1") )
						Array2Hue(data["ring0"], row.ring[0]);
					if( data.containsKey("ring2") )
						Array2Hue(data["ring2"], row.ring[1]);
					if( data.containsKey("ring3") )
						Array2Hue(data["ring3"], row.ring[2]);
				}
			}

			if( data.containsKey("filter") )
				row.filter = data["filter"];

			isSuccess = Change_Scenario(row);
			if (!isSuccess)
			{
				LOGE(LOGTAG_MSG, "UID:%s Unable to write row to Scenario_t", uidWhole);
				return 0;
			}
			else
			{
				LOGI(LOGTAG_MSG, "UID:%s write row to Scenario_t OK", uidWhole);
				return 1;
			}
			break;
		}
		case CLS_LIGHT_STATUS:		// Node information
		{
			// Query Device Status and feedback through event
			if( op_flag == GET ) {
				// Note: here we use uid as node_id
				/// if node_id is 0, return node list
				theConfig.lstNodes.showList(true, uidNum);
			}
			break;
		}
		case CLS_CONFIGURATION:		// Sys Config
		{
			if( op_flag == PUT ) {
				if( data.containsKey("csc") ) {
					// Change CSC
					theConfig.SetCloudSerialEnabled(data["csc"] > 0);
				} else if( data.containsKey("node_id") ) {
					UC node_id = (UC)data["node_id"];
					if( data.containsKey("new_id") ) {
						UC new_id = (UC)data["new_id"];
						String strCmd = String::format("%d:1:%d", node_id, new_id);
						theRadio.ProcessSend(strCmd);
						LOGN(LOGTAG_MSG, "Change nodeid:%d to %d", node_id, new_id);
					} else if( data.containsKey("ncf") && data.containsKey("value") ) {
						UC _config = (UC)data["ncf"];
						US _value = (US)data["value"];
						if( _config == NCF_DEV_ASSOCIATE ) {
							theConfig.SetRemoteNodeDevice(node_id, _value);
						} else {
							theRadio.SendNodeConfig(node_id, _config, _value);
						}
						LOGN(LOGTAG_MSG, "Set nodeid:%d config %d to %d", node_id, _config, _value);
					}
				} else if( data.containsKey("asrcmd") && data.containsKey("SNT_id") ) {
					theConfig.SetASR_SNT((UC)data["asrcmd"], (UC)data["SNT_id"]);
				}
			}
			// ToDo: more config
			break;
		}
		default:
		{
			LOGE(LOGTAG_MSG, "UID:%s Invalid", uidWhole);
		}
	}

	return 0;
}
//------------------------------------------------------------------
// Cloud Interface Action Types
//------------------------------------------------------------------

bool SmartControllerClass::Change_Rule(RuleRow_t row)
{
	int index;
	switch (row.op_flag)
	{
		case DELETE:
			//search rules table for uid
			index = Rule_table.search_uid(row.uid);
			if (index == -1) //uid not found
			{
				//add row
				if (!Rule_table.add(row))
				{
					LOGE(LOGTAG_MSG, "Error occured while adding Rule UID:%c%d", CLS_RULE, row.uid);
					return false;
				}
			}
			else //uid found
			{
				//update row
				if (!Rule_table.set(index, row))
				{
					LOGE(LOGTAG_MSG, "Error occured while updating Rule UID:%c%d", CLS_RULE, row.uid);
					return false;
				}
			}
			break;

		case POST:
		case PUT:
			//search rule table for uid
			index = Rule_table.search_uid(row.uid);
			if (index == -1) //uid not found
			{
				//add row
				if (!Rule_table.add(row))
				{
					LOGE(LOGTAG_MSG, "Error occured while adding Rule UID:%c%d", CLS_RULE, row.uid);
					return false;
				}

				if (row.op_flag == PUT)
				{
					LOGN(LOGTAG_MSG, "PUT found no updatable, Rule added UID:%c%d", CLS_RULE, row.uid);
				}
			}
			else //uid found
			{
				//update row
				if (!Rule_table.set(index, row))
				{
					LOGE(LOGTAG_MSG, "Error occured while updating Rule UID:%c%d", CLS_RULE, row.uid);
					return false;
				}

				if (row.op_flag == POST)
				{
					LOGN(LOGTAG_MSG, "POST found duplicate row, overwriting UID:%c%d", CLS_RULE, row.uid);
				}
			}
			break;
	}
	theConfig.SetRTChanged(true);
	return true;
}

bool SmartControllerClass::Change_Schedule(ScheduleRow_t row)
{
	int index;
	switch (row.op_flag)
	{
		case DELETE:
			//search schedule table for uid
			index = Schedule_table.search_uid(row.uid);
			if (index == -1) //uid not found
			{
				//make room for new row
				if (Schedule_table.isFull())
				{
					if (!Schedule_table.delete_one_outdated_row())
					{
						LOGW(LOGTAG_MSG, "Schedule_t full, cannot process command");
						return false;
					}
				}

				//add row
				if (!Schedule_table.add(row))
				{
					LOGE(LOGTAG_MSG, "Error occured while adding Schedule UID:%c%d", CLS_SCHEDULE, row.uid);
					return false;
				}

			}
			else //uid found
			{
				//bring over old alarm id to new row if it exists
				if (Schedule_table.get(index).run_flag == EXECUTED && Alarm.isAllocated(Schedule_table.get(index).alarm_id))
				{
					//copy old alarm id into new row
					row.alarm_id = Schedule_table.get(index).alarm_id;
				}
				else
				{
					LOGN(LOGTAG_MSG, "Previous alarm not set, replacing Schedule UID:%c%d", CLS_SCHEDULE, row.uid);
				}

				//update row
				if (!Schedule_table.set(index, row))
				{
					LOGE(LOGTAG_MSG, "Error occured while updating Schedule UID:%c%d", CLS_SCHEDULE, row.uid);
					return false;
				}
			}
			break;

		case POST:
		case PUT:
			//search schedule table for uid
			index = Schedule_table.search_uid(row.uid);
			if (index == -1) //uid not found
			{
				//make room for new row
				if (Schedule_table.isFull())
				{
					if (!Schedule_table.delete_one_outdated_row())
					{
						LOGW(LOGTAG_MSG, "Schedule_t full, cannot process command");
						return false;
					}
				}

				//add row
				if (!Schedule_table.add(row))
				{
					LOGE(LOGTAG_MSG, "Error occured while adding Schedule UID:%c%d", CLS_SCHEDULE, row.uid);
					return false;
				}

				if (row.op_flag == PUT)
				{
					LOGN(LOGTAG_MSG, "PUT found no updatable, Schedule added UID:%c%d", CLS_SCHEDULE, row.uid);
				}
			}
			else //uid found
			{
				//bring over old alarm id to new row if it exists
				if (Schedule_table.get(index).run_flag == EXECUTED && Alarm.isAllocated(Schedule_table.get(index).alarm_id))
				{
					//copy old alarm id into new row
					row.alarm_id = Schedule_table.get(index).alarm_id;
				}
				else
				{
					LOGN(LOGTAG_MSG, "Previous alarm not set, replacing Schedule UID:%c%d", CLS_SCHEDULE, row.uid);
				}

				//update row
				if (!Schedule_table.set(index, row))
				{
					LOGE(LOGTAG_MSG, "Error occured while updating Schedule UID:%c%d", CLS_SCHEDULE, row.uid);
					return false;
				}

				if (row.op_flag == POST)
				{
					LOGN(LOGTAG_MSG, "POST found duplicate row, overwriting UID:%c%d", CLS_SCHEDULE, row.uid);
				}
			}
			break;
	}
	theConfig.SetSCTChanged(true);
	return true;
}

bool SmartControllerClass::Change_Scenario(ScenarioRow_t row)
{
	int index;
	switch (row.op_flag)
	{
		case DELETE:
			//search scenario table for uid
			index = Scenario_table.search_uid(row.uid);
			if (index == -1) //uid not found
			{
				//make room for new row
				if (Scenario_table.isFull())
				{
					if (!Scenario_table.delete_one_outdated_row())
					{
						LOGW(LOGTAG_MSG, "Scenario_t full, cannot process command");
						return false;
					}
				}

				//add row
				if (!Scenario_table.add(row))
				{
					LOGE(LOGTAG_MSG, "Error occured while adding Scenario UID:%c%d", CLS_SCENARIO, row.uid);
					return false;
				}
			}
			else //uid found
			{
				//update row
				if (!Scenario_table.set(index, row))
				{
					LOGE(LOGTAG_MSG, "Error occured while updating Scenario UID:%c%d", CLS_SCENARIO, row.uid);
					return false;
				}
			}
			break;

		case POST:
		case PUT:
			//search scenario table for uid
			index = Scenario_table.search_uid(row.uid);
			if (index == -1) //uid not found
			{
				//make room for new row
				if (Scenario_table.isFull())
				{
					if (!Scenario_table.delete_one_outdated_row())
					{
						LOGW(LOGTAG_MSG, "Scenario_t full, cannot process command");
						return false;
					}
				}

				//add row
				if (!Scenario_table.add(row))
				{
					LOGE(LOGTAG_MSG, "Error occured while adding Scenario UID:%c%d", CLS_SCENARIO, row.uid);
					return false;
				}

				if (row.op_flag == PUT)
				{
					LOGN(LOGTAG_MSG, "PUT found no updatable, Scenario added UID:%c%d", CLS_SCENARIO, row.uid);
				}
			}
			else //uid found
			{
				//update row
				if (!Scenario_table.set(index, row))
				{
					LOGE(LOGTAG_MSG, "Error occured while updating Scenario UID:%c%d", CLS_SCENARIO, row.uid);
					return false;
				}

				if (row.op_flag == POST)
				{
					LOGN(LOGTAG_MSG, "POST found duplicate row, overwriting UID:%c%d", CLS_SCENARIO, row.uid);
				}
			}
			break;
	}
	theConfig.SetSNTChanged(true);
	return true;
}

bool SmartControllerClass::updateDevStatusRow(MyMessage msg)
{
	//find row to update
	ListNode<DevStatusRow_t> *DevStatusRowPtr = SearchDevStatus(msg.getDestination());
	if (DevStatusRowPtr == NULL)
	{
		LOGE(LOGTAG_MSG, "Error updating DevStatus_Table, didn't find node_id %d", msg.getDestination());
		return false;
	}

	switch (msg.getSensor()) //current possible values are S_CUSTOM and S_DIMMER
	{
	case S_CUSTOM:
		if (msg.getType() == V_VAR1)
		{
			/*
			uint64_t payload = msg.getUInt64();

			uint8_t ring_num = (payload >> (8 * 6)) & 0xff;
			Hue_t ring_col;

			ring_col.State = (payload >> (8 * 5)) & 0xff;
			ring_col.CW= (payload >> (8 * 4)) & 0xff;
			ring_col.WW = (payload >> (8 * 3)) & 0xff;
			ring_col.R = (payload >> (8 * 2)) & 0xff;
			ring_col.G = (payload >> (8 * 1)) & 0xff;
			ring_col.B = (payload >> (8 * 0)) & 0xff;*/
			uint8_t *payload = (uint8_t *)msg.getCustom();
			uint8_t ring_num = payload[0];
			Hue_t ring_col;
			memcpy(&ring_col, payload+1, sizeof(Hue_t));

			switch (ring_num)
			{
			case 0: //all rings
				if (ring_col.State == 0) { //if ring_num and state both equal 0, do not re-write colors, only turn rings off
					DevStatusRowPtr->data.ring[0].State = 0;
					DevStatusRowPtr->data.ring[1].State = 0;
					DevStatusRowPtr->data.ring[2].State = 0;
				} else {
					DevStatusRowPtr->data.ring[0] = ring_col;
					DevStatusRowPtr->data.ring[1] = ring_col;
					DevStatusRowPtr->data.ring[2] = ring_col;
				}
				break;

			case 1:
			case 2:
			case 3:
				if (ring_col.State == 0) { //if state=0 don't change colors
					DevStatusRowPtr->data.ring[ring_num-1].State = 0;
				} else {
					DevStatusRowPtr->data.ring[ring_num-1] = ring_col;
				}
				break;
			}
		}
		else
		{
			LOGE(LOGTAG_MSG, "Error updating DevStatus_Table, invalid subtype for S_CUSTOM sensor");
			return false;
		}
		break;

	case S_DIMMER:
		if (msg.getType() == V_STATUS) {
			if (msg.getBool())
			{
				DevStatusRowPtr->data.ring[0].State = 1;
				DevStatusRowPtr->data.ring[1].State = 1;
				DevStatusRowPtr->data.ring[2].State = 1;
			}
			else
			{
				DevStatusRowPtr->data.ring[0].State = 0;
				DevStatusRowPtr->data.ring[1].State = 0;
				DevStatusRowPtr->data.ring[2].State = 0;
			}
		}
		else if (msg.getType() == V_DIMMER) {
			//do nothing, DevStatusRow_t has no brightness attribute to update
		}
		else {
			LOGE(LOGTAG_MSG, "Error updating DevStatus_Table, invalid subtype for S_DIMMER sensor");
			return false;
		}
		break;

	default:
		LOGE(LOGTAG_MSG, "Error updating DevStatus_Table, invalid MyMessage sensor type");
		return false;
	}

	// ToDo: This (below) causes all changed DevStatus rows to be written to memory during SaveConfig().
	// This is very bad for the flash memory in the long run, as the memory will wear down each time the
	// user turns on or off the light
	DevStatusRowPtr->data.flash_flag = UNSAVED; //required
	DevStatusRowPtr->data.run_flag = EXECUTED; //redundant, already should be EXECUTED
	theConfig.SetDSTChanged(true);

	return true;
}

//This function takes in a MyMessage serial input, and sends it to the specified light.
//Upon success, also updates the devstatus table and brightness indicator
/*
bool SmartControllerClass::ExecuteLightCommand(String mySerialStr)
{
	//TESTING SAMPLES
	// Set S_CUSTOM:
	// all rings red: 1;23;1;1;24;1099528339456
	// all rings CW/WW: 1;23;1;1;24;2199006478336
	// all rings off: 1;23;1;1;24;16777215
	// ring 1 red: 1;23;1;1;24;282574505050112
	// Set S_DIMMER, V_STATUS on/off
	// 	1;4;1;1;2;1
	// Set S_DIMMER, V_DIMMER value 0-100
	//  1;4;1;1;3;50

	SERIAL_LN("Attempting to send MyMessage Serial string: %s", mySerialStr.c_str());

	//mysensors serial message
	MyMessage msg;
	MyParserSerial lv_SP;
	if( lv_SP.parse(msg, mySerialStr.c_str()) ) {
		if (theRadio.ProcessSend(msg)) //send message
		{
			SERIAL_LN("Sent message: from:%d dest:%d cmd:%d type:%d sensor:%d payl-len:%d",
				msg.getSender(), msg.getDestination(), msg.getCommand(),
				msg.getType(), msg.getSensor(), msg.getLength());

			if (updateDevStatusRow(msg)) //update devstatus;
			{
				//ToDo: update brightness indicator
				return true;
			}
		}
	}
	return false;
}
*/

// Match sensor data to condition
bool SmartControllerClass::Check_SensorData(UC _scope, UC _sr, UC _symbol, US _val1, US _val2)
{
	// Retrieve sensor data
	US senData = 0;
	switch( _scope ) {
		case SR_SCOPE_CONTROLLER:
		case SR_SCOPE_NODE:
		// ToDo: should distinguish node and more sensors
		if( _sr == sensorDHT ) {
			senData = (US)(m_temperature + 0.5);
		} else if( _sr == sensorDHT_h ) {
			senData = (US)(m_humidity + 0.5);
		} else if( _sr == sensorALS ) {
			senData = m_brightness;
		} else if( _sr == sensorPIR ) {
			senData = m_motion;
		} else if( _sr == sensorSMOKE ) {
			senData = m_smoke;
		} else if( _sr == sensorGAS ) {
			senData = m_gas;
		} else if( _sr == sensorDUST ) {
			senData = m_dust;
		}
		break;

		case SR_SCOPE_ANY:
		// ToDo:
		break;

		case SR_SCOPE_GROUP:
		// ToDo:
		break;

		default:
		return false;
	}

	// Assert value
	bool rc = false;
	switch( _symbol ) {
		case SR_SYM_EQ:
		rc = (senData == _val1);
		break;

		case SR_SYM_NE:
		rc = (senData != _val1);
		break;

		case SR_SYM_GT:
		rc = (senData > _val1);
		break;

		case SR_SYM_GE:
		rc = (senData >= _val1);
		break;

		case SR_SYM_LT:
		rc = (senData < _val1);
		break;

		case SR_SYM_LE:
		rc = (senData <= _val1);
		break;

		case SR_SYM_BW:
		rc = (senData >= _val1 && senData <= _val2);
		break;

		case SR_SYM_NB:
		rc = (senData < _val1 || senData > _val2);
		break;

		default:
		return false;
	}
	return rc;
}

// Execute Rule, called by Action_Rule(), AlarmTimerTriggered() and OnSensorDataChanged()
bool SmartControllerClass::Execute_Rule(ListNode<RuleRow_t> *rulePtr, bool _init, UC _sr)
{
	// Whether execute
	if( _init ) {
		// Start timer
		if( rulePtr->data.tmr_int ) {
			rulePtr->data.tmr_tic_start = millis();
			rulePtr->data.tmr_started = 1;
		}
	} else {
		// Check timer
		if( !rulePtr->data.tmr_started ) return false;
		if( rulePtr->data.tmr_span > 0 ) {
			// contintue or stop
			if( (millis() - rulePtr->data.tmr_tic_start) / 60000 > rulePtr->data.tmr_span ) {
				// Time out and stop
				rulePtr->data.tmr_started = 0;
				return false;
			}
		}
	}

	UC _cond;
	bool bTrigger = false;
	// Whether conditions contain this sensor
	if( _sr < 255 ) {
		for(_cond = 0; _cond < MAX_CONDITION_PER_RULE; _cond++ ) {
			if( !rulePtr->data.actCond[_cond].enabled ) return false;
			if( rulePtr->data.actCond[_cond].sr_id == _sr ) {
				// Go on check conditions
				bTrigger = true;
				break;
			}
		}
		// no this sensor
		if( !bTrigger ) return false;
	} else {
		bTrigger = true;
	}

	// Check conditions
	bool bTest;
	UC _connector = COND_SYM_NOT;
	for(_cond = 0; _cond < MAX_CONDITION_PER_RULE; _cond++ ) {
		if( !rulePtr->data.actCond[_cond].enabled ) break;

		bTest = Check_SensorData(rulePtr->data.actCond[_cond].sr_scope, rulePtr->data.actCond[_cond].sr_id,
				rulePtr->data.actCond[_cond].symbol, rulePtr->data.actCond[_cond].sr_value1, rulePtr->data.actCond[_cond].sr_value2);

		if( _connector != COND_SYM_NOT ) {
			if( _connector == COND_SYM_OR ) {
				bTrigger |= bTest;
			} else if( _connector == COND_SYM_AND ) {
				bTrigger &= bTest;
			}
		} else {
			bTrigger = bTest;
		}
		_connector = rulePtr->data.actCond[_cond].connector;
		// Exit earlier
		if( bTrigger && _connector == COND_SYM_OR ) break;
		if( !bTrigger && _connector == COND_SYM_AND ) break;
	}

	// Switch to desired scenario
	if( bTrigger ) {
		LOGI(LOGTAG_EVENT, "Rule %d triggered by sensor %d", rulePtr->data.uid, _sr);
		ChangeLampScenario(rulePtr->data.node_id, rulePtr->data.SNT_uid);

		// Send Notification
		if( rulePtr->data.notif_uid < 255 && Particle.connected() ) {
			StaticJsonBuffer<256> jBuf;
			JsonObject *jroot;
			jroot = &(jBuf.createObject());
			if( jroot->success() ) {
				(*jroot)["notif"] = rulePtr->data.notif_uid;
				(*jroot)["rule"] = rulePtr->data.uid;
				(*jroot)["nd"] = rulePtr->data.node_id;
				(*jroot)["snt"] = rulePtr->data.SNT_uid;
				char buffer[256];
				jroot->printTo(buffer, 256);
				PublishAlarm(buffer);
			}
		}
	}

	return bTrigger;
}

//------------------------------------------------------------------
// Acting on new rows in working memory Chains
//------------------------------------------------------------------
// Scan Rule list and create associated objectss, such as Schedule (Alarm), Scenario, etc.
void SmartControllerClass::ReadNewRules(bool force)
{
	if (theConfig.IsRTChanged() || force)
	{
		ListNode<RuleRow_t> *ruleRowPtr = Rule_table.getRoot();
		while (ruleRowPtr != NULL)
		{
			Action_Rule(ruleRowPtr);
			ruleRowPtr = ruleRowPtr->next;
		} //end of loop
	}
}

// Scan rule list and check conditions in accordance with changed sensor
void SmartControllerClass::OnSensorDataChanged(UC _sr)
{
	ListNode<RuleRow_t> *ruleRowPtr = Rule_table.getRoot();
	while (ruleRowPtr != NULL)
	{
		// Execute the rule with changed sensor
		Execute_Rule(ruleRowPtr, false, _sr);
		ruleRowPtr = ruleRowPtr->next;
	} //end of loop
}

bool SmartControllerClass::CreateAlarm(ListNode<ScheduleRow_t>* scheduleRow, uint32_t tag)
{
	//Use weekday, isRepeat, hour, min information to create appropriate alarm
	//The created alarm obj must have a field for rule_id so it has a reference to what created it

	//If isRepeat is 1 AND:
		//if weekdays value is between 1 and 7, the alarm is to be repeated weekly on the specified weekday
		//if weekdays value is 0, alarm is to be repeated daily
	//if isRepeat is 0:
		//alarm is to to be triggered on specified weekday; weekday cannot be 0.

	AlarmId alarm_id;
	if (scheduleRow->data.isRepeat == 1)
	{
		if (scheduleRow->data.weekdays > 0 && scheduleRow->data.weekdays <= 7) {
			//repeat weekly on given weekday
			alarm_id = Alarm.alarmRepeat((timeDayOfWeek_t)(int)scheduleRow->data.weekdays, (int)scheduleRow->data.hour, (int)scheduleRow->data.minute, 0, AlarmTimerTriggered);
			Alarm.setAlarmTag(alarm_id, tag);
		}
		else if (scheduleRow->data.weekdays == 0)
		{
			//weekdays == 0 refers to daily repeat
			time_t tmValue = AlarmHMS((int)scheduleRow->data.hour, (int)scheduleRow->data.minute, 0);
			alarm_id = Alarm.alarmRepeat(tmValue, AlarmTimerTriggered);
			Alarm.setAlarmTag(alarm_id, tag);
		}
		else
		{
			LOGN(LOGTAG_MSG, "Cannot create Alarm via UID:%c%d. Incorrect weekday value.", CLS_RULE, tag);
			return false;
		}
	}
	else if (scheduleRow->data.isRepeat == 0) {
		alarm_id = Alarm.alarmOnce((timeDayOfWeek_t)(int)scheduleRow->data.weekdays, (int)scheduleRow->data.hour, (int)scheduleRow->data.minute, 0, AlarmTimerTriggered);
		Alarm.setAlarmTag(alarm_id, tag);
	}
	else {
		LOGN(LOGTAG_MSG, "Cannot create Alarm via UID:%c%d. Incorrect isRepeat value.", CLS_RULE, tag);
		return false;
	}
	//Update that schedule row's alarm_id field with the newly created alarm's alarm_id
	scheduleRow->data.alarm_id = alarm_id;
	LOGI(LOGTAG_MSG, "Alarm %u created via UID:%c%d", alarm_id, CLS_RULE, tag);
	LOGD(LOGTAG_MSG, "Alarm %u creation info:", alarm_id);
	LOGD(LOGTAG_MSG, "-----weekday %d", (int)scheduleRow->data.weekdays);
	LOGD(LOGTAG_MSG, "-----hour %d", (int)scheduleRow->data.hour);
	LOGD(LOGTAG_MSG, "-----min %d", (int)scheduleRow->data.minute);

	return true;
}

bool SmartControllerClass::DestoryAlarm(AlarmId alarmID, UC SCT_uid)
{
	if(!Alarm.isAllocated(alarmID))
		return false;

	Alarm.disable(alarmID);
	Alarm.free(alarmID);
	LOGN(LOGTAG_MSG, "Destory alarm via UID:%c%d", CLS_SCHEDULE, SCT_uid);

	return true;
	//Note: Remember to always follow up this function call with setting the AlarmID to dtINVALID_ALARM_ID
}

bool SmartControllerClass::Action_Rule(ListNode<RuleRow_t> *rulePtr)
{
	if(!rulePtr)
		return false;

	if (rulePtr->data.run_flag == UNEXECUTED)
	{
		// Process Schedule: install alarm trigger
		if( rulePtr->data.SCT_uid < 255 ) {
			// Schedule will take over the rule, and exectue the rule later
			Action_Schedule(rulePtr->data.op_flag, rulePtr->data.SCT_uid, rulePtr->data.uid);
		} else {
			// Execute the rule with init-flag
			Execute_Rule(rulePtr, true);
		}

		// Process Conditions: install sensor triggers
		// ToDo: currently we scan all rules on sensor data changed by calling OnSensorDataChanged(),
		/// but in the future, we may need to install handlers to skim down processing

		rulePtr->data.run_flag = EXECUTED;
		theConfig.SetRTChanged(true);
	}

	return true;
}

bool SmartControllerClass::Action_Schedule(OP_FLAG parentFlag, UC uid, UC rule_uid)
{
	bool retVal = true;

	//get SCT_uid from rule rows and find SCT_uid in SCT LinkedList
	ListNode<ScheduleRow_t> *scheduleRow = SearchSchedule(uid);
	if (scheduleRow) //found schedule row in chain or Flash
	{
		if (scheduleRow->data.run_flag == UNEXECUTED)
		{
			switch(scheduleRow->data.op_flag)
			{
			case POST:
			case PUT:
				if(parentFlag == POST || parentFlag == PUT)
				{
					// Recreate Alarm
					DestoryAlarm(scheduleRow->data.alarm_id, uid);
					scheduleRow->data.alarm_id = dtINVALID_ALARM_ID;
					CreateAlarm(scheduleRow, rule_uid);
				}
				break;

			case DELETE:
				// Delete Alarm
				LOGI(LOGTAG_MSG, "Found to be deleted object UID:%c%d", CLS_SCHEDULE, uid);
				DestoryAlarm(scheduleRow->data.alarm_id, uid);
				scheduleRow->data.alarm_id = dtINVALID_ALARM_ID;
				break;

			case GET:			// ToDo:...
					break;
			}
		}
		else	// EXECUTED, double check
		{
			if(parentFlag != DELETE && parentFlag != GET && scheduleRow->data.op_flag != DELETE)
			{
				// The alarm is supposed to exist, otherwise create it
				if (!Alarm.isAllocated(scheduleRow->data.alarm_id))
				{
					CreateAlarm(scheduleRow, rule_uid);
				}
			}
		}

		// Parent Flag Special Check
		if( parentFlag == DELETE ) {
			// Destory Alarm if exists
			DestoryAlarm(scheduleRow->data.alarm_id, uid);
			scheduleRow->data.alarm_id = dtINVALID_ALARM_ID;
		}

		// Set flag anyway
		scheduleRow->data.run_flag = EXECUTED;
		theConfig.SetSCTChanged(true);
	}
	else //schedule row not found in chain or Flash
	{
		LOGW(LOGTAG_MSG, "Cannot find object with UID:%c%d", CLS_SCHEDULE, uid);
		retVal = false;
	}

	return retVal;
}

//------------------------------------------------------------------
// Table Search Functions (Search chain, then Flash)
//------------------------------------------------------------------
ListNode<ScheduleRow_t> *SmartControllerClass::SearchSchedule(UC uid)
{
	//search chain in working memory
	ListNode<ScheduleRow_t> *pObj = Schedule_table.search(uid);

	if(!pObj)
	{
		// Search Flash and validate data entry
		ScheduleRow_t row;
		if (uid < MAX_SCT_ROWS)
		{
			// Find it
			EEPROM.get(MEM_SCHEDULE_OFFSET + uid*SCT_ROW_SIZE, row);

			// flags should be 111
			if(row.uid == uid && row.op_flag == (OP_FLAG)1
							  && row.flash_flag == (FLASH_FLAG)1
							  && row.run_flag == (RUN_FLAG)1)
			{
				// Copy data entry to Working Memory and get the pointer
				row.op_flag = POST;				// op_flag should already be POST==(OP_FLAG)1
				row.run_flag = UNEXECUTED;		// need to create Alarm later
				row.flash_flag = SAVED;			// We know it has a copy in flash
				if (Change_Schedule(row))
				{
					//pObj = Schedule_table.search(uid);
					pObj = Schedule_table.getLast();		// Faster
					LOGN(LOGTAG_MSG, "UID:%c%d copy Flash to Schedule_t OK", CLS_SCHEDULE, uid);
				}
				else
				{
					LOGE(LOGTAG_MSG, "UID:%c%d Unable to copy Flash to Schedule_t", CLS_SCHEDULE, uid);
				}
			}
		}
	}

	return pObj;
}

ListNode<ScenarioRow_t>* SmartControllerClass::SearchScenario(UC uid)
{
	ListNode<ScenarioRow_t> *pObj = Scenario_table.search(uid); //search chain

	if (!pObj) //not found in working memory
	{
		//search Flash and validate data entry
		ScenarioRow_t row;
		if (uid < MAX_SNT_ROWS)
		{
			//find it
			theConfig.MemReadScenarioRow(row, MEM_SCENARIOS_OFFSET + uid*SNT_ROW_SIZE);

			//flags should be 111
			if (row.uid == uid && row.op_flag == (OP_FLAG)1
							   && row.flash_flag == (FLASH_FLAG)1
							   && row.run_flag == (RUN_FLAG)1)
			{
				// Copy data entry into working memory and get the pointer
				row.op_flag = POST;
				row.run_flag = UNEXECUTED;
				row.flash_flag = SAVED;			//we know it has a copy in flash
				if (Change_Scenario(row))
				{
					pObj = Scenario_table.getLast();
					LOGN(LOGTAG_MSG, "UID:%c%d copy Flash to Scenario_t OK", CLS_SCENARIO, uid);
				}
				else
				{
					LOGE(LOGTAG_MSG, "UID:%c%d Unable to copy Flash to Scenario_t", CLS_SCENARIO, uid);
				}
			}
		}
	}

	return pObj;
}

ListNode<DevStatusRow_t>* SmartControllerClass::SearchDevStatus(UC dest_id)
{
	ListNode<DevStatusRow_t> *tmp = DevStatus_table.getRoot();
	while (tmp != NULL)
	{
		if (tmp->data.node_id == dest_id) {
			return tmp;
		}
		tmp = tmp->next;
	}
	//do not need to search in flash because whole table is always loaded

	return NULL;
}

//------------------------------------------------------------------
// Device Operations
//------------------------------------------------------------------
BOOL SmartControllerClass::FindCurrentDevice()
{
	if( CURRENT_DEVICE == NODEID_DUMMY ) {
		m_pMainDev = SearchDevStatus(NODEID_MAINDEVICE);
	} else {
		m_pMainDev = SearchDevStatus(CURRENT_DEVICE);
	}
	return(m_pMainDev != NULL);
}

ListNode<DevStatusRow_t> *SmartControllerClass::FindDevice(UC _nodeID)
{
	ListNode<DevStatusRow_t> *DevStatusRowPtr = NULL;
	if( IS_CURRENT_DEVICE(_nodeID) ) {
		DevStatusRowPtr = m_pMainDev;
	} else {
		DevStatusRowPtr = SearchDevStatus(_nodeID);
	}
	return DevStatusRowPtr;
}

UC SmartControllerClass::GetDevOnOff(UC _nodeID)
{
	//(m_pMainDev->data.ring[0].BR < BR_MIN_VALUE ? true : !m_pMainDev->data.ring[0].State);
	return( thePanel.GetRingOnOff() ? DEVICE_SW_ON : DEVICE_SW_OFF);
}

UC SmartControllerClass::GetDevBrightness(UC _nodeID)
{
	UC _br = (UC)thePanel.GetDimmerValue();
//	ListNode<DevStatusRow_t> *DevStatusRowPtr = FindDevice(_nodeID);
//	if (DevStatusRowPtr) {
//		_br = DevStatusRowPtr->data.ring[0].BR;
//	}
	return _br;
}

US SmartControllerClass::GetDevCCT(UC _nodeID)
{
	return (US)thePanel.GetCCTValue();
}

US SmartControllerClass::VerifyDevicePresence(UC *_assoDev, UC _nodeID, UC _devType, uint64_t _identity)
{
	NodeIdRow_t lv_Node;
	// Veirfy identity
	lv_Node.nid = _nodeID;
	if( theConfig.lstNodes.get(&lv_Node) < 0 ) return 0;
	if( isIdentityEmpty(lv_Node.identity) || !isIdentityEqual(lv_Node.identity, &_identity) ) {
		LOGN(LOGTAG_MSG, "Failed to verify identity for device:%d", _nodeID);
		return 0;
	}
	// Update timestamp
	lv_Node.recentActive = Time.now();
	theConfig.lstNodes.update(&lv_Node);
	theConfig.SetNIDChanged(true);
	*_assoDev = lv_Node.device;

	US token = random(65535); // Random number
	if( _nodeID < NODEID_MIN_REMOTE ) {
		// Lamp
		// Search device in Device Status Table, add new item if not exists,
		// since the node has passed verification
		ListNode<DevStatusRow_t> *DevStatusRowPtr = SearchDevStatus(_nodeID);
		if (!DevStatusRowPtr) {
			if( theConfig.InitDevStatus(_nodeID) ) {
				theConfig.SetDSTChanged(true);
				DevStatusRowPtr = SearchDevStatus(_nodeID);
				//ToDo: remove
				//SERIAL_LN("Test DevStatus_table2 added item: %d, size: %d", _nodeID, theSys.DevStatus_table.size());
			}
		}
		if(!DevStatusRowPtr) {
			LOGW(LOGTAG_MSG, "Failed to get item form DST for device:%d", _nodeID);
			return 0;
		}

		// Update DST item
		DevStatusRowPtr->data.present = 1;
		DevStatusRowPtr->data.type = _devType;
		DevStatusRowPtr->data.token = token;
		DevStatusRowPtr->data.flash_flag = UNSAVED; //required
		DevStatusRowPtr->data.run_flag = EXECUTED; //redundant, already should be EXECUTED
		theConfig.SetDSTChanged(true);
	} else {
		// Remote
		theConfig.m_stMainRemote.node_id = _nodeID;
		theConfig.m_stMainRemote.present = 1;
		theConfig.m_stMainRemote.type = 1;
		theConfig.m_stMainRemote.token = token;
	}

	return token;
}

BOOL SmartControllerClass::ToggleLampOnOff(UC _nodeID)
{
	BOOL rc = false, _st;
	ListNode<DevStatusRow_t> *DevStatusRowPtr;
	if( IS_CURRENT_DEVICE(_nodeID) ) {
		DevStatusRowPtr = m_pMainDev;
	} else {
		DevStatusRowPtr = SearchDevStatus(_nodeID);
	}
	if (DevStatusRowPtr) {
		_st = (DevStatusRowPtr->data.ring[0].BR < BR_MIN_VALUE ? true : !DevStatusRowPtr->data.ring[0].State);
		rc = DevSoftSwitch(_st, _nodeID);
		// Wait for confirmation or not
		if( !rc ) {
			// no need to wait
			ConfirmLampOnOff(_nodeID, _st);
		}
	}
	return rc;
}

BOOL SmartControllerClass::ChangeLampBrightness(UC _nodeID, UC _percentage)
{
	BOOL rc = false;
	//ListNode<DevStatusRow_t> *DevStatusRowPtr = SearchDevStatus(_nodeID);
	//if (!DevStatusRowPtr) {
		String strCmd = String::format("%d:9:%d", _nodeID, _percentage);
		rc = theRadio.ProcessSend(strCmd);
	//}
	return rc;
}

BOOL SmartControllerClass::ChangeLampCCT(UC _nodeID, US _cct)
{
	BOOL rc = false;
	String strCmd = String::format("%d:11:%d", _nodeID, _cct);
	rc = theRadio.ProcessSend(strCmd);
	return rc;
}

BOOL SmartControllerClass::ChangeBR_CCT(UC _nodeID, UC _br, US _cct)
{
	String strCmd = String::format("%d:13:%d:%d", _nodeID, _br, _cct);
	return theRadio.ProcessSend(strCmd);
}

BOOL SmartControllerClass::ChangeLampScenario(UC _nodeID, UC _scenarioID)
{
	// Find node object
	ListNode<DevStatusRow_t> *DevStatusRowPtr = SearchDevStatus(_nodeID);
	if (DevStatusRowPtr == NULL)
	{
		LOGW(LOGTAG_MSG, "Failed to execte CMD_SCENARIO, wrong node_id %d", _nodeID);
	}

	// Find hue data of the 3 rings
	BOOL _findIt;
	ListNode<ScenarioRow_t> *rowptr = SearchScenario(_scenarioID);
	if (rowptr)
	{
		_findIt = true;
		String strCmd;
		if( rowptr->data.sw != DEVICE_SW_DUMMY ) {
			strCmd = String::format("%d:7:%d", _nodeID, rowptr->data.sw);
			theRadio.ProcessSend(strCmd);
		} else {
			UC lv_type = devtypCRing3;
			if( DevStatusRowPtr ) lv_type = DevStatusRowPtr->data.type;
			if(IS_SUNNY(lv_type)) {
				if( rowptr->data.ring[0].State == DEVICE_SW_OFF ) {
					strCmd = String::format("%d:7:0", _nodeID);
				} else {
					strCmd = String::format("%d:13:%d:%d", _nodeID, rowptr->data.ring[0].BR, rowptr->data.ring[0].CCT);
				}
				theRadio.ProcessSend(strCmd);
			} else { // Rainbow and Migrage
				MyMessage tmpMsg;
				UC payl_buf[MAX_PAYLOAD];
				UC payl_len;

				// All rings same settings
				bool bAllRings = (rowptr->data.ring[1].CCT == 256);

				for( UC idx = 0; idx < MAX_RING_NUM; idx++ ) {
					if( !bAllRings || idx == 0 ) {
						payl_len = CreateColorPayload(payl_buf, bAllRings ? RING_ID_ALL : idx + 1, rowptr->data.ring[idx].State,
												rowptr->data.ring[idx].BR, rowptr->data.ring[idx].CCT % 256, rowptr->data.ring[idx].R, rowptr->data.ring[idx].G, rowptr->data.ring[idx].B);
						tmpMsg.build(theRadio.getAddress(), _nodeID, 0, C_SET, V_RGBW, true);
						tmpMsg.set((void *)payl_buf, payl_len);
						theRadio.ProcessSend(&tmpMsg);
					}
					if( IS_MIRAGE(lv_type) ) {
						// ToDo: construct mirage message
						//tmpMsg.build(theRadio.getAddress(), _nodeID, NODEID_DUMMY, C_SET, V_DISTANCE, true);
						//tmpMsg.set((void *)payl_buf, payl_len);
						//theRadio.ProcessSend(&tmpMsg);
					}
				}
			}
		}
		rowptr->data.run_flag = EXECUTED;
		theConfig.SetSNTChanged(true);
	}
	else
	{
		_findIt = false;
		LOGE(LOGTAG_MSG, "Could not change node:%d light's color, scenario %d not found", _nodeID, _scenarioID);
	}

	// Publish Device-Scenario-Change message
	String strTemp = String::format("{'node_id':%d,'SNT_uid':%d,'found':%d}",
			 _nodeID, _scenarioID, _findIt);
	PublishDeviceStatus(strTemp.c_str());

	return _findIt;
}

BOOL SmartControllerClass::RequestDeviceStatus(UC _nodeID)
{
	BOOL rc = false;
	String strCmd = String::format("%d:12:%d", _nodeID);
	rc = theRadio.ProcessSend(strCmd);
	return rc;
}

BOOL SmartControllerClass::ConfirmLampOnOff(UC _nodeID, UC _st)
{
	BOOL rc = false;
	//m_pMainDev->data.ring[0].State = _st;
	ListNode<DevStatusRow_t> *DevStatusRowPtr = SearchDevStatus(_nodeID);
	if (DevStatusRowPtr) {
		DevStatusRowPtr->data.ring[0].State = _st;
		DevStatusRowPtr->data.ring[1].State = _st;
		DevStatusRowPtr->data.ring[2].State = _st;
		DevStatusRowPtr->data.run_flag = EXECUTED;
		DevStatusRowPtr->data.flash_flag = UNSAVED;
		DevStatusRowPtr->data.op_flag = POST;
		theConfig.SetDSTChanged(true);

		// Set panel ring on or off
		if( IS_CURRENT_DEVICE(_nodeID) ) {
			thePanel.SetRingOnOff(_st);
		}

		// Publish device status event
		StaticJsonBuffer<256> jBuf;
		JsonObject *jroot;
		jroot = &(jBuf.createObject());
		if( jroot->success() ) {
			(*jroot)["nd"] = _nodeID;
			(*jroot)["State"] = _st;
			char buffer[256];
			jroot->printTo(buffer, 256);
			PublishDeviceStatus(buffer);
		}

		rc = true;
	}
	return rc;
}

BOOL SmartControllerClass::ConfirmLampBrightness(UC _nodeID, UC _st, UC _percentage, UC _ringID)
{
	BOOL rc = false;
	UC r_index = (_ringID == RING_ID_ALL ? 0 : _ringID - 1);
	//m_pMainDev->data.ring[0].BR = _percentage;
	ListNode<DevStatusRow_t> *DevStatusRowPtr = SearchDevStatus(_nodeID);
	if (DevStatusRowPtr) {
		if( DevStatusRowPtr->data.ring[r_index].State != _st || DevStatusRowPtr->data.ring[r_index].BR != _percentage ) {
			DevStatusRowPtr->data.ring[r_index].State = _st;
			DevStatusRowPtr->data.ring[r_index].BR = _percentage;
			if( _ringID == RING_ID_ALL ) {
				DevStatusRowPtr->data.ring[1].State = _st;
				DevStatusRowPtr->data.ring[1].BR = _percentage;
				DevStatusRowPtr->data.ring[2].State = _st;
				DevStatusRowPtr->data.ring[2].BR = _percentage;
			}
			DevStatusRowPtr->data.run_flag = EXECUTED;
			DevStatusRowPtr->data.flash_flag = UNSAVED;
			DevStatusRowPtr->data.op_flag = POST;
			theConfig.SetDSTChanged(true);

			if( IS_CURRENT_DEVICE(_nodeID) ) {
				if( r_index == 0 ) {
					// Set panel ring to new position
					thePanel.UpdateDimmerValue(_percentage);
					// Set panel ring off
					thePanel.SetRingOnOff(_st);
				}
			}

			// Publish device status event
			StaticJsonBuffer<256> jBuf;
			JsonObject *jroot;
			jroot = &(jBuf.createObject());
			if( jroot->success() ) {
				(*jroot)["nd"] = _nodeID;
				if( _ringID != RING_ID_ALL ) {
					(*jroot)["Ring"] = _ringID;
				}
				(*jroot)["State"] = _st;
				(*jroot)["BR"] = _percentage;
				char buffer[256];
				jroot->printTo(buffer, 256);
				PublishDeviceStatus(buffer);
			}
		}

		rc = true;
	}
	return rc;
}

BOOL SmartControllerClass::ConfirmLampCCT(UC _nodeID, US _cct, UC _ringID)
{
	BOOL rc = false;
	UC r_index = (_ringID == RING_ID_ALL ? 0 : _ringID - 1);
	//m_pMainDev->data.ring[0].CCT = _cct;
	ListNode<DevStatusRow_t> *DevStatusRowPtr = SearchDevStatus(_nodeID);
	if (DevStatusRowPtr) {
		if( DevStatusRowPtr->data.ring[r_index].CCT != _cct ) {
			DevStatusRowPtr->data.ring[r_index].CCT = _cct;
			if( _ringID == RING_ID_ALL ) {
				DevStatusRowPtr->data.ring[1].CCT = _cct;
				DevStatusRowPtr->data.ring[2].CCT = _cct;
			}
			DevStatusRowPtr->data.run_flag = EXECUTED;
			DevStatusRowPtr->data.flash_flag = UNSAVED;
			DevStatusRowPtr->data.op_flag = POST;
			theConfig.SetDSTChanged(true);

			if( IS_CURRENT_DEVICE(_nodeID) ) {
				if( r_index == 0 ) {
					// Update cooresponding panel CCT value
					thePanel.UpdateCCTValue(_cct);
					thePanel.SetRingOnOff(DevStatusRowPtr->data.ring[0].State);
				}
			}

			// Publish device status event
			StaticJsonBuffer<256> jBuf;
			JsonObject *jroot;
			jroot = &(jBuf.createObject());
			if( jroot->success() ) {
				(*jroot)["nd"] = _nodeID;
				if( _ringID != RING_ID_ALL ) {
					(*jroot)["Ring"] = _ringID;
				}
				(*jroot)["CCT"] = _cct;
				char buffer[256];
				jroot->printTo(buffer, 256);
				PublishDeviceStatus(buffer);
			}
		}

		rc = true;
	}
	return rc;
}

BOOL SmartControllerClass::ConfirmLampHue(UC _nodeID, UC _white, UC _red, UC _green, UC _blue, UC _ringID)
{
	BOOL rc = false;
	UC r_index = (_ringID == RING_ID_ALL ? 0 : _ringID - 1);
	ListNode<DevStatusRow_t> *DevStatusRowPtr = SearchDevStatus(_nodeID);
	if (DevStatusRowPtr) {
		if( (DevStatusRowPtr->data.ring[r_index].CCT % 256) != _white ||
		    DevStatusRowPtr->data.ring[r_index].R != _red ||
			  DevStatusRowPtr->data.ring[r_index].G != _green ||
			  DevStatusRowPtr->data.ring[r_index].B != _blue )
		{
			DevStatusRowPtr->data.ring[r_index].CCT = _white;
			DevStatusRowPtr->data.ring[r_index].R = _red;
			DevStatusRowPtr->data.ring[r_index].G = _green;
			DevStatusRowPtr->data.ring[r_index].B = _blue;
			if( _ringID == RING_ID_ALL ) {
				DevStatusRowPtr->data.ring[1].CCT = _white;
				DevStatusRowPtr->data.ring[1].R = _red;
				DevStatusRowPtr->data.ring[1].G = _green;
				DevStatusRowPtr->data.ring[1].B = _blue;
				DevStatusRowPtr->data.ring[2].CCT = _white;
				DevStatusRowPtr->data.ring[2].R = _red;
				DevStatusRowPtr->data.ring[2].G = _green;
				DevStatusRowPtr->data.ring[2].B = _blue;
			}
			DevStatusRowPtr->data.run_flag = EXECUTED;
			DevStatusRowPtr->data.flash_flag = UNSAVED;
			DevStatusRowPtr->data.op_flag = POST;
			theConfig.SetDSTChanged(true);

			// Publish device status event
			StaticJsonBuffer<256> jBuf;
			JsonObject *jroot;
			jroot = &(jBuf.createObject());
			if( jroot->success() ) {
				(*jroot)["nd"] = _nodeID;
				if( _ringID != RING_ID_ALL ) {
					(*jroot)["Ring"] = _ringID;
				}
				(*jroot)["W"] = _white;
				(*jroot)["R"] = _red;
				(*jroot)["G"] = _green;
				(*jroot)["B"] = _blue;
				char buffer[256];
				jroot->printTo(buffer, 256);
				PublishDeviceStatus(buffer);
			}
		}
		rc = true;
	}
	return rc;
}

BOOL SmartControllerClass::ConfirmLampTop(UC _nodeID, UC *_payl, UC _len)
{
	BOOL rc = false, bChanged = false;
	UC _ringID, _L1, _L2, _L3;
	UC r_index, _pos = 3;

	if( _len >= 7 ) {
		ListNode<DevStatusRow_t> *DevStatusRowPtr = SearchDevStatus(_nodeID);
		if (DevStatusRowPtr) {
			StaticJsonBuffer<256> jBuf;
			JsonObject *jroot;
			jroot = &(jBuf.createObject());
			String sRingName;
			if( jroot->success() ) {
				(*jroot)["nd"] = _nodeID;
			}

			while( _pos + 3 < _len )
			{
				_ringID = _payl[_pos++];
				_L1 = _payl[_pos++];
				_L2 = _payl[_pos++];
				_L3 = _payl[_pos++];

				// Publish device topology event
				r_index = (_ringID == RING_ID_ALL ? 0 : _ringID - 1);
				if( DevStatusRowPtr->data.ring[r_index].L1 != _L1 ||
				    DevStatusRowPtr->data.ring[r_index].L2 != _L2 ||
					  DevStatusRowPtr->data.ring[r_index].L3 != _L3 )
				{
					DevStatusRowPtr->data.ring[r_index].L1 = _L1;
					DevStatusRowPtr->data.ring[r_index].L2 = _L2;
					DevStatusRowPtr->data.ring[r_index].L3 = _L3;
					if( _ringID == RING_ID_ALL ) {
						DevStatusRowPtr->data.ring[1].L1 = _L1;
						DevStatusRowPtr->data.ring[1].L2 = _L2;
						DevStatusRowPtr->data.ring[1].L3 = _L3;
						DevStatusRowPtr->data.ring[2].L1 = _L1;
						DevStatusRowPtr->data.ring[2].L2 = _L2;
						DevStatusRowPtr->data.ring[2].L3 = _L3;
					}
					bChanged = true;

					if( jroot->success() ) {
						sRingName = String::format("ring%d", _ringID);
						(*jroot)[sRingName][0] = _L1;
						(*jroot)[sRingName][1] = _L2;
						(*jroot)[sRingName][2] = _L3;
					}
				}
			}

			if( bChanged ) {
				// Set data changed flag
				DevStatusRowPtr->data.run_flag = EXECUTED;
				DevStatusRowPtr->data.flash_flag = UNSAVED;
				DevStatusRowPtr->data.op_flag = POST;
				theConfig.SetDSTChanged(true);

				// Publish event
				if( jroot->success() ) {
					char buffer[256];
					jroot->printTo(buffer, 256);
					PublishDeviceStatus(buffer);
				}
			}
		}
	}

	return rc;
}

BOOL SmartControllerClass::QueryDeviceStatus(UC _nodeID, UC _ringID)
{
	ListNode<DevStatusRow_t> *DevStatusRowPtr = SearchDevStatus(_nodeID);
	if (DevStatusRowPtr) {
		StaticJsonBuffer<256> jBuf;
		JsonObject *jroot;
		char buffer[256];
		jroot = &(jBuf.createObject());
		if( jroot->success() ) {
			if( IS_SUNNY(DevStatusRowPtr->data.type) ) {
				(*jroot)["nd"] = DevStatusRowPtr->data.node_id;
				(*jroot)["State"] = DevStatusRowPtr->data.ring[0].State;
				(*jroot)["BR"] = DevStatusRowPtr->data.ring[0].BR;
				(*jroot)["CCT"] = DevStatusRowPtr->data.ring[0].CCT;
				jroot->printTo(buffer, 256);
				return PublishDeviceStatus(buffer);
			} else if( IS_RAINBOW(DevStatusRowPtr->data.type) || IS_MIRAGE(DevStatusRowPtr->data.type) ) {
				UC r_index;
				BOOL _bMore = false;
				do {
					if( _ringID > MAX_RING_NUM ) {
						_bMore = false;
						break;
					}
					(*jroot)["nd"] = DevStatusRowPtr->data.node_id;
					if( _ringID == RING_ID_ALL ) {
						r_index = 0;
						if( IsAllRingHueSame(DevStatusRowPtr) ) {
							(*jroot)["Ring"] = RING_ID_ALL;
						} else {
							(*jroot)["Ring"] = RING_ID_1;
							_bMore = true;
							_ringID = RING_ID_2;	// Next
						}
					} else {
						r_index = _ringID - 1;
						(*jroot)["Ring"] = RING_ID_1;
						if( _bMore ) _ringID++;
					}
					(*jroot)["State"] = DevStatusRowPtr->data.ring[r_index].State;
					(*jroot)["BR"] = DevStatusRowPtr->data.ring[r_index].BR;
					(*jroot)["W"] = DevStatusRowPtr->data.ring[r_index].CCT % 256;
					(*jroot)["R"] = DevStatusRowPtr->data.ring[r_index].R;
					(*jroot)["G"] = DevStatusRowPtr->data.ring[r_index].G;
					(*jroot)["B"] = DevStatusRowPtr->data.ring[r_index].B;
					jroot->printTo(buffer, 256);
					PublishDeviceStatus(buffer);
				} while(_bMore);
			}
			return true;
		}
	}
	return false;
}

BOOL SmartControllerClass::IsAllRingHueSame(ListNode<DevStatusRow_t> *pDev)
{
	if( pDev ) {
		if( pDev->data.ring[0].CCT != pDev->data.ring[1].CCT || pDev->data.ring[0].CCT != pDev->data.ring[2].CCT ) {
			return false;
		}
		if( pDev->data.ring[0].R != pDev->data.ring[1].R || pDev->data.ring[0].R != pDev->data.ring[2].R ) {
			return false;
		}
		if( pDev->data.ring[0].G != pDev->data.ring[1].G || pDev->data.ring[0].G != pDev->data.ring[2].G ) {
			return false;
		}
		if( pDev->data.ring[0].B != pDev->data.ring[1].B || pDev->data.ring[0].B != pDev->data.ring[2].B ) {
			return false;
		}
	}
	return true;
}
//------------------------------------------------------------------
// Printing tables/working memory chains
//------------------------------------------------------------------
String SmartControllerClass::print_devStatus_table(int row)
{
	String strShortDesc;

	SERIAL_LN("==== DevStatus Row %d ====", row);
	SERIAL_LN("%cuid = %d, type = %d", DevStatus_table.get(row).node_id == CURRENT_DEVICE ? '*' : ' ', DevStatus_table.get(row).uid, DevStatus_table.get(row).type);
	SERIAL_LN("node_id = %d, present = %s", DevStatus_table.get(row).node_id, (DevStatus_table.get(row).present ? "true" : "false"));

	if( IS_SUNNY(DevStatus_table.get(row).type) ) {
		SERIAL_LN("Status: %s, BR: %d, CCT: %d\n\r", DevStatus_table.get(row).ring[0].State ? "On" : "Off",
					DevStatus_table.get(row).ring[0].BR, DevStatus_table.get(row).ring[0].CCT);
		strShortDesc = String::format("%cnid:%d St:%d BR:%d, CCT:%d",
				DevStatus_table.get(row).node_id == CURRENT_DEVICE ? '*' : ' ',
				DevStatus_table.get(row).node_id, DevStatus_table.get(row).ring[0].State,
				DevStatus_table.get(row).ring[0].BR, DevStatus_table.get(row).ring[0].CCT);
	} else {
		SERIAL_LN("Status: %s, BR: %d, WRGB: %d,%d,%d,%d\n\r", DevStatus_table.get(row).ring[0].State ? "On" : "Off",
					DevStatus_table.get(row).ring[0].BR, DevStatus_table.get(row).ring[0].CCT,
				  DevStatus_table.get(row).ring[0].R, DevStatus_table.get(row).ring[0].G,
					DevStatus_table.get(row).ring[0].B);
		strShortDesc = String::format("%cnid:%d St:%d BR:%d, WRGB:%d,%d,%d,%d",
				DevStatus_table.get(row).node_id == CURRENT_DEVICE ? '*' : ' ',
				DevStatus_table.get(row).node_id, DevStatus_table.get(row).ring[0].State,
				DevStatus_table.get(row).ring[0].BR, DevStatus_table.get(row).ring[0].CCT,
			  DevStatus_table.get(row).ring[0].R, DevStatus_table.get(row).ring[0].G,
			  DevStatus_table.get(row).ring[0].B);
	}
	return(strShortDesc);

/*
	SERIAL_LN("==========================");
	SERIAL_LN("==== DevStatus Row %d ====", row);

	switch (DevStatus_table.get(row).op_flag)
	{
	case 0: SERIAL_LN("op_flag = GET"); break;
	case 1: SERIAL_LN("op_flag = POST"); break;
	case 2: SERIAL_LN("op_flag = PUT"); break;
	case 3: SERIAL_LN("op_flag = DELETE"); break;
	}
	switch (Schedule_table.get(row).flash_flag)
	{
	case 0: SERIAL_LN("flash_flag = UNSAVED"); break;
	case 1: SERIAL_LN("flash_flag = SAVED"); break;
	}
	switch (Schedule_table.get(row).run_flag)
	{
	case 0: SERIAL_LN("run_flag = UNEXECUTED"); break;
	case 1: SERIAL_LN("run_flag = EXECUTED"); break;
	}
	SERIAL_LN("uid = %d, type = %d", DevStatus_table.get(row).uid, DevStatus_table.get(row).type);
	SERIAL_LN("node_id = %d, present = %s", DevStatus_table.get(row).node_id, (DevStatus_table.get(row).present ? "true" : "false"));
	SERIAL_LN("ring1 = %s", hue_to_string(DevStatus_table.get(row).ring1).c_str());
	SERIAL_LN("ring2 = %s", hue_to_string(DevStatus_table.get(row).ring2).c_str());
	SERIAL_LN("ring3 = %s", hue_to_string(DevStatus_table.get(row).ring3).c_str());
	*/
}

void SmartControllerClass::print_schedule_table(int row)
{
	SERIAL_LN("====================");
	SERIAL_LN("==== SCT-Row %d ====", row);

	switch (Schedule_table.get(row).op_flag)
	{
	case 0: SERIAL_LN("op_flag = GET"); break;
	case 1: SERIAL_LN("op_flag = POST"); break;
	case 2: SERIAL_LN("op_flag = PUT"); break;
	case 3: SERIAL_LN("op_flag = DELETE"); break;
	}
	switch (Schedule_table.get(row).flash_flag)
	{
	case 0: SERIAL_LN("flash_flag = UNSAVED"); break;
	case 1: SERIAL_LN("flash_flag = SAVED"); break;
	}
	switch (Schedule_table.get(row).run_flag)
	{
	case 0: SERIAL_LN("run_flag = UNEXECUTED"); break;
	case 1: SERIAL_LN("run_flag = EXECUTED"); break;
	}
	SERIAL_LN("uid = %d", Schedule_table.get(row).uid);
	SERIAL_LN("weekdays = %d", Schedule_table.get(row).weekdays);
	SERIAL_LN("isRepeat = %s", (Schedule_table.get(row).isRepeat ? "true" : "false"));
	SERIAL_LN("hour = %d", Schedule_table.get(row).hour);
	SERIAL_LN("min = %d", Schedule_table.get(row).minute);
	SERIAL_LN("alarm_id = %u", Schedule_table.get(row).alarm_id);
}

void SmartControllerClass::print_scenario_table(int row)
{
	ScenarioRow_t rowData = Scenario_table.get(row);
	SERIAL_LN("====================");
	SERIAL_LN("==== SNT-Row %d ====", row);

	switch (rowData.op_flag)
	{
	case 0: SERIAL_LN("op_flag = GET"); break;
	case 1: SERIAL_LN("op_flag = POST"); break;
	case 2: SERIAL_LN("op_flag = PUT"); break;
	case 3: SERIAL_LN("op_flag = DELETE"); break;
	}
	switch (rowData.flash_flag)
	{
	case 0: SERIAL_LN("flash_flag = UNSAVED"); break;
	case 1: SERIAL_LN("flash_flag = SAVED"); break;
	}
	switch (rowData.run_flag)
	{
	case 0: SERIAL_LN("run_flag = UNEXECUTED"); break;
	case 1: SERIAL_LN("run_flag = EXECUTED"); break;
	}
	SERIAL_LN("uid = %d", rowData.uid);
	if( rowData.sw != DEVICE_SW_DUMMY ) {
		SERIAL_LN("switch = %d", rowData.sw);
	} else {
		SERIAL_LN("ring1 = %s", hue_to_string(rowData.ring[0]).c_str());
		SERIAL_LN("ring2 = %s", hue_to_string(rowData.ring[1]).c_str());
		SERIAL_LN("ring3 = %s", hue_to_string(rowData.ring[2]).c_str());
	}
	SERIAL_LN("filter = %d", rowData.filter);
}

void SmartControllerClass::print_rule_table(int row)
{
	SERIAL_LN("===================");
	SERIAL_LN("==== RT-Row %d ====", row);

	switch (Rule_table.get(row).op_flag)
	{
	case 0: SERIAL_LN("op_flag = GET"); break;
	case 1: SERIAL_LN("op_flag = POST"); break;
	case 2: SERIAL_LN("op_flag = PUT"); break;
	case 3: SERIAL_LN("op_flag = DELETE"); break;
	}
	switch (Rule_table.get(row).flash_flag)
	{
	case 0: SERIAL_LN("flash_flag = UNSAVED"); break;
	case 1: SERIAL_LN("flash_flag = SAVED"); break;
	}
	switch (Rule_table.get(row).run_flag)
	{
	case 0: SERIAL_LN("run_flag = UNEXECUTED"); break;
	case 1: SERIAL_LN("run_flag = EXECUTED"); break;
	}
	SERIAL_LN("uid = %d", Rule_table.get(row).uid);
	SERIAL_LN("node_id = %d", Rule_table.get(row).node_id);
	SERIAL_LN("SCT_uid = %d", Rule_table.get(row).SCT_uid);
	SERIAL_LN("SNT_uid = %d", Rule_table.get(row).SNT_uid);
	SERIAL_LN("notif_uid = %d", Rule_table.get(row).notif_uid);
	SERIAL_LN("tmr_int = %d", Rule_table.get(row).tmr_int);
	SERIAL_LN("tmr_started = %d", Rule_table.get(row).tmr_started);
	SERIAL_LN("tmr_span = %d m", Rule_table.get(row).tmr_span);
}

//------------------------------------------------------------------
// Place all util func below
//------------------------------------------------------------------
// Copy JSON array to Hue structure
void SmartControllerClass::Array2Hue(JsonArray& data, Hue_t& hue)
{
	hue.State = data[0];
	hue.BR = data[1];
	hue.CCT = data[2];
	hue.R = data[3];
	hue.G = data[4];
	hue.B = data[5];
}

String SmartControllerClass::hue_to_string(Hue_t hue)
{
	String out = "";
	out.concat("State:");
	out.concat(hue.State);
	out.concat("|");
	out.concat("BR:");
	out.concat(hue.BR);
	out.concat("|");
	out.concat("CCT:");
	out.concat(hue.CCT);
	out.concat("|");
	out.concat("R:");
	out.concat(hue.R);
	out.concat("|");
	out.concat("G:");
	out.concat(hue.G);
	out.concat("|");
	out.concat("B:");
	out.concat(hue.B);

	return out;
}

UC SmartControllerClass::CreateColorPayload(UC *payl, uint8_t ring, uint8_t State, uint8_t BR, uint8_t W, uint8_t R, uint8_t G, uint8_t B)
{
	// Payload length
	UC payl_len = 0;
	if( payl ) {
		payl[payl_len++] = ring;
		payl[payl_len++] = State;
		payl[payl_len++] = BR;
		payl[payl_len++] = W;
		payl[payl_len++] = R;
		payl[payl_len++] = G;
		payl[payl_len++] = B;
	}

	return payl_len;
}
