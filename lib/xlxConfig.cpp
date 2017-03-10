/**
 * xlxConfig.cpp - Xlight Config library for loading & saving configuration
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
 * 1. configurations are stored in the emulated EEPROM and P1 external Flash
 * 1.1 The Photon and Electron have 2047 bytes of emulated EEPROM
 *     with address space from 0x0000 to 0x07FF.
 * 1.2 The P1 has 1M bytes of external Flash, but can only be accessed via
 *     3rd party API currently.
 * 2. Use EEPROM class (high level API) to access the emulated EEPROM.
 * 3. Use spark-flashee-eeprom (low level 3rd party API) to access P1 external Flash.
 * 4. Please refer to xliMemoryMap.h for memory allocation.
 *
 * ToDo:
 * 1. Move default config values to header as global #define's
 * 2.
 * 3.
**/

#include "xlxConfig.h"
#include "xliPinMap.h"
#include "xlxLogger.h"
#include "xliMemoryMap.h"
#include "xlxPanel.h"
#include "xlxRF24Server.h"
#include "xlSmartController.h"

using namespace Flashee;

// the one and only instance of ConfigClass
ConfigClass theConfig = ConfigClass();

//------------------------------------------------------------------
// Xlight Node List Class
//------------------------------------------------------------------
int NodeListClass::getMemSize()
{
	return(sizeof(NodeIdRow_t) * _count);
}

int NodeListClass::getFlashSize()
{
	return(min(sizeof(NodeIdRow_t) * _size, MEM_NODELIST_LEN));
}

// Load node list from EEPROM
bool NodeListClass::loadList()
{
	NodeIdRow_t lv_Node;
	for(int i = 0; i < theConfig.GetNumNodes(); i++) {
		int offset = MEM_NODELIST_OFFSET + i * sizeof(NodeIdRow_t);
		if( offset >= MEM_NODELIST_OFFSET + MEM_NODELIST_LEN - sizeof(NodeIdRow_t) ) break;

		EEPROM.get(offset, lv_Node);
		// Initialize two preset nodes
		if( i == 0 ) {
			if( lv_Node.nid != NODEID_MAINDEVICE ) {
				lv_Node.nid = NODEID_MAINDEVICE;
				resetIdentity(lv_Node.identity);
				lv_Node.recentActive = 0;
				m_isChanged = true;
			}
		} else if(  i == 1 && theConfig.GetNumNodes() <= 2 ) {
			if( lv_Node.nid != NODEID_MIN_REMOTE ) {
				lv_Node.nid = NODEID_MIN_REMOTE;
				resetIdentity(lv_Node.identity);
				lv_Node.recentActive = 0;
				m_isChanged = true;
			}
		} else if( lv_Node.nid == NODEID_DUMMY || lv_Node.nid == 0 ) {
			theConfig.SetNumNodes(count());
			break;
		}
		if( add(&lv_Node) < 0 ) break;
	}
	saveList();
	return true;
}

bool NodeListClass::saveList()
{
	if( m_isChanged ) {
		m_isChanged = false;
		/*
		for(int i = 0; i < count(); i++) {
			int offset = MEM_NODELIST_OFFSET + i * sizeof(NodeIdRow_t);
			if( offset > MEM_NODELIST_OFFSET + MEM_NODELIST_LEN - sizeof(NodeIdRow_t) ) break;
			EEPROM.put(offset, _pItems[i]);
		}*/
		NodeIdRow_t lv_buf[MAX_NODE_PER_CONTROLLER];
		memset(lv_buf, 0x00, sizeof(lv_buf));
		memcpy(lv_buf, _pItems, sizeof(NodeIdRow_t) * count());
		EEPROM.put(MEM_NODELIST_OFFSET, lv_buf);
		theConfig.SetNumNodes(count());
	}
	return true;
}

void NodeListClass::showList()
{
	char strDisplay[64];
	UL lv_now = Time.now();
	for(int i=0; i < _count; i++) {
		SERIAL_LN("%cNo.%d - NodeID: %d (%s) actived %ds ago",
				_pItems[i].nid == CURRENT_DEVICE ? '*' : ' ', i,
		    _pItems[i].nid, PrintMacAddress(strDisplay, _pItems[i].identity),
				(_pItems[i].recentActive > 0 ? lv_now - _pItems[i].recentActive : -1));
	}
}

