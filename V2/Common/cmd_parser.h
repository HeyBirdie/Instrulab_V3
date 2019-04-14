/*
  *****************************************************************************
  * @file    cmd_parser.h
  * @author  Y3288231
  * @date    Dec 15, 2014
  * @brief   This file contains functions for parsing commands
  ***************************************************************************** 
*/ 
#ifndef CMD_PARSER_H_
#define CMD_PARSER_H_

/** @addtogroup Parser
  * @{
  */
	
// Constant definitions =======================================================
extern xQueueHandle cmdParserMessageQueue;
	
// Types definitions ==========================================================
// Exported variables =========================================================
// Exported functions =========================================================
/** @defgroup Parser_Prototypes Parser Function Prototypes
  * @{
  */
void CmdParserTask(void const *argument);
/**
  * @}
  */

#endif /* CMD_PARSER_H_ */

/**
  * @}
  */
