/**
  ******************************************************************************
  * @file    stm32f4xx_it.c
  * @date    18/01/2015 10:00:32
  * @brief   Interrupt Service Routines.
  ******************************************************************************
  *
  * COPYRIGHT(c) 2015 STMicroelectronics
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "stm32f3xx_hal.h"
#include "stm32f3xx.h"
#include "stm32f3xx_it.h"
#include "cmsis_os.h"
#include "comms_hal.h"
#include "counter.h"
#include "tim.h"
#include "logic_analyzer.h"

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */
/* External variables --------------------------------------------------------*/
 
extern void xPortSysTickHandler(void);
void LOG_ANLYS_handle_interrupt(uint32_t pf);

#ifdef USE_USB
extern PCD_HandleTypeDef hpcd_USB_FS;
#endif //USE_USB
extern UART_HandleTypeDef huart2;
extern DMA_HandleTypeDef hdma_usart2_rx;
extern DMA_HandleTypeDef hdma_usart2_tx;

#ifdef USE_COUNTER

#endif //USE_COUNTER

/******************************************************************************/
/*            Cortex-M4 Processor Interruption and Exception Handlers         */ 
/******************************************************************************/

/**
* @brief This function handles System tick timer.
*/
void SysTick_Handler(void)
{
  /* USER CODE BEGIN SysTick_IRQn 0 */

  /* USER CODE END SysTick_IRQn 0 */
  if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) {
    xPortSysTickHandler();
  }
  HAL_IncTick();
  /* USER CODE BEGIN SysTick_IRQn 1 */

  /* USER CODE END SysTick_IRQn 1 */
}

/**
* @brief This function handles USB On The Go FS global interrupt.
*/

#ifdef USE_USB
/**
* @brief This function handles USB Low Priority or CAN_RX0 interrupts.
*/
void USB_LP_CAN_RX0_IRQHandler(void)
{
  /* USER CODE BEGIN USB_LP_CAN_RX0_IRQn 0 */

  /* USER CODE END USB_LP_CAN_RX0_IRQn 0 */
  HAL_NVIC_ClearPendingIRQ(USB_LP_CAN_RX0_IRQn);
  HAL_PCD_IRQHandler(&hpcd_USB_FS);
  /* USER CODE BEGIN USB_LP_CAN_RX0_IRQn 1 */

  /* USER CODE END USB_LP_CAN_RX0_IRQn 1 */
}
#endif //USE_USB

/**
* @brief This function handles USART2 global interrupt.
*/
void USART2_IRQHandler(void)
{
  /* USER CODE BEGIN USART2_IRQn 0 */
	char c = (uint16_t)(USART2->RDR & (uint16_t)0x01FF);
	commsRecieveUART(c);
	HAL_NVIC_ClearPendingIRQ(USART2_IRQn);
  /* USER CODE END USART2_IRQn 0 */
  
  /* USER CODE BEGIN USART2_IRQn 1 */

  /* USER CODE END USART2_IRQn 1 */
}

/* USER CODE BEGIN 1 */
/**
* @brief This function is not used. Handles DMA1 channel6 global interrupt.
*/
void DMA1_Channel6_IRQHandler(void)
{
  /* USER CODE BEGIN DMA1_Channel6_IRQn 0 */

  /* USER CODE END DMA1_Channel6_IRQn 0 */
  //HAL_DMA_IRQHandler(&hdma_usart2_rx);
  /* USER CODE BEGIN DMA1_Channel6_IRQn 1 */
  /* USER CODE END DMA1_Channel6_IRQn 1 */
}

#ifdef USE_COUNTER
/**
* @brief This function handles DMA1 channel2 global interrupt.
*/
void DMA1_Channel2_IRQHandler(void)
{
  HAL_DMA_IRQHandler(&hdma_tim2_up);
}
#endif //USE_COUNTER


#ifdef USE_LOG_ANLYS
/**
* @brief This function handles DMA1 channel7 global interrupt for Logic Analyzer.
*	Saves DMA CNDTR pointer state to find out later where the trigger occured.
* NOT USED ANYMORE !!!
*/
void DMA1_Channel7_IRQHandler(void)
{
  HAL_DMA_IRQHandler(&hdma_tim4_up);
	//HAL_DMA_IRQHandler(&hdma_usart2_tx);
	//huart2.gState =HAL_UART_STATE_READY;
//	  HAL_DMA_IRQHandler(&hdma_usart2_tx);
}

extern DMA_HandleTypeDef hdma_tim1_up;