// Get a new NodeID
UC NodeListClass::requestNodeID(char type, uint64_t identity)
{
	// Must provide identity
	if( identity == 0 ) return 0;

	UC nodeID = 0;		// error
	switch( type ) {
	case NODE_TYP_LAMP:
		// 1, 8 - 63
		/// Check Main DeviceID
		nodeID = getAvailableNodeId(NODEID_MAINDEVICE, NODEID_MIN_DEVCIE, NODEID_MAX_DEVCIE, identity);
		break;

	case NODE_TYP_REMOTE:
		// 64 - 127
		nodeID = getAvailableNodeId(NODEID_MIN_REMOTE, NODEID_MIN_REMOTE+1, NODEID_MAX_REMOTE, identity);
		break;

	case NODE_TYP_THIRDPARTY:
		// ToDo: support thirdparty device in the future
		break;

	default:
		break;
	}

	// Add or Update
	if( nodeID > 0 ) {
		NodeIdRow_t lv_Node;
		lv_Node.nid = nodeID;
		copyIdentity(lv_Node.identity, &identity);
		lv_Node.recentActive = Time.now();
		UC oldCnt = count();
		add(&lv_Node);
		if( count() > oldCnt && nodeID <= NODEID_MAX_DEVCIE ) {
			// Functional device added
			theConfig.SetNumDevices(theConfig.GetNumDevices() + 1);
		}
		theConfig.SetNumNodes(count());
		m_isChanged = true;

		if( type == NODE_TYP_LAMP ) {
			if( theConfig.InitDevStatus(nodeID) ) {
				theConfig.SetDSTChanged(true);
				//ToDo: remove
				SERIAL_LN("Test DevStatus_table added item: %d, size: %d", nodeID, theSys.DevStatus_table.size());
			}
		}
	}
	return nodeID;
}

UC NodeListClass::getAvailableNodeId(UC defaultID, UC minID, UC maxID, uint64_t identity)
{
	UC oldestNode;
	UL oldestTime;
	NodeIdRow_t lv_Node;
	if( defaultID > 0 ) {
		lv_Node.nid = defaultID;
		if( get(&lv_Node) < 0 ) {
			return 0;
		} else if( lv_Node.recentActive == 0 || isIdentityEmpty(lv_Node.identity) || isIdentityEqual(lv_Node.identity, &identity) ) {
			// DefaultID is available
			return defaultID;
		} else {
			oldestNode = defaultID;
			oldestTime = lv_Node.recentActive;
		}
	} else {
		oldestNode = 0;
		oldestTime = Time.now();
	}

	// Stage 1: Check Identity and reuse if possible
	for(int i = 0; i < count(); i++) {
		if( _pItems[i].nid > maxID ) break;
		if( _pItems[i].nid < minID ) continue;

		if( isIdentityEqual(_pItems[i].identity, &identity) ) {
			// Return matched item
			return _pItems[i].nid;
		} else if( oldestNode == 0 || oldestTime > _pItems[i].recentActive ) {
			// Update inactive record
			oldestNode = _pItems[i].nid;
			oldestTime = _pItems[i].recentActive;
		}
	}

	// Stage 2: Otherwise, get a unused NodeID from corresponding segment
	for( UC nodeID = minID; nodeID <= maxID; nodeID++ ) {
		lv_Node.nid = nodeID;
		// Seek unused NodeID
		if( get(&lv_Node) < 0 ) return nodeID;
	}

	// Stage 3: Otherwise, overwrite the longest inactive entry within the segment
	return oldestNode;
}

BOOL NodeListClass::clearNodeId(UC nodeID)
{
	if( nodeID == NODEID_GATEWAY || nodeID == NODEID_DUMMY ) return false;

	NodeIdRow_t lv_Node;
	if( nodeID == NODEID_MAINDEVICE || nodeID == NODEID_MIN_REMOTE ) {
		// Update with black item
		lv_Node.nid = nodeID;
		resetIdentity(lv_Node.identity);
		lv_Node.recentActive = 0;
		update(&lv_Node);
	} else if ( nodeID < NODEID_MIN_DEVCIE ) {
		return false;
	} else {
		// remove item
		lv_Node.nid = nodeID;
		remove(&lv_Node);
		if( nodeID >= NODEID_MIN_DEVCIE && nodeID <= NODEID_MAX_DEVCIE ) {
			theConfig.SetNumDevices(theConfig.GetNumDevices() - 1);
		}
		theConfig.SetNumNodes(count());
	}

	m_isChanged = true;
	LOGN(LOGTAG_EVENT, "NodeID:%d is cleared", nodeID);
	return true;
}

//------------------------------------------------------------------
// Xlight Config Class
//------------------------------------------------------------------
ConfigClass::ConfigClass()
{
	P1Flash = Devices::createWearLevelErase();

  m_isLoaded = false;
  m_isChanged = false;
  m_isDSTChanged = false;
  m_isSCTChanged = false;
  m_isRTChanged = false;
  m_isSNTChanged = false;
	m_lastTimeSync = millis();
  InitConfig();
}

