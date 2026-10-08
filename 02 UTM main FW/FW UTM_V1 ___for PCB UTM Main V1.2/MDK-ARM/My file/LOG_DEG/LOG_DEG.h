/**
  ******************************************************************************
  * @Date           : 05.11.2025
  * @Author			: Trong Huu
  * @Brief          : Define use SEGGER RTT Viewer debug
  ******************************************************************************
  * @UpdateLog


  * @Note


  ******************************************************************************
  */

#ifndef LOG_DEG_H_
#define LOG_DEG_H_

#include "SEGGER_RTT.h"

//----------------------------------
// Un-comment here to no debug log
//---------------------------------
#define USE_LOG_DEBUG

//----------------------------------
// while (LOG_DEG_KeyCheck());
//---------------------------------
#define LOG_DEG_KeyCheck()		(SEGGER_RTT_GetKey() <= 0 ? 1 : 0)

//----------------------------------
// wait forever for new ket press
// LOG_DEG_KeyWaitLock();
//---------------------------------
#define LOG_DEG_KeyWaitLock()	(SEGGER_RTT_WaitKey())


#ifdef USE_LOG_DEBUG
    #define LOG_DEG(...) SEGGER_RTT_printf(0, __VA_ARGS__)
#else
    #define LOG_DEG(...)
#endif


#endif /* END FILE */
