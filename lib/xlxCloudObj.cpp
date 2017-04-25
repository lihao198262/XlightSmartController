/**
 * xlxCloudObj.cpp - Xlight Cloud Object library
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
 *
 * ToDo:
 * 1.
**/

#include "xlxCloudObj.h"
#include "xlxLogger.h"

//------------------------------------------------------------------
// Xlight Cloud Object Class
//------------------------------------------------------------------
CloudObjClass::CloudObjClass()
{
  m_SysID = "";
  m_SysVersion = "";
  m_SysStatus = STATUS_OFF;
  m_temperature = 0.0;
  m_humidity = 0.0;
  m_brightness = 0;
  m_motion = false;
  m_sound = false;
  m_gas = 0;
  m_dust = 0;
  m_smoke = 0;
  m_noise = 0;
  m_jpRoot = &(m_jBuf.createObject());
  m_strCldCmd = "";
}

// Initialize Cloud Variables & Functions
void CloudObjClass::InitCloudObj()
{
  if( m_jpRoot->success() ) {
    (*m_jpRoot)["device"] = m_SysID.c_str();
    m_jpData = &(m_jpRoot->createNestedObject("sensor"));
  } else {
    m_jpData = &(JsonObject::invalid());
  }

#ifdef USE_PARTICLE_CLOUD
  Particle.variable(CLV_SysID, &m_SysID, STRING);
  Particle.variable(CLV_TimeZone, &m_tzString, STRING);
  Particle.variable(CLV_SysStatus, &m_SysStatus, INT);
  Particle.variable(CLV_JSONData, &m_jsonData, STRING);
  Particle.variable(CLV_LastMessage, &m_lastMsg, STRING);

  Particle.function(CLF_SetTimeZone, &CloudObjClass::CldSetTimeZone, this);
  Particle.function(CLF_PowerSwitch, &CloudObjClass::CldPowerSwitch, this);
  Particle.function(CLF_JSONCommand, &CloudObjClass::CldJSONCommand, this);
  Particle.function(CLF_JSONConfig, &CloudObjClass::CldJSONConfig, this);
  Particle.function(CLF_SetCurTime, &CloudObjClass::CldSetCurrentTime, this);
#endif
}

String CloudObjClass::GetSysID()
{
	return m_SysID;
}

String CloudObjClass::GetSysVersion()
{
	return m_SysVersion;
}

int CloudObjClass::CldJSONCommand(String jsonCmd)
{
  if( m_cmdList.size() > MQ_MAX_CLOUD_MSG ) {
    LOGW(LOGTAG_MSG, "JSON commands exceeded queue size");
    return 0;
  }

  m_cmdList.add(jsonCmd);
  return 1;
}

int CloudObjClass::CldJSONConfig(String jsonData)
{
  if( m_configList.size() > MQ_MAX_CLOUD_MSG ) {
    LOGW(LOGTAG_MSG, "JSON config exceeded queue size");
    return 0;
  }

  m_configList.add(jsonData);
  return 1;
}

// Here we can either send/publish data on individual data entry basis,
/// or compose data entries into one larger json string and transfer it in main loop.
BOOL CloudObjClass::UpdateTemperature(float value)
{
  if( m_temperature != value ) {
    m_temperature = value;
    OnSensorDataChanged(sensorDHT);
    if( m_jpData->success() )
    {
      (*m_jpData)["DHTt"] = value;
    }
    return true;
  }
  return false;
}

BOOL CloudObjClass::UpdateHumidity(float value)
{
  if( m_humidity != value ) {
    m_humidity = value;
    OnSensorDataChanged(sensorDHT_h);
    if( m_jpData->success() )
    {
      (*m_jpData)["DHTh"] = value;
    }
    return true;
  }
  return false;
}

BOOL CloudObjClass::UpdateBrightness(uint8_t nid, uint8_t value)
{
  if( m_brightness != value ) {
    m_brightness = value;
    OnSensorDataChanged(sensorALS);
    /*
    if( m_jpData->success() )
    {
      (*m_jpData)["ALS"] = value;
    }
    */
#ifdef USE_PARTICLE_CLOUD
    // Publis right away
    if( Particle.connected() ) {
      String strTemp = String::format("{'nd':%d,'ALS':%d}", nid, value);
      Particle.publish(CLT_NAME_SensorData, strTemp, CLT_TTL_MotionData, PRIVATE);
    }
#endif
    return true;
  }
  return false;
}