void ConfigClass::InitConfig()
{
  memset(&m_config, 0x00, sizeof(m_config));
  m_config.version = VERSION_CONFIG_DATA;
  m_config.timeZone.id = 90;              // Toronto
  m_config.timeZone.offset = -300;        // -5 hours
  m_config.timeZone.dst = 1;              // 1 or 0
  m_config.sensorBitmap = 0;
	SetSensorEnabled(sensorDHT);
	SetSensorEnabled(sensorALS);
	//SetSensorEnabled(sensorMIC);
	SetSensorEnabled(sensorPIR);
  strcpy(m_config.Organization, XLA_ORGANIZATION);
  strcpy(m_config.ProductName, XLA_PRODUCT_NAME);
  strcpy(m_config.Token, XLA_TOKEN);
  m_config.indBrightness = 0;
	m_config.mainDevID = NODEID_MAINDEVICE;
  m_config.typeMainDevice = (UC)devtypWRing3;
  m_config.numDevices = 1;
  m_config.numNodes = 2;	// One main device( the smart lamp) and one remote control
  m_config.enableCloudSerialCmd = false;
	m_config.enableSpeaker = false;
	m_config.enableDailyTimeSync = true;
	m_config.rfPowerLevel = RF24_PA_LEVEL_GW;
	m_config.maxBaseNetworkDuration = MAX_BASE_NETWORK_DUR;
	m_config.useCloud = CLOUD_ENABLE;
	m_config.stWiFi = 1;
}

BOOL ConfigClass::InitDevStatus(UC nodeID)
{
	if( theSys.SearchDevStatus(nodeID) ) return false;

	Hue_t whiteHue;
	whiteHue.B = 0;
	whiteHue.G = 0;
	whiteHue.R = 0;
	whiteHue.BR = 50;
	whiteHue.CCT = 2700;
	whiteHue.State = 1; // 1 for on, 0 for off

	//ToDo: ensure radio pairing has occured before doing this step in the future
	DevStatusRow_t first_row;

	first_row.op_flag = POST;						// 1
	first_row.flash_flag = UNSAVED;			// 0
	first_row.run_flag = EXECUTED;			// 1
	first_row.uid = theSys.DevStatus_table.size();
	first_row.node_id = nodeID;
	first_row.present = 0;
	first_row.token = 0;
	first_row.type = devtypWRing3;  // White 3 rings
	first_row.ring[0] = whiteHue;
	first_row.ring[1] = whiteHue;
	first_row.ring[2] = whiteHue;

	return theSys.DevStatus_table.add(first_row);
}

BOOL ConfigClass::MemWriteScenarioRow(ScenarioRow_t row, uint32_t address)
{
#ifdef MCU_TYPE_P1
	return P1Flash->write<ScenarioRow_t>(row, address);
#else
	return false;
#endif
}

BOOL ConfigClass::MemReadScenarioRow(ScenarioRow_t &row, uint32_t address)
{
#ifdef MCU_TYPE_P1
	return P1Flash->read<ScenarioRow_t>(row, address);
#else
	return false;
#endif
}

BOOL ConfigClass::LoadConfig()
{
  // Load System Configuration
  if( sizeof(Config_t) <= MEM_CONFIG_LEN )
  {
    EEPROM.get(MEM_CONFIG_OFFSET, m_config);
    if( m_config.version == 0xFF
      || m_config.timeZone.id == 0
      || m_config.timeZone.id > 500
      || m_config.timeZone.dst > 1
      || m_config.timeZone.offset < -780
      || m_config.timeZone.offset > 780
      || m_config.numDevices > MAX_DEVICE_PER_CONTROLLER
			|| m_config.numNodes < 2
			|| m_config.numNodes > MAX_NODE_PER_CONTROLLER
      || m_config.typeMainDevice == devtypUnknown
      || m_config.typeMainDevice >= devtypDummy
			|| m_config.rfPowerLevel > RF24_PA_MAX
		 	|| m_config.useCloud > CLOUD_MUST_CONNECT
		 	|| (IS_NOT_DEVICE_NODEID(m_config.mainDevID) && m_config.mainDevID != NODEID_DUMMY) )
    {
      InitConfig();
      m_isChanged = true;
      LOGW(LOGTAG_MSG, "Sysconfig is empty, use default settings.");
      SaveConfig();
    }
    else
    {
      LOGI(LOGTAG_MSG, "Sysconfig loaded.");
    }
    m_isLoaded = true;
    m_isChanged = false;
		UpdateTimeZone();
  } else {
    LOGE(LOGTAG_MSG, "Failed to load Sysconfig, too large.");
  }

	// Load Device Status
	LoadDeviceStatus();

	// We don't load Schedule Table directly
	// We don't load Scenario Table directly

	// Load Rules
	LoadRuleTable();

	// Load NodeID List
	LoadNodeIDList();

  return m_isLoaded;
}

