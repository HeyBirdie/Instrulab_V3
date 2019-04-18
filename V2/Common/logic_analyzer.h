/**
  *****************************************************************************
  * @file    logic_analyzer.h
  * @author  HeyBirdie
  * @date    Nov 4, 2017
  * @brief   This file contains definitions and prototypes of logic analyzer
						 functions.
  ***************************************************************************** 
*/ 

#ifdef USE_LOG_ANLYS
#ifndef LOG_ANLYS_H_
#define LOG_ANLYS_H_

/* Includes */
#include <stdint.h>
#include "stm32f3xx_hal.h"

// Constant definitions =======================================================
//#define LOG_ANLYS_DEFAULT_SAMPLING_FREQ 	1000
//#define LOG_ANLYS_DEFAULT_PRETRIGGER 			0x8000    //[ms]
//#define LOG_ANLYS_DEFAULT_DATA_LEN 50   // SHOULD BE SET TO 100 !!!

/** @addtogroup Logic_Analyzer
  * @{
  */

/* Enums */
/** @defgroup Logic_Analyzer_Enums Logic analyzer enums
  * @{
  */
	
	/**
  * @brief  LA enable state - ENABLED, DISABLED. Used due to shared resources.
	*					E.g. Counter uses the same ISR (TIMx) as Logic analyzer.
  */
typedef enum{
	LOGA_DISABLED = 0,
	LOGA_ENABLED
}enableTypeDef;

	/**
  * @brief  LA modes.
  */
typedef enum{
	LOGA_MODE_AUTO = 0,
	LOGA_MODE_NORMAL = 1,
	LOGA_MODE_SINGLE = 2
}triggerModeTypeDef;

	/**
  * @brief  LA state. 
  */
typedef enum{
	LOGA_IDLE = 0,    			  // during and after initialization & after deinit
	LOGA_SAMPLING,					  // sampling started
	LOGA_DATA_SENDING,			  // data sending to PC
	LOGA_DATA_SENT,			  		// data sending to PC
	LOGA_WAIT_FOR_RESTART,	  // data sent to PC & wating for start command from host
	LOGA_DONE,
	LOGA_ERR
}stateTypeDef;

	/**
  * @brief  LA trigger channel selection. 
  */
typedef enum{
	TRIG_CHAN1 = 0,
	TRIG_CHAN2,
	TRIG_CHAN3,
	TRIG_CHAN4,
	TRIG_CHAN5,
	TRIG_CHAN6,
	TRIG_CHAN7,
	TRIG_CHAN8
}trigConfigTypeDef;

	/**
  * @brief  LA trigger edge selection. 
  */
typedef enum{
	TRIG_EDGE_RISING = 0,
	TRIG_EDGE_FALLING
}trigEdgeTypeDef;

	/**
  * @brief  LA trigger occurence flag. 
  */
typedef enum{
	TRIG_OCCURRED = 0,
	TRIG_NOT_OCCURRED
}trigOccurrenceTypeDef;
/**
  * @}
  */

/** @defgroup Logic_Analyzer_Struct_Definition LA Structure Definition
  * @{
  */

	/**
  * @brief  Logic Analyzer Struct
  */
typedef struct{		
	uint32_t triggerPointer;
	uint32_t samplingFreq;
	uint16_t preTriggerTime;
	uint16_t *bufferMemory;
	uint16_t samplesNumber;
//	uint16_t userTrigger;  // user trigger chosen in PC app associated with the data
	
	stateTypeDef state;
	enableTypeDef enable;
	triggerModeTypeDef triggerMode;
	trigConfigTypeDef trigConfig;
	trigEdgeTypeDef trigEdge;
	trigOccurrenceTypeDef trigOccur;
}logAnlysTypeDef;
/**
  * @}
  */

// Externs ===========================================================
extern volatile logAnlysTypeDef logAnlys;

/** @defgroup Logic_Analyzer_Prototypes LA Function Prototypes
  * @{
  */
void LogAnlysTask(void const *argument);

void logAnlysSendInit(void);
void logAnlysSendDeinit(void);
void logAnlysSendStart(void);
void logAnlysSendStop(void);

void logAnlysSendTriggerRising(void);
void logAnlysSendTriggerFalling(void);

void logAnlysInit(void);
void logAnlysDeinit(void);
void logAnlysStart(void);
void logAnlysStop(void);

void logAnlysSetSamplingFreq(uint32_t arrPsc);
void logAnlysSetSamplesNum(uint16_t samplesNum);

void logAnlysSetPosttrigger(uint32_t arrPsc);
void logAnlysSetPretrigger(uint32_t timeInMilliseconds);
//void logAnlysUserTrigger(uint16_t userTrigger);		// user trigger sent from app to mcu and then back to app with data

void logAnlysSetTriggerChannel(uint32_t chan);
void logAnlysSetTriggerRising(void);
void logAnlysSetTriggerFalling(void);
void logAnlysDisablePostTrigIRQ(void);

void logAnlysSetDefault(void);

void logAnlysPeriodElapsedCallback(void);
void triggerEventOccuredCallback(void);	

/**
  * @}
  */

#endif /* LOG_ANLYS__H_ */

#endif //USE_LOG_ANLYS

/**
  * @}
  */