/**
  * @brief  Logic Analyzer ISR triggered by an incoming signal edge on GPIO.
	*					Stores time of trigger and starts posttrigger time (TIM4) in order to sample the rest.
  */
void EXTI15_10_IRQHandler(void){
	TIM_PostTrigger_SoftwareStart();
	logAnlys.triggerPointer = hdma_tim1_up.Instance->CNDTR;	
	LOG_ANLYS_handle_interrupt(EXTI->PR & 0x3fc0);
}

/**
  * @brief  Logic Analyzer ISR triggered by an incoming signal edge on GPIO.
	*					Stores time of trigger and starts posttrigger time (TIM4) in order to sample the rest.
  */
void EXTI9_5_IRQHandler(void){
	TIM_PostTrigger_SoftwareStart();
	logAnlys.triggerPointer = hdma_tim1_up.Instance->CNDTR;	
	LOG_ANLYS_handle_interrupt(EXTI->PR & 0x3fc0); //mask the pending requests to get interrupts from selected pins only
}

/**
  * @brief  Logic Analyzer callback called from EXTI9_5_IRQHandler or EXTI15_10_IRQHandler.
	*					Distinguishes the source of the trigger.
  */
void LOG_ANLYS_handle_interrupt(uint32_t pr){
	uint8_t isRightPin = 0;
	
	if(pr & (1 << 6) ){ 	//pending request on pin 6
		__HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_6);
		if(logAnlys.trigConfig == TRIG_CHAN1){
			HAL_NVIC_DisableIRQ(EXTI9_5_IRQn);
			isRightPin = 1;
		}
	}
	if(pr & (1 << 7) ){ 	//pending request on pin 7
		__HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_7);
		if(logAnlys.trigConfig == TRIG_CHAN2){
			HAL_NVIC_DisableIRQ(EXTI9_5_IRQn);
			isRightPin = 1;
		}
	}
	if(pr & (1 << 8) ){ 	//pending request on pin 8
		__HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_8);
		if(logAnlys.trigConfig == TRIG_CHAN3){
			HAL_NVIC_DisableIRQ(EXTI9_5_IRQn);
			isRightPin = 1;
		}
	}
	if(pr & (1 << 9) ){ 	//pending request on pin 9
		__HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_9);
		if(logAnlys.trigConfig == TRIG_CHAN4){
			HAL_NVIC_DisableIRQ(EXTI9_5_IRQn);
			isRightPin = 1;
		}
	}
	if(pr & (1 << 10) ){ 	//pending request on pin 10
		__HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_10);
		if(logAnlys.trigConfig == TRIG_CHAN5){
			HAL_NVIC_DisableIRQ(EXTI15_10_IRQn);
			isRightPin = 1;
		}
	}
	if(pr & (1 << 11) ){ 	//pending request on pin 11
		__HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_11);
		if(logAnlys.trigConfig == TRIG_CHAN6){
			HAL_NVIC_DisableIRQ(EXTI15_10_IRQn);
			isRightPin = 1;
		}
	}
	if(pr & (1 << 12) ){ 	//pending request on pin 12
		__HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_12);
		if(logAnlys.trigConfig == TRIG_CHAN7){
			HAL_NVIC_DisableIRQ(EXTI15_10_IRQn);
			isRightPin = 1;
		}
	}
	if(pr & (1 << 13) ){ 	//pending request on pin 13
		__HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_13);
		if(logAnlys.trigConfig == TRIG_CHAN8){
			HAL_NVIC_DisableIRQ(EXTI15_10_IRQn);
			isRightPin = 1;
		}
	}
	
	if(isRightPin == 1){		
		logAnlys.trigOccur = TRIG_OCCURRED;
//		TIM_PostTrigger_SoftwareStart();
	}
	else{
		//stop TIM4 and reset		
		//HAL_TIM_Base_Stop(&htim4);
		TIM4->CR1 &= ~(TIM_CR1_CEN);
		__HAL_TIM_SET_COUNTER(&htim4, 0x00);		
	}
}


#endif //USE_LOG_ANLYS

/**
  * @brief  ISR used by Counter and Logic Analyzer.
  */
#if defined(USE_COUNTER) || defined(USE_LOG_ANLYS)
/**
* @brief This function handles TIM4 global interrupt.
*/
void TIM4_IRQHandler(void)
{
//  HAL_TIM_IRQHandler(&htim4);
	
	if(logAnlys.enable == LOGA_ENABLED){
		LOG_ANLYS_PeriodElapsedCallback(&htim4);		
	}else{
		COUNTER_PeriodElapsedCallback(&htim4);
	}
}
#endif // USE_COUNTER || USE_LOG_ANLYS

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