BOOL ConfigClass::SaveConfig()
{
	// Check changes on Panel
	SetBrightIndicator(thePanel.GetDimmerValue());

  if( m_isChanged )
  {
    EEPROM.put(MEM_CONFIG_OFFSET, m_config);
    m_isChanged = false;
    LOGI(LOGTAG_MSG, "Sysconfig saved.");
  }

	// Save Device Status
	SaveDeviceStatus();

	// Save Schedule Table
	SaveScheduleTable();

	// Save Rule Table
	SaveRuleTable();

	// Save Scenario Table
	SaveScenarioTable();

	// Save NodeID List
	SaveNodeIDList();

  return true;
}

BOOL ConfigClass::IsConfigLoaded()
{
  return m_isLoaded;
}

BOOL ConfigClass::IsConfigChanged()
{
  return m_isChanged;
}

void ConfigClass::SetConfigChanged(BOOL flag)
{
	m_isChanged = flag;
}

BOOL ConfigClass::IsDSTChanged()
{
  return m_isDSTChanged;
}

void ConfigClass::SetDSTChanged(BOOL flag)
{
	m_isDSTChanged = flag;
}

BOOL ConfigClass::IsSCTChanged()
{
  return m_isSCTChanged;
}

void ConfigClass::SetSCTChanged(BOOL flag)
{
	m_isSCTChanged = flag;
}

BOOL ConfigClass::IsRTChanged()
{
	return m_isRTChanged;
}

void ConfigClass::SetRTChanged(BOOL flag)
{
	m_isRTChanged = flag;
}

BOOL ConfigClass::IsSNTChanged()
{
	return m_isSNTChanged;
}

void ConfigClass::SetSNTChanged(BOOL flag)
{
	m_isSNTChanged = flag;
}

BOOL ConfigClass::IsNIDChanged()
{
	return lstNodes.m_isChanged;
}

void ConfigClass::SetNIDChanged(BOOL flag)
{
	lstNodes.m_isChanged = flag;
}

UC ConfigClass::GetVersion()
{
  return m_config.version;
}

BOOL ConfigClass::SetVersion(UC ver)
{
  if( ver != m_config.version )
  {
    m_config.version = ver;
    m_isChanged = true;
  }
	return true;
}

US ConfigClass::GetTimeZoneID()
{
  return m_config.timeZone.id;
}

BOOL ConfigClass::SetTimeZoneID(US tz)
{
  if( tz == 0 || tz > 500 )
    return false;

  if( tz != m_config.timeZone.id )
  {
    m_config.timeZone.id = tz;
    m_isChanged = true;
    theSys.m_tzString = GetTimeZoneJSON();
  }
  return true;
}

void ConfigClass::UpdateTimeZone()
{
	// Change System Timezone
	Time.zone((float)GetTimeZoneOffset() / 60 + GetDaylightSaving());
}

UC ConfigClass::GetDaylightSaving()
{
  return m_config.timeZone.dst;
}

BOOL ConfigClass::SetDaylightSaving(UC flag)
{
  if( flag > 1 )
    return false;

  if( flag != m_config.timeZone.dst )
  {
    m_config.timeZone.dst = flag;
    m_isChanged = true;
    theSys.m_tzString = GetTimeZoneJSON();
		UpdateTimeZone();
  }
  return true;
}

SHORT ConfigClass::GetTimeZoneOffset()
{
  return m_config.timeZone.offset;
}

SHORT ConfigClass::GetTimeZoneDSTOffset()
{
  return (m_config.timeZone.offset + m_config.timeZone.dst * 60);
}

BOOL ConfigClass::SetTimeZoneOffset(SHORT offset)
{
  if( offset >= -780 && offset <= 780)
  {
    if( offset != m_config.timeZone.offset )
    {
      m_config.timeZone.offset = offset;
      m_isChanged = true;
      theSys.m_tzString = GetTimeZoneJSON();
			// Change System Timezone
			UpdateTimeZone();
    }
    return true;
  }
  return false;
}

String ConfigClass::GetTimeZoneJSON()
{
  String jsonStr = String::format("{\"id\":%d,\"offset\":%d,\"dst\":%d}",
      GetTimeZoneID(), GetTimeZoneOffset(), GetDaylightSaving());
  return jsonStr;
}

String ConfigClass::GetOrganization()
{
  String strName = m_config.Organization;
  return strName;
}

void ConfigClass::SetOrganization(const char *strName)
{
  strncpy(m_config.Organization, strName, sizeof(m_config.Organization) - 1);
  m_isChanged = true;
}