BOOL CloudObjClass::UpdateMotion(uint8_t nid, bool value)
{
  if( m_motion != value ) {
    m_motion = value;
    OnSensorDataChanged(sensorPIR);
    /*
    if( m_jpData->success() )
    {
      (*m_jpData)["PIR"] = value;
    }
    */
#ifdef USE_PARTICLE_CLOUD
    // Publis right away
    if( Particle.connected() ) {
      String strTemp = String::format("{'nd':%d,'PIR':%d}", nid, value);
      Particle.publish(CLT_NAME_SensorData, strTemp, CLT_TTL_MotionData, PRIVATE);
    }
#endif
    return true;
  }
  return false;
}

BOOL CloudObjClass::UpdateGas(uint8_t nid, uint16_t value)
{
  if( m_gas != value ) {
    m_gas = value;
    OnSensorDataChanged(sensorGAS);
    /*
    if( m_jpData->success() )
    {
      (*m_jpData)["GAS"] = value;
    }
    */
#ifdef USE_PARTICLE_CLOUD
    // Publis right away
    if( Particle.connected() ) {
      String strTemp = String::format("{'nd':%d,'GAS':%d}", nid, value);
      Particle.publish(CLT_NAME_SensorData, strTemp, CLT_TTL_MotionData, PRIVATE);
    }
#endif
    return true;
  }
  return false;
}

BOOL CloudObjClass::UpdateDust(uint8_t nid, uint16_t value)
{
  if( m_dust != value ) {
    m_dust = value;
    OnSensorDataChanged(sensorDUST);
    /*
    if( m_jpData->success() )
    {
      (*m_jpData)["PM25"] = value;
    }
    */
#ifdef USE_PARTICLE_CLOUD
    // Publis right away
    if( Particle.connected() ) {
      String strTemp = String::format("{'nd':%d,'PM25':%d}", nid, value);
      Particle.publish(CLT_NAME_SensorData, strTemp, CLT_TTL_MotionData, PRIVATE);
    }
#endif
    return true;
  }
  return false;
}

BOOL CloudObjClass::UpdateSmoke(uint8_t nid, uint16_t value)
{
  if( m_smoke != value ) {
    m_smoke = value;
    OnSensorDataChanged(sensorSMOKE);
    /*
    if( m_jpData->success() )
    {
      (*m_jpData)["SMOKE"] = value;
    }
    */
#ifdef USE_PARTICLE_CLOUD
    // Publis right away
    if( Particle.connected() ) {
      String strTemp = String::format("{'nd':%d,'SMOKE':%d}", nid, value);
      Particle.publish(CLT_NAME_SensorData, strTemp, CLT_TTL_MotionData, PRIVATE);
    }
#endif
    return true;
  }
  return false;
}

BOOL CloudObjClass::UpdateSound(uint8_t nid, bool value)
{
  if( m_sound != value ) {
    m_sound = value;
    OnSensorDataChanged(sensorMIC_b);
    /*
    if( m_jpData->success() )
    {
      (*m_jpData)["PIR"] = value;
    }
    */
#ifdef USE_PARTICLE_CLOUD
    // Publis right away
    if( Particle.connected() ) {
      String strTemp = String::format("{'nd':%d,'MIC':%d}", nid, value);
      Particle.publish(CLT_NAME_SensorData, strTemp, CLT_TTL_MotionData, PRIVATE);
    }
#endif
    return true;
  }
  return false;
}

BOOL CloudObjClass::UpdateNoise(uint8_t nid, uint16_t value)
{
  if( m_noise != value ) {
    m_noise = value;
    OnSensorDataChanged(sensorMIC);
    /*
    if( m_jpData->success() )
    {
      (*m_jpData)["MIC"] = value;
    }
    */
#ifdef USE_PARTICLE_CLOUD
    // Publis right away
    if( Particle.connected() ) {
      String strTemp = String::format("{'nd':%d,'NOISE':%d}", nid, value);
      Particle.publish(CLT_NAME_SensorData, strTemp, CLT_TTL_MotionData, PRIVATE);
    }
#endif
    return true;
  }
  return false;
}

// Compose JSON Data String
void CloudObjClass::UpdateJSONData()
{
  static UL lastTick = millis();

  // Publish sensor data
#ifdef USE_PARTICLE_CLOUD
  if( Particle.connected() ) {
    if( m_jpData->success() ) {
      char buf[512];
      m_jpData->printTo(buf, 512);
      String strTemp = buf;
      if( strTemp.length() > 5 ) {
        if( m_jsonData != strTemp || (millis() - lastTick) / 1000 >= RTE_DELAY_PUBLISH ) {
          m_jsonData = strTemp;
          lastTick = millis();
          Particle.publish(CLT_NAME_SensorData, strTemp, CLT_TTL_SensorData, PRIVATE);
        }
      }
    }
  }
#endif
}

