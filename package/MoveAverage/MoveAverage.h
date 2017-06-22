//  MoveAverage.h - Moving average lib is a base class for sensor data processing

#ifndef DTIT_MOVEAVERAGE_INCLUDED_
#define DTIT_MOVEAVERAGE_INCLUDED_

#include "application.h"

class CMoveAverage
{
public:
  CMoveAverage(uint8_t f_iSize = 5);
  ~CMoveAverage();

  bool AddData(const float f_data);
  bool IsDataReady();
  float GetValue();

private:
  // Moving average
  uint8_t m_ptr;
  float *m_pData;
  float m_Sum;
  float m_Value;
  uint8_t m_nSize;						// Buffer size
  bool m_bReady;
};

#endif