String ConfigClass::GetProductName()
{
  String strName = m_config.ProductName;
  return strName;
}

void ConfigClass::SetProductName(const char *strName)
{
  strncpy(m_config.ProductName, strName, sizeof(m_config.ProductName) - 1);
  m_isChanged = true;
}

String ConfigClass::GetToken()
{
  String strName = m_config.Token;
  return strName;
}

void ConfigClass::SetToken(const char *strName)
{
  strncpy(m_config.Token, strName, sizeof(m_config.Token) - 1);
  m_isChanged = true;
}

BOOL ConfigClass::ConfigClass::IsCloudSerialEnabled()
{
	return m_config.enableCloudSerialCmd;
}

void ConfigClass::SetCloudSerialEnabled(BOOL sw)
{
	if( sw != m_config.enableCloudSerialCmd ) {
		m_config.enableCloudSerialCmd = sw;
		m_isChanged = true;
	}
}

BOOL ConfigClass::ConfigClass::IsSpeakerEnabled()
{
	return m_config.enableSpeaker;
}

void ConfigClass::SetSpeakerEnabled(BOOL sw)
{
	if( sw != m_config.enableSpeaker ) {
		m_config.enableSpeaker = sw;
		m_isChanged = true;
	}
}

BOOL ConfigClass::ConfigClass::IsDailyTimeSyncEnabled()
{
	return m_config.enableDailyTimeSync;
}

void ConfigClass::SetDailyTimeSyncEnabled(BOOL sw)
{
	if( sw != m_config.enableDailyTimeSync ) {
		m_config.enableDailyTimeSync = sw;
		m_isChanged = true;
	}
}

void ConfigClass::DoTimeSync()
{
	// Request time synchronization from the Particle Cloud
	Particle.syncTime();
	m_lastTimeSync = millis();
	LOGI(LOGTAG_EVENT, "Time synchronized");
}

BOOL ConfigClass::CloudTimeSync(BOOL _force)
{
	if( _force ) {
		DoTimeSync();
		return true;
	} else if( theConfig.IsDailyTimeSyncEnabled() ) {
		if( (millis() - m_lastTimeSync) / 1000 > SECS_PER_DAY ) {
			DoTimeSync();
			return true;
		}
	}
	return false;
}

US ConfigClass::GetSensorBitmap()
{
	return m_config.sensorBitmap;
}

void ConfigClass::SetSensorBitmap(US bits)
{
	if( bits != m_config.sensorBitmap )
  {
    m_config.sensorBitmap = bits;
    m_isChanged = true;
  }
}

BOOL ConfigClass::IsSensorEnabled(sensors_t sr)
{
    return(BITTEST(m_config.sensorBitmap, sr));
}

void ConfigClass::SetSensorEnabled(sensors_t sr, BOOL sw)
{
  US bits = m_config.sensorBitmap;
  if (sw) {
    bits = BITSET(m_config.sensorBitmap, sr);
  } else {
    bits = BITUNSET(m_config.sensorBitmap, sr);
  }
	SetSensorBitmap(bits);
}

UC ConfigClass::GetBrightIndicator()
{
  return m_config.indBrightness;
}

BOOL ConfigClass::SetBrightIndicator(UC level)
{
  if( level != m_config.indBrightness )
  {
    m_config.indBrightness = level;
    m_isChanged = true;
    return true;
  }

  return false;
}

UC ConfigClass::GetMainDeviceID()
{
	return m_config.mainDevID;
}

BOOL ConfigClass::SetMainDeviceID(UC devID)
{
	if( IS_NOT_DEVICE_NODEID(devID) && devID != NODEID_DUMMY )
		return false;

  if( devID != m_config.mainDevID )
  {
    m_config.mainDevID = devID;
		theSys.FindCurrentDevice();		// Refresh current device pointer
    m_isChanged = true;
  }
  return true;
}

UC ConfigClass::GetMainDeviceType()
{
  return m_config.typeMainDevice;
}

BOOL ConfigClass::SetMainDeviceType(UC type)
{
  if( type > devtypUnknown && type < devtypDummy )
  {
    if( type != m_config.typeMainDevice )
    {
      m_config.typeMainDevice = type;
      m_isChanged = true;
    }
    return true;
  }
  return false;
}


UC ConfigClass::GetNumDevices()
{
  return m_config.numDevices;
}

BOOL ConfigClass::SetNumDevices(UC num)
{
  if( m_config.numDevices <= MAX_DEVICE_PER_CONTROLLER )
  {
    if( num != m_config.numDevices )
    {
      m_config.numDevices = num;
      m_isChanged = true;
    }
    return true;
  }
  return false;
}

