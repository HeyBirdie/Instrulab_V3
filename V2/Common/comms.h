/**
  *****************************************************************************
  * @file    comms.h
  * @author  Y3288231
  * @date    Dec 15, 2014
  * @brief   This file contains prototypes of communication functions
  ***************************************************************************** 
*/ 
#ifndef COMMS_H_
#define COMMS_H_

/** @addtogroup Comms
  * @{
  */

// Constant definitions =======================================================
// Types definitions ==========================================================
/** @defgroup Comms_Enums Comms enums.
  * @{
  */
typedef enum
{
	BUFF_EMPTY = 0,
	BUFF_DATA,
	BUFF_FULL
}commBufferState;
/**
  * @}
  */

/** @defgroup Comms_Struct_Definition Comms Structure Definition
  * @{
  */
typedef struct
{
  uint8_t * memory;
  uint16_t bufferSize;
  uint16_t writePointer;
  uint16_t readPointer;
  commBufferState state;
} commBuffer;
/**
  * @}
  */

// Exported variables =========================================================
extern xQueueHandle messageQueue;

/** @defgroup Comms_Prototypes Comms Function Prototypes
  * @{
  */
// Exported functions =========================================================
void LLCommTask(void const *argument);
void CommTask(void const *argument);
uint8_t commBufferReadByte(uint8_t *ret);
uint8_t commBufferReadNBytes(uint8_t *mem, uint16_t count);
uint16_t commBufferLookNewBytes(uint8_t *mem);
uint16_t commBufferCounter(void);
void commsInit(void);
uint8_t commInputByte(uint8_t chr);
uint16_t getBytesAvailable(void);

/**
  * @}
  */

#endif /* COMMS_H_ */

/**
  * @}
  */
