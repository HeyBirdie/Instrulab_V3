/*
  *****************************************************************************
  * @file    mcu_config.h
  * @author  Y3288231
  * @date    jan 15, 2014
  * @brief   Hardware abstraction for communication
  ***************************************************************************** 
*/ 
#ifndef STM32F4_CONFIG_H_
#define STM32F4_CONFIG_H_

#include "stm32f3xx_hal.h"
#include "stm32f3_discovery.h"
#include "firmware_version.h"
//#include "usb_device.h"
#include "math.h"
#include "err_list.h"

#define IDN_STRING "STM32F3-Discovery" //max 30 chars
#define MCU "STM32F303VT"

// Communication constatnts ===================================================
#define COMM_BUFFER_SIZE 256
#define UART_SPEED 115200

#define USART_GPIO GPIOA
#define USART_TX GPIO_PIN_2
#define USART_RX GPIO_PIN_3
#define USART_TX_PIN_STR "PA2_" //must be 4 chars
#define USART_RX_PIN_STR "PA3_" //must be 4 chars 

#define USB_DP_PIN_STR "PA12" //must be 4 chars
#define USB_DM_PIN_STR "PA11" //must be 4 chars

// Scope constatnts ===================================================
#define MAX_SAMPLING_FREQ 4000000 //smps
#define MAX_ADC_CHANNELS 4

#define MAX_SCOPE_BUFF_SIZE 20000 //in bytes
#define SCOPE_BUFFER_MARGIN 350

#define SCOPE_CH1_PIN_STR "PC0_" //must be 4 chars
#define SCOPE_CH2_PIN_STR "PC1_" //must be 4 chars
#define SCOPE_CH3_PIN_STR "PB13" //must be 4 chars
#define SCOPE_CH4_PIN_STR "PB12" //must be 4 chars

#define SCOPE_VREF 3300
#define SCOPE_VREF_INT (uint16_t)*((uint16_t *)0x1FFFF7BA)

#define RANGE_1_LOW 0
#define RANGE_1_HI SCOPE_VREF
#define RANGE_2_LOW -SCOPE_VREF
#define RANGE_2_HI SCOPE_VREF*2
#define RANGE_3_LOW 0
#define RANGE_3_HI 0
#define RANGE_4_LOW 0
#define RANGE_4_HI 0


//scope channels inputs
static const uint8_t ANALOG_DEFAULT_INPUTS[MAX_ADC_CHANNELS]={2,4,1,1};
static const uint8_t ANALOG_VREF_INPUTS[MAX_ADC_CHANNELS]={8,9,3,3};

#define ADC1_NUM_CHANNELS 9
static const uint16_t ANALOG_PIN_ADC1[ADC1_NUM_CHANNELS] = {				GPIO_PIN_0,			GPIO_PIN_1,			GPIO_PIN_0,			GPIO_PIN_1,			GPIO_PIN_2,			GPIO_PIN_3,			GPIO_PIN_11,		0,							0};
static GPIO_TypeDef * ANALOG_GPIO_ADC1[ADC1_NUM_CHANNELS] = {				GPIOA,					GPIOA,					GPIOC,					GPIOC,					GPIOC,					GPIOC,					GPIOB,					0,							0};
static const uint32_t ANALOG_CHANNEL_ADC1[ADC1_NUM_CHANNELS] = {		ADC_CHANNEL_1,	ADC_CHANNEL_2,	ADC_CHANNEL_6,	ADC_CHANNEL_7,	ADC_CHANNEL_8,	ADC_CHANNEL_9,	ADC_CHANNEL_14, ADC_CHANNEL_16, ADC_CHANNEL_18};
static const char* ANALOG_CHANN_ADC1_NAME[ADC1_NUM_CHANNELS] = { 		"PA0", 					"PA1", 					"PC0", 					"PC1", 					"PC2", 					"PC3", 					"PB11", 				"Temp", 				"Vref" };