UC ConfigClass::GetNumNodes()
{
	return m_config.numNodes;
}

BOOL ConfigClass::SetNumNodes(UC num)
{
	if( m_config.numNodes <= MAX_NODE_PER_CONTROLLER )
  {
    if( num != m_config.numNodes )
    {
      m_config.numNodes = num;
      m_isChanged = true;
    }
    return true;
  }
  return false;
}

US ConfigClass::GetMaxBaseNetworkDur()
{
	return m_config.maxBaseNetworkDuration;
}

BOOL ConfigClass::SetMaxBaseNetworkDur(US dur)
{
	if( dur != m_config.maxBaseNetworkDuration ) {
		m_config.maxBaseNetworkDuration = dur;
		m_isChanged = true;
		return true;
	}
	return false;
}

UC ConfigClass::GetUseCloud()
{
	return m_config.useCloud;
}

BOOL ConfigClass::SetUseCloud(UC opt)
{
	if( opt != m_config.useCloud && opt <= CLOUD_MUST_CONNECT ) {
		m_config.useCloud = opt;
		m_isChanged = true;
		return true;
	}
	return false;
}

BOOL ConfigClass::GetWiFiStatus()
{
  return m_config.stWiFi;
}

BOOL ConfigClass::SetWiFiStatus(BOOL _st)
{
  if( _st != m_config.stWiFi ) {
    m_config.stWiFi = _st;
    m_isChanged = true;
		LOGN(LOGTAG_STATUS, "Wi-Fi status set to %d", _st);
    return true;
  }
  return false;
}

UC ConfigClass::GetRFPowerLevel()
{
	return m_config.rfPowerLevel;
}

BOOL ConfigClass::SetRFPowerLevel(UC level)
{
	if( level > RF24_PA_MAX ) level = RF24_PA_MAX;
	if( level != m_config.rfPowerLevel ) {
		m_config.rfPowerLevel = level;
		theRadio.setPALevel(level);
		m_isChanged = true;
		return true;
	}
	return false;
}

// Load Device Status
BOOL ConfigClass::LoadDeviceStatus()
{
	if (sizeof(DevStatusRow_t) <= MEM_DEVICE_STATUS_LEN)
	{
		DevStatusRow_t DevStatusArray[MAX_DEVICE_PER_CONTROLLER];
		EEPROM.get(MEM_DEVICE_STATUS_OFFSET, DevStatusArray);
		//check row values / error cases

		for (int i = 0; i < MAX_DEVICE_PER_CONTROLLER; i++)
		{	// 111
			if (DevStatusArray[i].op_flag == POST
				&& DevStatusArray[i].flash_flag == SAVED
				&& DevStatusArray[i].run_flag == EXECUTED
				&& DevStatusArray[i].uid == i)
			{
				if (theSys.DevStatus_table.add(DevStatusArray[i]))
				{
					LOGD(LOGTAG_MSG, "Loaded device status row for node_id:%d, uid:%d", DevStatusArray[i].node_id, DevStatusArray[i].uid);
				}
				else
				{
					LOGW(LOGTAG_MSG, "DevStatus row %d failed to load from flash", i);
				}
			}
		}

		// This code should also be where device pairing happens.
		// Currently initiating with assumption of 1 light w node_id=1
		if (theSys.DevStatus_table.size() == 0)
		{
			if( InitDevStatus(NODEID_MAINDEVICE) ) {
				m_isDSTChanged = true;
				LOGW(LOGTAG_MSG, "Device status table blank, loaded default status.");
				SaveConfig();
			}
		}
		else
		{
			LOGD(LOGTAG_MSG, "Device status table loaded.");
		}

		m_isDSTChanged = false;
	}
	else
	{
		LOGW(LOGTAG_MSG, "Failed to load device status table, too large.");
		return false;
	}

	return true;
}

