/**
 * MoveAverage.cpp - Moving average lib is a base class for sensor data processing
 *
 * Created by Baoshi Sun <bs.sun@datatellit.com>
 * Copyright (C) 2015-2017 DTIT
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
 *
 * DESCRIPTION
 *
 * ToDo:
 *
**/

#include "MoveAverage.h"

CMoveAverage::CMoveAverage(uint8_t f_iSize)
{
  m_pData = NULL;
  if( f_iSize > 0 ) m_pData = new float[f_iSize];
	if( m_pData ) {
		m_nSize = f_iSize;
    memset(m_pData, 0x00, sizeof(float) * f_iSize);
  }
	else
		m_nSize = 0;

	m_ptr = 0;
	m_Sum = 0;
  m_Value = 0;
	m_bReady = false;
}

CMoveAverage::~CMoveAverage()
{
	if( m_pData ) {
		delete[] m_pData;
    m_pData = NULL;
  }
  m_nSize = 0;
  m_ptr = 0;
	m_Sum = 0;
	m_bReady = false;
}

bool CMoveAverage::AddData(const float f_data)
{
  if( !m_pData ) return false;

  if( f_data != m_pData[m_ptr] ) {
    m_Sum += f_data;
    m_Sum -= m_pData[m_ptr];
    m_pData[m_ptr] = f_data;
  }
  m_ptr = (m_ptr + 1) % m_nSize;
  if( !m_bReady ) {
    m_bReady = (m_ptr == 0);
  }

  if( m_bReady ) m_Value = m_Sum / m_nSize;
  return m_bReady;
}

bool CMoveAverage::IsDataReady()
{
  return m_bReady;
}

float CMoveAverage::GetValue()
{
  return m_Value;
}