#define ADC2_NUM_CHANNELS 10
static const uint16_t ANALOG_PIN_ADC2[ADC2_NUM_CHANNELS] = {				GPIO_PIN_6,			GPIO_PIN_7,			GPIO_PIN_4,			GPIO_PIN_0,			GPIO_PIN_1,			GPIO_PIN_2,			GPIO_PIN_3,			GPIO_PIN_5,			GPIO_PIN_2,			0};
static GPIO_TypeDef * ANALOG_GPIO_ADC2[ADC2_NUM_CHANNELS] = {				GPIOA,					GPIOA,					GPIOC,					GPIOC,					GPIOC,					GPIOC,					GPIOC,					GPIOC,					GPIOB,					0};
static const uint32_t ANALOG_CHANNEL_ADC2[ADC2_NUM_CHANNELS] = {		ADC_CHANNEL_3,	ADC_CHANNEL_4,	ADC_CHANNEL_5,	ADC_CHANNEL_6,	ADC_CHANNEL_7,	ADC_CHANNEL_8,	ADC_CHANNEL_9,	ADC_CHANNEL_11,	ADC_CHANNEL_12,	ADC_CHANNEL_18};
static const char* ANALOG_CHANN_ADC2_NAME[ADC2_NUM_CHANNELS] = { 		"PA6", 					"PA7", 					"PC4", 					"PC0", 					"PC1", 					"PC2", 					"PC3", 					"PC5", 					"PB2" 					"Vref"};

#define ADC3_NUM_CHANNELS 4
static const uint16_t ANALOG_PIN_ADC3[ADC3_NUM_CHANNELS] = {				GPIO_PIN_1,			GPIO_PIN_13,		GPIO_PIN_0,			0};
static GPIO_TypeDef * ANALOG_GPIO_ADC3[ADC3_NUM_CHANNELS] = {				GPIOB,					GPIOB,					GPIOB,					0};
static const uint32_t ANALOG_CHANNEL_ADC3[ADC3_NUM_CHANNELS] = {		ADC_CHANNEL_1,	ADC_CHANNEL_5,	ADC_CHANNEL_12,	ADC_CHANNEL_18};
static const char* ANALOG_CHANN_ADC3_NAME[ADC3_NUM_CHANNELS] = { 		"PB1", 					"PB13", 				"PB0", 					"Vref" };

#define ADC4_NUM_CHANNELS 4
static const uint16_t ANALOG_PIN_ADC4[ADC4_NUM_CHANNELS] = {				GPIO_PIN_13,		GPIO_PIN_14,		GPIO_PIN_15,		0};
static GPIO_TypeDef * ANALOG_GPIO_ADC4[ADC4_NUM_CHANNELS] = {				GPIOB,					GPIOB,					GPIOB,					0};					
static const uint32_t ANALOG_CHANNEL_ADC4[ADC4_NUM_CHANNELS] = {		ADC_CHANNEL_3,	ADC_CHANNEL_4,	ADC_CHANNEL_5,	ADC_CHANNEL_18};
static const char* ANALOG_CHANN_ADC4_NAME[ADC4_NUM_CHANNELS] = { 		"PB13", 				"PB14", 				"PB15", 				"Vref" };


static const uint8_t NUM_OF_ANALOG_INPUTS[MAX_ADC_CHANNELS]={ADC1_NUM_CHANNELS,ADC2_NUM_CHANNELS,ADC3_NUM_CHANNELS,ADC4_NUM_CHANNELS}; //number of ADC channels {ADC1,ADC2,ADC3,ADC4}



// Generator constatnts ===================================================
#define MAX_GENERATING_FREQ 2000000 //smps
#define MAX_DAC_CHANNELS 2
#define MAX_GENERATOR_BUFF_SIZE 2000
#define	DAC_DATA_DEPTH 12

#define GEN_VREF 3300
#define GEN_VDDA 3300
#define GEN_VREF_INT 1200

#define GEN_CH1_PIN_STR "PA4_" //must be 4 chars
#define GEN_CH2_PIN_STR "PA5_" //must be 4 chars



//Definition of assert to check length of strings
#define CASSERT(ex) {typedef char cassert_type[(ex)?1:-1];}


#endif /* STM32F4_CONFIG_H_ */