// Save Device Status
BOOL ConfigClass::SaveDeviceStatus()
{
	if (m_isDSTChanged)
	{
		bool success_flag = true;
		ListNode<DevStatusRow_t> *rowptr = theSys.DevStatus_table.getRoot();
		while (rowptr != NULL)
		{
			if (rowptr->data.run_flag == EXECUTED && rowptr->data.flash_flag == UNSAVED)
			{
				DevStatusRow_t tmpRow = rowptr->data;

				switch (rowptr->data.op_flag)
				{
				case DELETE:
					//change flags to 000 to indicate flash row is empty
					tmpRow.op_flag = GET;
					tmpRow.flash_flag = UNSAVED;
					tmpRow.run_flag = UNEXECUTED;
					break;

				case PUT:
				case POST:
				case GET:
					//change flags to 111 to indicate flash row is occupied
					tmpRow.op_flag = POST;
					tmpRow.flash_flag = SAVED;
					tmpRow.run_flag = EXECUTED;
					break;
				}

				//write tmpRow to flash using uid
				int row_index = rowptr->data.uid;
				if ((row_index) < MAX_DEVICE_PER_CONTROLLER)
				{
					EEPROM.put(MEM_DEVICE_STATUS_OFFSET + row_index*DST_ROW_SIZE, tmpRow);
					rowptr->data.flash_flag = SAVED;
				}
				else
				{
					LOGE(LOGTAG_MSG, "Error, cannot write DevStatus row %d to flash, out of memory bounds", row_index);
					success_flag = false;
				}
			}
			rowptr = rowptr->next;
		}
		if (success_flag)
		{
			m_isDSTChanged = false;
			LOGD(LOGTAG_MSG, "Device status table saved.");
		}
		else
		{
			LOGE(LOGTAG_MSG, "Unable to write 1 or more Device status table rows to flash");
		}
	}
}

// Save Schedule Table
BOOL ConfigClass::SaveScheduleTable()
{
  if( m_isSCTChanged )
  {
	  bool success_flag = true;

	  ListNode<ScheduleRow_t> *rowptr = theSys.Schedule_table.getRoot();
	  while (rowptr != NULL)
	  {
		  if (rowptr->data.run_flag == EXECUTED && rowptr->data.flash_flag == UNSAVED)
		  {
			  ScheduleRow_t tmpRow = rowptr->data; //copy of data to write to flash

			  switch (rowptr->data.op_flag)
			  {
				case DELETE:
					//change flags to 000 to indicate flash row is empty
					tmpRow.op_flag = GET;
					tmpRow.flash_flag = UNSAVED;
					tmpRow.run_flag = UNEXECUTED;
					break;

				case PUT:
				case POST:
				case GET:
					//change flags to 111 to indicate flash row is occupied
					tmpRow.op_flag = POST;
					tmpRow.flash_flag = SAVED;
					tmpRow.run_flag = EXECUTED;
					break;
			  }

			  //write tmpRow to flash
			  int row_index = rowptr->data.uid;
			  if (row_index < MAX_SCT_ROWS)
			  {
				  EEPROM.put(MEM_SCHEDULE_OFFSET + row_index*SCT_ROW_SIZE, tmpRow); //write to flash

				  rowptr->data.flash_flag = SAVED; //toggle flash flag
			  }
			  else
			  {
				  LOGE(LOGTAG_MSG, "Error, cannot write Schedule row %d to flash, out of memory bounds", row_index);
				  success_flag = false;
			  }
		  }
		  rowptr = rowptr->next;
	  }

	  if (success_flag)
	  {
		  m_isSCTChanged = false;
		  LOGD(LOGTAG_MSG, "Schedule table saved.");
			return true;
	  }
	  else
	  {
		  LOGE(LOGTAG_MSG, "Unable to write 1 or more Schedule table rows to flash");
	  }
  }

	return false;
}

// Save Scenario Table
BOOL ConfigClass::SaveScenarioTable()
{
  if (m_isSNTChanged)
  {
	  bool success_flag = true;

	  ListNode<ScenarioRow_t> *rowptr = theSys.Scenario_table.getRoot();
	  while (rowptr != NULL)
	  {
		  if (rowptr->data.run_flag == EXECUTED && rowptr->data.flash_flag == UNSAVED)
		  {
			  ScenarioRow_t tmpRow = rowptr->data; //copy of data to write to p1

			  switch (rowptr->data.op_flag)
			  {
			  case DELETE:
				  //change flags to indicate flash row is empty
				  tmpRow.op_flag = GET;
				  tmpRow.flash_flag = UNSAVED;
				  tmpRow.run_flag = UNEXECUTED;
				  break;
			  case PUT:
			  case POST:
			  case GET:
				  //change flags to 111 to indicate flash row is occupied
				  tmpRow.op_flag = POST;
				  tmpRow.flash_flag = SAVED;
				  tmpRow.run_flag = EXECUTED;
				  break;
			  }

			  //write tmpRow to p1
			  int row_index = rowptr->data.uid;
			  if (row_index < MAX_SNT_ROWS)
			  {
#ifdef MCU_TYPE_P1
				  P1Flash->write<ScenarioRow_t>(tmpRow, MEM_SCENARIOS_OFFSET + row_index*SNT_ROW_SIZE);
#endif
				  rowptr->data.flash_flag = SAVED; //toggle flash flag
			  }
			  else
			  {
				  LOGE(LOGTAG_MSG, "Error, cannot write Scenario row %d to flash, out of memory bounds", row_index);
				  success_flag = false;
			  }
		  }
		  rowptr = rowptr->next;
	  }

	  if (success_flag)
	  {
		  m_isSNTChanged = false;
		  LOGD(LOGTAG_MSG, "Scenario table saved.");
			return true;
	  }
	  else
	  {
		  LOGE(LOGTAG_MSG, "Unable to write 1 or more Scenario table rows to flash");
	  }
  }

	return false;
}