// Publish LOG message and update cloud variable
BOOL CloudObjClass::PublishLog(const char *msg)
{
  BOOL rc = true;
  m_lastMsg = msg;
#ifdef USE_PARTICLE_CLOUD
  if( Particle.connected() ) {
    rc = Particle.publish(CLT_NAME_LOGMSG, msg, CLT_TTL_LOGMSG, PRIVATE);
  }
#endif
  return rc;
}

// Publish Device status
BOOL CloudObjClass::PublishDeviceStatus(const char *msg)
{
  BOOL rc = true;
#ifdef USE_PARTICLE_CLOUD
  if( Particle.connected() ) {
    rc = Particle.publish(CLT_NAME_DeviceStatus, msg, CLT_TTL_DeviceStatus, PRIVATE);
  }
#endif
  return rc;
}

void CloudObjClass::GotNodeConfigAck(const UC _nodeID, const UC *data)
{
	String strTemp;

  strTemp = String::format("{'node_id':%d,'ver':%d,'type':%d,'senMap':%d,'funcMap':%d,'data':[%d,%d,%d,%d,%d,%d]}",
			 _nodeID, data[0], data[1],	data[2] + data[3]*256, data[4] + data[5]*256,
       data[6], data[7], data[8], data[9], data[10], data[11]);
	PublishDeviceConfig(strTemp.c_str());
}

// Publish Device Config
BOOL CloudObjClass::PublishDeviceConfig(const char *msg)
{
  BOOL rc = true;
#ifdef USE_PARTICLE_CLOUD
  if( Particle.connected() ) {
    rc = Particle.publish(CLT_NAME_DeviceConfig, msg, CLT_TTL_DeviceConfig, PRIVATE);
  }
#endif
  return rc;
}

// Publish Alarm
BOOL CloudObjClass::PublishAlarm(const char *msg)
{
  BOOL rc = true;
#ifdef USE_PARTICLE_CLOUD
  if( Particle.connected() ) {
    rc = Particle.publish(CLT_NAME_Alarm, msg, CLT_TTL_Alarm, PRIVATE);
  }
#endif
  return rc;
}

// Concatenate string with regard to the length limitation of cloud API
/// Return value:
/// 0 - string is intact, can be executed
/// 1 - waiting for more input
/// -1 - error
int CloudObjClass::ProcessJSONString(String inStr)
{
  String strTemp = inStr;
  StaticJsonBuffer<COMMAND_JSON_SIZE * 8> lv_jBuf;
  m_jpCldCmd = &(lv_jBuf.parseObject(const_cast<char*>(strTemp.c_str())));
  if (!m_jpCldCmd->success())
  {
		if( m_strCldCmd.length() > 0 ) {

			m_strCldCmd.concat(inStr);
			String debugstr = m_strCldCmd;
			StaticJsonBuffer<COMMAND_JSON_SIZE*3 * 8> lv_jBuf2; //buffer size must be larger than COMMAND_JSON_SIZE
			m_jpCldCmd = &(lv_jBuf2.parseObject(const_cast<char*>(m_strCldCmd.c_str())));
			m_strCldCmd = "";		// Already concatenated
			if (!m_jpCldCmd->success()) {
				SERIAL_LN("Could not parse the concatenated string: %s", debugstr.c_str());
				return -1;
			}
		} else {
			return -1;
		}
  }

	if (m_jpCldCmd->containsKey("x0") ) {
		// Begin of a new string
		m_strCldCmd = (*m_jpCldCmd)["x0"];
		return 1;
	}
	else if (m_jpCldCmd->containsKey("x1")) {
		// Concatenate
		strTemp = (*m_jpCldCmd)["x1"];
		m_strCldCmd.concat(strTemp);
		return 1;
  } else if( m_strCldCmd.length() > 0 ) {
		m_strCldCmd.concat(inStr);
		String debugstr = m_strCldCmd;
		StaticJsonBuffer<COMMAND_JSON_SIZE * 8> lv_jBuf3;
		m_jpCldCmd = &(lv_jBuf3.parseObject(const_cast<char*>(m_strCldCmd.c_str())));

		m_strCldCmd = "";		// Already concatenated
		if (!m_jpCldCmd->success()) {
			SERIAL_LN("Could not parse the string: %s", debugstr.c_str());
			return -1;
		}
  }

  return 0;
}
