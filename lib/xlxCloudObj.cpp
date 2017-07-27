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

#include "MyMessage.h"
#include "xlxCloudObj.h"
#include "xlxLogger.h"
#include "xlxBLEInterface.h"

//------------------------------------------------------------------
// Xlight Cloud Object Class
//------------------------------------------------------------------
CloudObjClass::CloudObjClass()
{
  m_SysID = "";
  m_SysVersion = "";
  m_nAppVersion = VERSION_CONFIG_DATA;
  m_SysStatus = STATUS_OFF;

  m_temperature.node_id = 0;
  m_temperature.data = 0.0;

  m_humidity.node_id = 0;
  m_humidity.data = 0.0;

  m_brightness.node_id = 0;
  m_brightness.data = 0;

  m_motion.node_id = 0;
  m_motion.data = false;

  m_irKey.node_id = 0;
  m_irKey.data = false;

  m_sound.node_id = 0;
  m_sound.data = false;

  m_gas.node_id = 0;
  m_gas.data = 0;

  m_dust.node_id = 0;
  m_dust.data = 0;

  m_smoke.node_id = 0;
  m_smoke.data = 0;

  m_noise.node_id = 0;
  m_noise.data = 0;

  m_strCldCmd = "";
}

// Initialize Cloud Variables & Functions
void CloudObjClass::InitCloudObj()
{
#ifdef USE_PARTICLE_CLOUD
  Particle.variable(CLV_SysID, &m_SysID, STRING);
  Particle.variable(CLV_AppVersion, &m_nAppVersion, INT);
  Particle.variable(CLV_TimeZone, &m_tzString, STRING);
  Particle.variable(CLV_SysStatus, &m_SysStatus, INT);
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

BOOL CloudObjClass::UpdateDHT(uint8_t nid, float _temp, float _humi)
{
  static float preTemp = 255;
  static float preHumi = 255;

  if( _temp > 100 && (_humi > 100 || _humi < 0) ) return false;

  BOOL temp_ok = false;
  BOOL humi_ok = false;
  if( nid > 0 ) {
    if( _humi >= 0 && _humi <= 100 ) {
      if( m_humidity.data != _humi || m_humidity.node_id != nid ) {
        m_humidity.node_id = nid;
        m_humidity.data = _humi;
        humi_ok = true;
      }
    }
    if( _temp <= 100 ) {
      if( m_temperature.data != _temp || m_temperature.node_id != nid )
      {
        m_temperature.node_id = nid;
        m_temperature.data = _temp;
        temp_ok = true;
      }
    }
  } else {
    if( _humi >= 0 && _humi <= 100 ) {
      if( m_sysHumi.AddData(_humi) ) {
        _humi = m_sysHumi.GetValue();
        if( _humi != preHumi ) {
          preHumi = _humi;
          humi_ok = true;
        }
      }
    }
    if( _temp <= 100 ) {
      if( m_sysTemp.AddData(_temp) ) {
        _temp = m_sysTemp.GetValue();
        if( _temp != preTemp ) {
          preTemp = _temp;
          temp_ok = true;
        }
      }
    }
  }

  if( temp_ok ) OnSensorDataChanged(sensorDHT, nid);
  if( humi_ok ) OnSensorDataChanged(sensorDHT_h, nid);

#ifdef USE_PARTICLE_CLOUD
  // Publis right away
  if( Particle.connected() && (temp_ok || humi_ok) ) {
    String strTemp;

    // Temperature Message
    if( humi_ok && _temp < 100 ) temp_ok = true;

    if( temp_ok && humi_ok ) {
      strTemp = String::format("{'nd':%d,'DHTt':%.2f,'DHTh':%.2f}", nid, _temp, _humi);
    } else if( temp_ok ) {
      strTemp = String::format("{'nd':%d,'DHTt':%.2f}", nid, _temp);
    } else {
      strTemp = String::format("{'nd':%d,'DHTh':%.2f}", nid, _humi);
    }
    Particle.publish(CLT_NAME_SensorData, strTemp, CLT_TTL_SensorData, PRIVATE);
  }
#endif

  return true;
}

BOOL CloudObjClass::UpdateBrightness(uint8_t nid, uint8_t value)
{
  if( m_brightness.data != value || m_brightness.node_id != nid ) {
    m_brightness.node_id = nid;
    m_brightness.data = value;
    OnSensorDataChanged(sensorALS, nid);
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

BOOL CloudObjClass::UpdateMotion(uint8_t nid, uint8_t sensor, uint8_t value)
{
  if( sensor == S_MOTION || sensor == S_IR ) {
    if( sensor == S_MOTION ) {
      if( m_motion.data != value || m_motion.node_id != nid ) {
        m_motion.node_id = nid;
        m_motion.data = value;
        OnSensorDataChanged(sensorPIR, nid);
      }
    } else if( sensor == S_IR ) {
      if( m_irKey.data != value || m_irKey.node_id != nid ) {
        m_irKey.node_id = nid;
        m_irKey.data = value;
        OnSensorDataChanged(sensorIRKey, nid);
      }
    }

#ifdef USE_PARTICLE_CLOUD
    // Publis right away
    if( Particle.connected() ) {
      String strTemp = String::format("{'nd':%d,'%s':%d}", nid, (sensor == S_MOTION ? "PIR" : "IRK"), value);
      Particle.publish(CLT_NAME_SensorData, strTemp, CLT_TTL_MotionData, PRIVATE);
    }
#endif
    return true;
  }
  return false;
}

BOOL CloudObjClass::UpdateGas(uint8_t nid, uint16_t value)
{
  if( m_gas.data != value || m_gas.node_id != nid ) {
    m_gas.node_id = nid;
    m_gas.data = value;
    OnSensorDataChanged(sensorGAS, nid);
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
  if( m_dust.data != value || m_dust.node_id != nid ) {
    m_dust.node_id = nid;
    m_dust.data = value;
    OnSensorDataChanged(sensorDUST, nid);
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
  if( m_smoke.data != value || m_smoke.node_id != nid ) {
    m_smoke.node_id = nid;
    m_smoke.data = value;
    OnSensorDataChanged(sensorSMOKE, nid);
#ifdef USE_PARTICLE_CLOUD
    // Publis right away
    if( Particle.connected() ) {
      String strTemp = String::format("{'nd':%d,'SMK':%d}", nid, value);
      Particle.publish(CLT_NAME_SensorData, strTemp, CLT_TTL_MotionData, PRIVATE);
    }
#endif
    return true;
  }
  return false;
}

BOOL CloudObjClass::UpdateSound(uint8_t nid, uint8_t value)
{
  if( m_sound.data != value || m_sound.node_id != nid ) {
    m_sound.node_id = nid;
    m_sound.data = value;
    OnSensorDataChanged(sensorMIC_b, nid);
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
  if( m_noise.data != value || m_noise.node_id != nid ) {
    m_noise.node_id = nid;
    m_noise.data = value;
    OnSensorDataChanged(sensorMIC, nid);
#ifdef USE_PARTICLE_CLOUD
    // Publis right away
    if( Particle.connected() ) {
      String strTemp = String::format("{'nd':%d,'NOS':%d}", nid, value);
      Particle.publish(CLT_NAME_SensorData, strTemp, CLT_TTL_MotionData, PRIVATE);
    }
#endif
    return true;
  }
  return false;
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

#ifndef DISABLE_BLE
  // Notify via BLE
  //if( theBLE.isGood() ) theBLE.sendNotification(CLT_ID_LOGMSG, msg);
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

#ifndef DISABLE_BLE
  // Notify via BLE
  if( theBLE.isGood() ) theBLE.sendNotification(CLT_ID_DeviceStatus, msg);
#endif

  return rc;
}

void CloudObjClass::GotNodeConfigAck(const UC _nodeID, const UC *data)
{
	String strTemp;

  strTemp = String::format("{'nd':%d,'ver':%d,'tp':%d,'senMap':%d,'funcMap':%d,'data':[%d,%d,%d,%d,%d,%d]}",
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

#ifndef DISABLE_BLE
  // Notify via BLE
  if( theBLE.isGood() ) theBLE.sendNotification(CLT_ID_DeviceConfig, msg);
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

#ifndef DISABLE_BLE
  // Notify via BLE
  if( theBLE.isGood() ) theBLE.sendNotification(CLT_ID_Alarm, msg);
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