// Load Rules from P1 Flash
BOOL ConfigClass::LoadRuleTable()
{
#ifdef MCU_TYPE_P1
	RuleRow_t RuleArray[MAX_RT_ROWS];
	if (RT_ROW_SIZE*MAX_RT_ROWS <= MEM_RULES_LEN)
	{
		if (P1Flash->read<RuleRow_t[MAX_RT_ROWS]>(RuleArray, MEM_RULES_OFFSET))
		{
			for (int i = 0; i < MAX_RT_ROWS; i++) //interate through RuleArray for non-empty rows
			{
				if (RuleArray[i].op_flag == POST
					&& RuleArray[i].flash_flag == SAVED
					&& RuleArray[i].run_flag == EXECUTED
					&& RuleArray[i].uid == i)
				{
					//change flags to be written into working memory chain
					RuleArray[i].op_flag = POST;
					RuleArray[i].run_flag = UNEXECUTED;
					RuleArray[i].flash_flag = SAVED;		//Already know it exists in flash
					RuleArray[i].tmr_started = 0;
					if (!theSys.Rule_table.add(RuleArray[i])) //add non-empty row to working memory chain
					{
						LOGW(LOGTAG_MSG, "Rule row %d failed to load from flash", i);
					}
				}
				//else: row is either empty or trash; do nothing
			}
			m_isRTChanged = false; //since we are not calling SaveConfig(), change flag to false again
		}
		else
		{
			LOGW(LOGTAG_MSG, "Failed to read the rule table from flash.");
			return false;
		}
	}
	else
	{
		LOGW(LOGTAG_MSG, "Failed to load rule table, too large.");
		return false;
	}
#endif

	return true;
}

// Save Rule Table
BOOL ConfigClass::SaveRuleTable()
{
	if ( m_isRTChanged )
	{
		bool success_flag = true;

		ListNode<RuleRow_t> *rowptr = theSys.Rule_table.getRoot();
		while (rowptr != NULL)
		{
			if (rowptr->data.run_flag == EXECUTED && rowptr->data.flash_flag == UNSAVED)
			{
				RuleRow_t tmpRow = rowptr->data; //copy of data to write to p1

				switch (rowptr->data.op_flag)
				{
				case DELETE:
					//change flags to indicate flash row is empty
					tmpRow.op_flag = GET;
					tmpRow.flash_flag = UNSAVED;
					tmpRow.run_flag = UNEXECUTED;
					break;
				case PUT:
				case POST:
				case GET:
					//change flags to 111 to indicate flash row is occupied
					tmpRow.op_flag = POST;
					tmpRow.flash_flag = SAVED;
					tmpRow.run_flag = EXECUTED;
					break;
				}

				//write tmpRow to p1
				int row_index = rowptr->data.uid;
				if (row_index < MAX_RT_ROWS)
				{
	#ifdef MCU_TYPE_P1
					P1Flash->write<RuleRow_t>(tmpRow, MEM_RULES_OFFSET + row_index*RT_ROW_SIZE);
	#endif
					rowptr->data.flash_flag = SAVED; //toggle flash flag
				}
				else
				{
					LOGE(LOGTAG_MSG, "Error, cannot write Schedule row %d to flash, out of memory bounds", row_index);
					success_flag = false;
				}
			}
			rowptr = rowptr->next;
		}

		if (success_flag)
		{
			m_isRTChanged = false;
			LOGD(LOGTAG_MSG, "Rule table saved.");
			return true;
		}
		else
		{
			LOGE(LOGTAG_MSG, "Unable to write 1 or more Rule table rows to flash");
		}
	}

	return false;
}

// Load NodeID List
BOOL ConfigClass::LoadNodeIDList()
{
	BOOL rc = lstNodes.loadList();
	if( rc ) {
		LOGD(LOGTAG_MSG, "NodeList loaded - %d", lstNodes.count());
	} else {
		LOGW(LOGTAG_MSG, "Failed to load NodeList.");
	}
	return rc;
}

// Save NodeID List
BOOL ConfigClass::SaveNodeIDList()
{
	if( !IsNIDChanged() ) return true;

	BOOL rc = lstNodes.saveList();
	if( rc ) {
		LOGI(LOGTAG_MSG, "NodeList saved.");
	} else {
		LOGW(LOGTAG_MSG, "Failed to save NodeList.");
	}
	return rc;
}
