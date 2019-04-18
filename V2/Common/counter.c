/**
  *****************************************************************************
  * @file    counter.c
  * @author  HeyBirdie
  * @date    June 3, 2017
  * @brief   This file contains counter functions
  ***************************************************************************** 
*/ 

// Includes ===================================================================
	#ifdef USE_COUNTER
#include "cmsis_os.h"
#include "mcu_config.h"
#include "comms.h"
#include "counter.h"
#include "tim.h"
#include "stdlib.h"
#include "portmacro.h"

/** @defgroup Counter Counter
  * @{
  */

/** @defgroup Counter_Private_Variables Counter Private Variables
  * @{
  */
xQueueHandle counterMessageQueue;
xSemaphoreHandle counterMutex;
portTickType xStartTime;

volatile counterTypeDef counter;
/**
  * @}
  */

/* Obsolete variables */
static uint32_t ic1PassNum = 1;
static uint32_t ic2PassNum = 1;

/** @defgroup Counter_Function_Definitions Counter Function Definitions
  * @{
  */
/**
  * @brief  Counter task function.
  * 				Task is getting messages from other tasks and takes care about counter functions.
  * @param  Task handler, parameters pointer
  * @retval None
  */
void CounterTask(void const *argument)
{
	counterMessageQueue = xQueueCreate(5, 20);  // xQueueCreate(5, sizeof(double)); e.g.
	counterMutex = xSemaphoreCreateRecursiveMutex();	
	
	if(counterMessageQueue == 0){
		while(1); // Queue was not created and must not be used.
	}
	char message[20];
	
	while(1){
		
		xQueueReceive(counterMessageQueue, message, portMAX_DELAY);		
		xSemaphoreTakeRecursive(counterMutex, portMAX_DELAY);
		
		if(message[0]=='1'){
			counterInitETR();
		}else if(message[0]=='2'){
			counterInitIC();
		}else if(message[0]=='3'){
			counterInitREF();
		}else if(message[0]=='4'){
			counterInitTI();
		}else if(message[0]=='5'){
			counterStart();
		}else if(message[0]=='6'){
			counterStop();
		}else if(message[0]=='7'){
			counter_deinit();
		}else if(message[0]=='8'){
			counterGateConfig(counter.counterEtr.gateTime);
		}else{		
		}	
		
		xSemaphoreGiveRecursive(counterMutex);
	}
}

/* ************************************************************************************** */
/* -------------------------------- Counter basic settings ------------------------------ */
/* ************************************************************************************** */
/**
  * @brief  Counter mode initialization function.
	*					Sends the request to the queue of Counter task. 
	*					Called from parser (cmd_parser.c).
	* @param  mode:	ETR - direct method, IC - reciprocal, TI - time interval, REF - measuring with reference
  * @retval None
  */
void counterSetMode(uint8_t mode){
	switch(mode){
		case ETR:
			xQueueSendToBack(counterMessageQueue, "1SetEtrMode", portMAX_DELAY);
			break;
		case IC:
			xQueueSendToBack(counterMessageQueue, "2SetIcMode", portMAX_DELAY);
			break;
		case REF:
			xQueueSendToBack(counterMessageQueue, "3SetRefMode", portMAX_DELAY);
			break;	
		case TI:
			xQueueSendToBack(counterMessageQueue, "4SetTiMode", portMAX_DELAY);
			break;		
	}
}

/**
  * @brief  Counter start request.
	*					Sends the request to the queue of Counter task. 
	*					Called from parser (cmd_parser.c).
	* @param  None
  * @retval None
  */
void counterSendStart(void){	
	xQueueSendToBack(counterMessageQueue, "5StartCounter", portMAX_DELAY);
}

/**
  * @brief  Counter stop request.
	*					Sends the request to the queue of Counter task. 
	*					Called from parser (cmd_parser.c).
	* @param  None
  * @retval None
  */
void counterSendStop(void){	
	xQueueSendToBack(counterMessageQueue, "6StopCounter", portMAX_DELAY);
}

/**
  * @brief  Counter deinitialization request.
	*					Sends the request to the queue of Counter task. 
	*					Called from parser (cmd_parser.c).
	* @param  None
  * @retval None
  */
void counterDeinit(void){
	xQueueSendToBack(counterMessageQueue, "7DeinitCounter", portMAX_DELAY);
}

/* ************************************************************************************** */
/* ---------------------------- Counter INIT DEINIT functions --------------------------- */
/* ************************************************************************************** */
/**
  * @brief  Counter direct (External TRigger) mode initialization function.
	*					Called from Counter task.
	* @param  None
  * @retval None
  */
void counterInitETR(void){
	counter_deinit();
	counter.state = COUNTER_ETR;
	TIM_counter_etr_init();
}

/**
  * @brief  Counter reciprocal (Input Capture) mode initialization function.
	*					Called from Counter task.
	* @param  None
  * @retval None
  */
void counterInitIC(void){
	counter_deinit();
	counter.state = COUNTER_IC;
	TIM_counter_ic_init();
}

/**
  * @brief  Counter reference measurement mode initialization function. 
	*					Allows measuring frequency with precise external clock source.
	*					Called from Counter task.
	* @param  None
  * @retval None
  */
void counterInitREF(void){
	counter_deinit();
	counter.state = COUNTER_REF;
	TIM_counter_ref_init();
}

/**
  * @brief  Counter Time Interval mode initialization function. 
	*					Allows measuring time between two events (rising/falling edge) on single or two channels.
	*					Called from Counter task.
	* @param  None
  * @retval None
  */
void counterInitTI(void){
	counter_deinit();
	counter.state = COUNTER_TI;
	TIM_counter_ti_init();
}

/**
  * @brief  Counter deinitialization function.
	* @param  None
  * @retval None
  */
void counter_deinit(void){
	switch(counter.state){
		case COUNTER_ETR:
			TIM_etr_deinit();
			break;
		case COUNTER_IC:
			TIM_ic_deinit();
			break;
		case COUNTER_REF:
			TIM_ref_deinit();
			break;	
		case COUNTER_TI:
			TIM_ti_deinit();
			break;		
		case COUNTER_IDLE:
			/* no hacer nada */
			break;		
	}
}

/* ************************************************************************************** */
/* ---------------------------- Counter START STOP functions ---------------------------- */
/* ************************************************************************************** */
/**
  * @brief  Counter start function.
	*					Called from Counter task.
	* @param  None
  * @retval None
  */
void counterStart(void){
	switch(counter.state){
		case COUNTER_ETR:
		 	TIM_ETR_Start();
			break;
		case COUNTER_IC:
			TIM_IC_Start();
			break;
		case COUNTER_REF:
			TIM_ETR_Start();
			break;
		case COUNTER_TI:
			TIM_TI_Start();
			break;		
		case COUNTER_IDLE:
			/* no hacer nada */
			break;
	}	
}

/**
  * @brief  Counter stop function.
	*					Called from Counter task.
	* @param  None
  * @retval None
  */
void counterStop(void){
	switch(counter.state){
		case COUNTER_ETR:
		 	TIM_ETR_Stop();
			break;
		case COUNTER_IC:
			TIM_IC_Stop();
			break;
		case COUNTER_REF:
			TIM_ETR_Stop();
			break;
		case COUNTER_TI:
			TIM_TI_Stop();		
			break;		
		case COUNTER_IDLE:
			/* no hacer nada */
			break;
	}	
}

/**
  * @brief  Setter for time gating of direct counting (ETR).
	* @param  gateTime: 100, 500, 1000, 5000, 10000 in milliseconds
  * @retval None
  */
void counterSetEtrGate(uint16_t gateTime){
	counter.counterEtr.gateTime = gateTime;			
	xQueueSendToBack(counterMessageQueue, "8SetEtrGate", portMAX_DELAY);
}

/**
  * @brief  Setter for measuring with reference (REF). The PSC and REF numbers to be multiplied (number of REF clock ticks to be counted).
						Note that REF mode uses ETR struct, some ETR functions...
	* @param  psc: 16-bit value for timer PSC register.
  * @retval None
  */
void counterSetRefPsc(uint16_t psc){	
	counter.counterEtr.psc = psc - 1;
	TIM_ARR_PSC_Config(counter.counterEtr.arr, counter.counterEtr.psc);		
}

/**
  * @brief  Setter for measuring with reference (REF).
						Note that REF mode uses ETR struct, some ETR functions...
	* @param  psc: 16-bit value for timer ARR register.
  * @retval None
  */
void counterSetRefArr(uint16_t arr){		
	counter.counterEtr.arr = arr - 1;
	TIM_ARR_PSC_Config(counter.counterEtr.arr, counter.counterEtr.psc);			
}

/**
  * @brief  Setter for counter IC buffer size (number of edges counted) on channel 1.
	* @param  buffer: range between 2 - xxx (max. value depends on free memory availability)
  * @retval None
  */
void counterSetIc1SampleCount(uint16_t buffer){
	xSemaphoreTakeRecursive(counterMutex, portMAX_DELAY);
	counter.counterIc.ic1BufferSize = buffer + 1;						 // PC app sends number of samples but IC needs the number of edges, therefore "buffer + 1";		
	DMA_Restart(&hdma_tim2_ch1);	
	xSemaphoreGiveRecursive(counterMutex);
}

/**
  * @brief  Setter for counter IC buffer size (number of edges counted) on channel 2.
	* @param  buffer: range between 2 - xxx (max. value depends on free memory availability)
  * @retval None
  */
void counterSetIc2SampleCount(uint16_t buffer){
	xSemaphoreTakeRecursive(counterMutex, portMAX_DELAY);	
	counter.counterIc.ic2BufferSize = buffer + 1;	
	DMA_Restart(&hdma_tim2_ch2_ch4);	
	xSemaphoreGiveRecursive(counterMutex);		
}

/**
  * @brief  Setter for counter IC signal prescaler on channel 1.
	* @param  presc: 1, 2, 4, 8
  * @retval None
  */
void counterSetIc1Prescaler(uint16_t presc){	
	TIM_IC1_PSC_Config(presc);
	DMA_Restart(&hdma_tim2_ch1);
}

/**
  * @brief  Setter for counter IC signal prescaler on channel 2.
	* @param  presc: 1, 2, 4, 8
  * @retval None
  */
void counterSetIc2Prescaler(uint16_t presc){		
	TIM_IC2_PSC_Config(presc);	
	DMA_Restart(&hdma_tim2_ch2_ch4);	
}

/**
  * @brief  Init duty cycle measuring during reciprocal (IC) mode on channel 1.
	* @param  None
  * @retval None
  */
void counterIc1DutyCycleInit(void){	
	counter.icDutyCycle = DUTY_CYCLE_CH1_ENABLED;
	TIM_IC_DutyCycle_Init();	
}

/**
  * @brief  Deinit duty cycle measuring during reciprocal (IC) mode on channel 1.
	* @param  None
  * @retval None
  */
void counterIc1DutyCycleDeinit(void){	
	TIM_IC_DutyCycle_Deinit();		
	counter.icDutyCycle = DUTY_CYCLE_DISABLED;
}

/**
  * @brief  Init duty cycle measuring during reciprocal (IC) mode on channel 2.
	* @param  None
  * @retval None
  */
void counterIc2DutyCycleInit(void){	
	counter.icDutyCycle = DUTY_CYCLE_CH2_ENABLED;
	TIM_IC_DutyCycle_Init();	
}

/**
  * @brief  Deinit duty cycle measuring during reciprocal (IC) mode on channel 2.
	* @param  None
  * @retval None
  */
void counterIc2DutyCycleDeinit(void){		
	TIM_IC_DutyCycle_Deinit();		
	counter.icDutyCycle = DUTY_CYCLE_DISABLED;
}

/**
  * @brief  Start duty cycle measurement on channel 1 or 2.
	* @param  None
  * @retval None
  */
void counterIcDutyCycleEnable(void){
	TIM_IC_DutyCycle_Start();
}

/**
  * @brief  Stop duty cycle measurement on channel 1 or 2.
	* @param  None
  * @retval None
  */
void counterIcDutyCycleDisable(void){
	TIM_IC_DutyCycle_Stop();
}

/**
	* @brief  Configures the trigger edge sensitivity to rising/falling. 
	*					For IC and TI modes on channel 1.					
	* @param  None
  * @retval None 
  */
void counterSetIcTi1_RisingFalling(void){	
	TIM_IC1_RisingFalling();	
	DMA_Restart(&hdma_tim2_ch1);
}	

/**
	* @brief  Configures the trigger edge sensitivity to rising.
	*					For IC and TI modes on channel 1.					
	* @param  None
  * @retval None 
  */
void counterSetIcTi1_Rising(void){
	counter.eventChan1 = EVENT_RISING;
	TIM_IC1_RisingOnly();	
}	

/**
	* @brief  Configures the trigger edge sensitivity to falling.
	*					For IC and TI modes on channel 1.					
	* @param  None
  * @retval None 
  */
void counterSetIcTi1_Falling(void){
	counter.eventChan1 = EVENT_FALLING;
	TIM_IC1_FallingOnly();	
}

/**
	* @brief  Configures the trigger edge sensitivity to rising/falling.
	*					For IC and TI modes on channel 2.					
	* @param  None
  * @retval None 
  */
void counterSetIcTi2_RisingFalling(void){
	TIM_IC2_RisingFalling();
	DMA_Restart(&hdma_tim2_ch2_ch4);	
}	

/**
	* @brief  Configures the trigger edge sensitivity to rising.
	*					For IC and TI modes on channel 2.					
	* @param  None
  * @retval None 
  */
void counterSetIcTi2_Rising(void){
	counter.eventChan2 = EVENT_RISING;
	TIM_IC2_RisingOnly();	
}	

/**
	* @brief  Configures the trigger edge sensitivity to falling.
	*					For IC and TI modes on channel 2.					
	* @param  None
  * @retval None 
  */
void counterSetIcTi2_Falling(void){
	counter.eventChan2 = EVENT_FALLING;
	TIM_IC2_FallingOnly();	
}


/**
	* @brief  Configures sequence of events AB to be measured. E.g. setting event A on channel 1 and event B on channel 2 and sequence AB then we can measure time between AB. 
	*					For TI mode only.			
	* @param  None
  * @retval None 
  */
void counterSetTiSequence_AB(void){
	TIM_TI_Sequence_AB();
}

/**
	* @brief  Configures sequence of events BA to be measured. E.g. setting event A on channel 2 and event B on channel 1 or vice versa and sequence BA, we can measure time between BA. 
	*					For TI mode only.			
	* @param  None
  * @retval None 
  */
void counterSetTiSequence_BA(void){
	TIM_TI_Sequence_BA();
}

/**
  * @brief  Function configuring event sequence dependence - refer to tim.c TIM_TI_Start() for more information.
	* @param  None
  * @retval None
  */
void counterSetTiMode_Independent(void){
	counter.tiMode = TI_MODE_EVENT_SEQUENCE_INDEP;
}

void counterSetTiMode_Dependent(void){
	counter.tiMode = TI_MODE_FAST_EVENT_SEQUENCE_DEP;
} 

/**
  * @brief  Setter for counter TI measurement timeout.
	* @param  timeout: 500 - 28000 in milliseconds
  * @retval None
  */
void counterSetTiTimeout(uint32_t timeout){
	counter.counterIc.tiTimeout = timeout;				
}

/* ************************************************************************************** */
/* ----------------------------- Counter callback functions ----------------------------- */
/* ************************************************************************************** */
/**
  * @brief  This function is executed in case of DMA transfer complete event of direct (ETR) or reference (REF) mode.
	*					The DMA transfer complete event is triggered after TIM4 gate time elapses.
  * @param  Pointer to DMA handle structure.
  * @retval None
  * @state  VERY USED
  */
void COUNTER_ETR_DMA_CpltCallback(DMA_HandleTypeDef *dmah)
{			
	portBASE_TYPE xHigherPriorityTaskWoken;
	xSemaphoreTakeFromISR(counterMutex, &xHigherPriorityTaskWoken);
	
	/***** Counter ETR handle *****/
	if(counter.state == COUNTER_ETR){			
		counter.counterEtr.etrp = TIM_ETPS_GetPrescaler();
		double gateFreq = ((double)tim4clk / (double)((counter.counterEtr.arr + 1) * (counter.counterEtr.psc + 1)));			/* TIM4 gating frequency */	
		counter.counterEtr.freq = ((double)counter.counterEtr.buffer * gateFreq * counter.counterEtr.etrp);								/* Sampled frequency */
		/* Configure the ETR input prescaler */
		TIM_ETRP_Config(counter.counterEtr.freq);	
		
		if(counter.sampleCntChange != SAMPLE_COUNT_CHANGED){
			xQueueSendToBackFromISR(messageQueue, "GEtrDataSend", &xHigherPriorityTaskWoken);
		}else{
			counter.sampleCntChange = SAMPLE_COUNT_NOT_CHANGED;
		}		
		
	/***** Counter REF handle *****/
	}else if(counter.state == COUNTER_REF){		
		if((counter.sampleCntChange != SAMPLE_COUNT_CHANGED) && (xTaskGetTickCount() - xStartTime) < 100){
			xQueueSendToBackFromISR(messageQueue, "ORefWarning", &xHigherPriorityTaskWoken);
			TIM_REF_SecondInputDisable();					
		}else if(counter.sampleCntChange != SAMPLE_COUNT_CHANGED){	
			xQueueSendToBackFromISR(messageQueue, "GRefDataSend", &xHigherPriorityTaskWoken);			
		}else{
			counter.sampleCntChange = SAMPLE_COUNT_NOT_CHANGED;
		}				
	}
	
	xSemaphoreGiveFromISR(counterMutex, &xHigherPriorityTaskWoken);
}

/**
  * @brief  This function is executed in case of TIM4 period elapse event. This function accompanies 
						IC (freq. meas. + Duty Cycle) and TI (time interval between 2 events meas.) modes.
  * @param  Pointer to TIM handle structure.
  * @retval None
  * @state  VERY USED
  */
void counterPeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if(counter.icDutyCycle == DUTY_CYCLE_DISABLED){
		if(counter.state == COUNTER_IC){
			/* Input Capture Low Frequency coutner */
			counterIcProcess();
		}else{
			/* Time Interval mode of measurement */
			counterTiProcess();
		}						
	}else{		
		/* Duty cycle measurement in Low Frequency counter */
		counterIcDutyCycleProcess();
	}
}

/**
  * @brief  Function colaborating with counterPeriodElapsedCallback to handle every 100 ms captured events if the required samples were transfered.
						Frequency of IC1 or IC2 channel is computed and sent to PC app. This approach replaces DMA data transfercomplete interrupts	of both 
						channels - the higher frequencies measured the CPU more heavy loaded, therefore replaced.
						BIN "semaphore" implemented due to communication issue when data send to PC. (too slow)
  * @param  None
  * @retval None
  * @state  VERY USED
  */
void counterIcProcess(void)
{	
	portBASE_TYPE xHigherPriorityTaskWoken;
	xSemaphoreTakeFromISR(counterMutex, &xHigherPriorityTaskWoken);	
	
	if(counter.bin != BIN0){
		/* BINx is used to alternate data sending from IC1 and IC2. Thanks to DMA_TransferComplete function
			 if there's still no data available from one source (ICx) the second one is not stalled. Meaning,
			 IC channels don't have to necessarilly rotate/alternate if the difference of frequencies is big. */
		counter.bin = BIN0;
		
		if(DMA_TransferComplete(&hdma_tim2_ch1)){				

			counter.counterIc.ic1psc = TIM_IC1PSC_GetPrescaler();			
			uint32_t capture1 = counter.counterIc.ic1buffer[counter.counterIc.ic1BufferSize-1] - counter.counterIc.ic1buffer[0];
			counter.counterIc.ic1freq = (double)(tim2clk*(counter.counterIc.psc+1)*counter.counterIc.ic1psc)*((double)(counter.counterIc.ic1BufferSize-1)/(double)capture1);				
			
			DMA_Restart(&hdma_tim2_ch1);
			counter.icChannel1 = COUNTER_IRQ_IC;
			xQueueSendToBackFromISR(messageQueue, "GIcDataSend", &xHigherPriorityTaskWoken);									
		}
		
	}else if(counter.bin != BIN1){
		
		counter.bin = BIN1;
		
		if(DMA_TransferComplete(&hdma_tim2_ch2_ch4)){
						
			counter.counterIc.ic2psc = TIM_IC2PSC_GetPrescaler();				
			uint32_t capture2 = counter.counterIc.ic2buffer[counter.counterIc.ic2BufferSize-1] - counter.counterIc.ic2buffer[0];
			counter.counterIc.ic2freq = (double)(tim2clk*(counter.counterIc.psc+1)*counter.counterIc.ic2psc)*((double)(counter.counterIc.ic2BufferSize-1)/(double)capture2);					
						
			DMA_Restart(&hdma_tim2_ch2_ch4);		
			counter.icChannel2 = COUNTER_IRQ_IC;		
			xQueueSendToBackFromISR(messageQueue, "GIcDataSend", &xHigherPriorityTaskWoken);	
		}
	}
	
	xSemaphoreGiveFromISR(counterMutex, &xHigherPriorityTaskWoken);			
}

/**
  * @brief  Function colaborating with counterPeriodElapsedCallback to handle 
						2 independent input events on 2 separate channels - Time Interval meas. 
						mode. CounterIc.ic1freq is used to send time delay between two events.
  * @param  None
  * @retval None
  * @state  VERY USED
  */
void counterTiProcess(void)
{
	portBASE_TYPE xHigherPriorityTaskWoken;
	xSemaphoreTakeFromISR(counterMutex, &xHigherPriorityTaskWoken);	
	
	/* Check timeout. */
	if((xTaskGetTickCountFromISR() - xStartTime) <= counter.counterIc.tiTimeout){
		/* Check the event sequence - AB or BA */
		if(counter.abba == BIN0){			
			/* Check DMA transfer channel 1 occured */			
			if(DMA_TransferComplete(&hdma_tim2_ch2_ch4)){					
				counter.counterIc.ic1freq = counter.counterIc.ic2buffer[0] / (double)tim2clk;																			
				TIM_TI_Stop();		
				counter.tiState = SEND_TI_DATA;						
				xQueueSendToBackFromISR(messageQueue, "GTiBuffersTaken", &xHigherPriorityTaskWoken);					
			}
		/* If timeout occured stop TI counter and send alert to PC application. */		
		}else{				
			/* Check DMA transfer channel 2 occured */			
			if(DMA_TransferComplete(&hdma_tim2_ch1)){					
				counter.counterIc.ic1freq = counter.counterIc.ic1buffer[0] / (double)tim2clk;												
				TIM_TI_Stop();					
				counter.tiState = SEND_TI_DATA;						
				xQueueSendToBackFromISR(messageQueue, "GTiBuffersTaken", &xHigherPriorityTaskWoken);							
			}
		}		
	}else{
		TIM_TI_Stop();					
		counter.tiState = TIMEOUT;	
		xQueueSendToBackFromISR(messageQueue, "GTiTimoutOccurred", &xHigherPriorityTaskWoken);			
	}
		
	xSemaphoreGiveFromISR(counterMutex, &xHigherPriorityTaskWoken);		
}

/**
  * @brief  Function colaborating with counterPeriodElapsedCallback to handle 
						one input (TI1 or TI2) that is feeding two IC registers to calculate pulse width
						and duty cycle. BIN implemented due to UART speed issue when sending data to PC.
  * @param  None
  * @retval None
  * @state  VERY USED
  */
void counterIcDutyCycleProcess(void)
{
	portBASE_TYPE xHigherPriorityTaskWoken;
	xSemaphoreTakeFromISR(counterMutex, &xHigherPriorityTaskWoken);	

	if(counter.icDutyCycle == DUTY_CYCLE_CH1_ENABLED){	
		if(DMA_TransferComplete(&hdma_tim2_ch1)){
			/* Calculate duty cycle (ic1freq) and pulse width(ic2freq). Frequency struct variables temporarily used. */
			counter.counterIc.ic1freq = (counter.counterIc.ic2buffer[0] / (double)counter.counterIc.ic1buffer[0]) * 100;
			counter.counterIc.ic2freq = counter.counterIc.ic2buffer[0] / (double)tim2clk;
				
			TIM_IC_DutyCycleDmaRestart();		
			
			/* DMA transfers some unspecified number immediately after 
				 Duty Cycle start - getting rid of it. */
			if(counter.bin == BIN0){
				counter.bin = BIN1;
			}else{
				xQueueSendToBackFromISR(messageQueue, "GIc1DutyCycle", &xHigherPriorityTaskWoken);		
			}								
		}
	}else if(counter.icDutyCycle == DUTY_CYCLE_CH2_ENABLED){
		if(DMA_TransferComplete(&hdma_tim2_ch2_ch4)){			
			counter.counterIc.ic1freq = (counter.counterIc.ic1buffer[0] / (double)counter.counterIc.ic2buffer[0]) * 100;
			counter.counterIc.ic2freq = counter.counterIc.ic1buffer[0] / (double)tim2clk;
			
			TIM_IC_DutyCycleDmaRestart();			
			
			if(counter.bin == BIN0){
				counter.bin = BIN1;
			}else{
				xQueueSendToBackFromISR(messageQueue, "GIc2DutyCycle", &xHigherPriorityTaskWoken);		
			}					
		}
	}	
			
	xSemaphoreGiveFromISR(counterMutex, &xHigherPriorityTaskWoken);			
}

/* ************************************************************************************** */
/* ----------------------------- Counter specific functions ----------------------------- */
/* ************************************************************************************** */
/**
	* @brief  This function configures ARR and PSC registers of 16b-it timer if running on 72 MHz. 	
	* @param  gateTime: gate time in milliseconds
  * @retval none 
  */
void counterGateConfig(uint16_t gateTime)
{				
	switch(gateTime){
		case 100:														/* min.	gate time 00.10 second */
			counter.counterEtr.psc = 7199;
			counter.counterEtr.arr = 999;			
			break;
		case 500: 													/* ----	gate time 00.50 second */
			counter.counterEtr.psc = 5999;
			counter.counterEtr.arr = 5999;		
			break;		
		case 1000: 													/* ----	gate time 01.00 second */
			counter.counterEtr.psc = 7199;
			counter.counterEtr.arr = 9999;		
			break;				
		case 5000: 													/* ----	gate time 05.00 second */
			counter.counterEtr.psc = 59999;
			counter.counterEtr.arr = 5999;	
			break;		
		case 10000: 												/* max. gate time 10.00 second */
			counter.counterEtr.psc = 35999;
			counter.counterEtr.arr = 19999;			
			break;
		default:
			break;			
	}
	
	TIM_ARR_PSC_Config(counter.counterEtr.arr, counter.counterEtr.psc);
}

/**
  * @brief  Counter set Default values
  * @param  None
  * @retval None
	* @state 	USED
  */
void counterEtrRefSetDefault(void)
{
	if(counter.state==COUNTER_ETR){
		counter.counterEtr.psc = TIM4_PSC;	
		counter.counterEtr.arr = TIM4_ARR;
		counter.counterEtr.gateTime = 100;				/* 100 ms */												
	}else{
		counter.counterEtr.psc = 59999;	
		counter.counterEtr.arr = 59999;				
	}
	counter.counterEtr.etrp = 1;
	counter.counterEtr.buffer = 0;
	counter.sampleCntChange = SAMPLE_COUNT_CHANGED;			
}

void counterIcTiSetDefault(void)
{
	if(counter.state == COUNTER_IC){
		counter.counterIc.ic1BufferSize = 2;			/* the lowest value of icxBufferSize is 2! - 1 sample for IC frequency measuring */
		counter.counterIc.ic2BufferSize = 2;
		counter.icChannel1 = COUNTER_IRQ_IC_PASS;
		counter.icChannel2 = COUNTER_IRQ_IC_PASS;				
	}else{
		counter.counterIc.ic1BufferSize = 1;			/* only 1 sample for one event that occurs on one single channel */
		counter.counterIc.ic2BufferSize = 1;
		counter.counterIc.tiTimeout = 10000;
		counter.eventChan1 = EVENT_RISING;
		counter.eventChan2 = EVENT_RISING;
	}
	counter.counterIc.ic1psc = 1;
	counter.counterIc.ic2psc = 1;
	TIM_IC1_PSC_Config(1);
	TIM_IC2_PSC_Config(1);	
	counter.counterIc.psc = 0;		
	counter.counterIc.arr = 0xFFFFFFFF;
}


/** @defgroup Counter OBSOLETE Functions NOT USED
  * @{
  */
/* ************************************************************************************** */
/* --------------------------- Obsolete functions - not used ---------------------------- */
/* ************************************************************************************** */
/**
  * @brief  This function is executed in case of DMA transfer complete event of Input Capture channel 1.
  * @param  Pointer to DMA handle structure.
  * @retval None
  * @state  NOT USED, OBSOLETE
  */
void COUNTER_IC1_DMA_CpltCallback(DMA_HandleTypeDef *dmah)
{
	portBASE_TYPE xHigherPriorityTaskWoken;
	xSemaphoreTakeFromISR(counterMutex, &xHigherPriorityTaskWoken);
	
	counter.icChannel1 = COUNTER_IRQ_IC;
	counter.counterIc.ic1psc = TIM_IC1PSC_GetPrescaler();
	
	uint32_t capture1 = counter.counterIc.ic1buffer[counter.counterIc.ic1BufferSize-1] - counter.counterIc.ic1buffer[0];
	counter.counterIc.ic1freq = (double)(tim2clk*(counter.counterIc.psc+1)*counter.counterIc.ic1psc)*((double)(counter.counterIc.ic1BufferSize-1)/(double)capture1);
	TIM_IC1PSC_Config(counter.counterIc.ic1freq);		
	
	counterIc1BufferConfig(counter.counterIc.ic1BufferSize);
	
	xSemaphoreGiveFromISR(counterMutex, &xHigherPriorityTaskWoken);		
	
	/* The expression " > 5" adjusts the frequency of data sending (in this case 5 Hz). 
		 With this parameter also expression " / 5" has to be changed according to first exp. 
		 Implemented in order to lower a bus load (every 200 ms). */
	if ((counter.counterIc.ic1freq / counter.counterIc.ic1BufferSize > 5) && \
											 	(ic1PassNum < (counter.counterIc.ic1freq / counter.counterIc.ic1BufferSize / 5))){
		ic1PassNum++;		
	} else {
		xQueueSendToBackFromISR(messageQueue, "GIcDataSend", &xHigherPriorityTaskWoken);		
		ic1PassNum = 1;
	}
}

/**
  * @brief  This function is executed in case of DMA transfer complete event of Input Capture channel 2.
  * @param  Pointer to DMA handle structure.
  * @retval None
  * @state  NOT USED, OBSOLETE
  */
void COUNTER_IC2_DMA_CpltCallback(DMA_HandleTypeDef *dmah)
{
	portBASE_TYPE xHigherPriorityTaskWoken;
	xSemaphoreTakeFromISR(counterMutex, &xHigherPriorityTaskWoken);
	
	counter.icChannel2 = COUNTER_IRQ_IC;	
	counter.counterIc.ic2psc = TIM_IC2PSC_GetPrescaler();
		
	uint32_t capture2 = counter.counterIc.ic2buffer[counter.counterIc.ic2BufferSize-1] - counter.counterIc.ic2buffer[0];
	counter.counterIc.ic2freq = (double)(tim2clk*(counter.counterIc.psc+1)*counter.counterIc.ic2psc)*((double)(counter.counterIc.ic2BufferSize-1)/(double)capture2);
	TIM_IC2PSC_Config(counter.counterIc.ic2freq);		
	
	counterIc2BufferConfig(counter.counterIc.ic2BufferSize);
	
	xSemaphoreGiveFromISR(counterMutex, &xHigherPriorityTaskWoken);	
	
	/* The expression " > 5" adjusts the frequency of data sending (in this case 5 Hz). 
		 With this parameter also expression " / 5" has to be changed according to first exp. 
		 Implemented in order to lower a bus load (every 200 ms). */
	if ((counter.counterIc.ic2freq / counter.counterIc.ic2BufferSize > 5) && \
											 	(ic2PassNum < (counter.counterIc.ic2freq / counter.counterIc.ic2BufferSize / 5))){
		ic2PassNum++;		
	} else {
		xQueueSendToBackFromISR(messageQueue, "LIcDataSend", &xHigherPriorityTaskWoken);		
		ic2PassNum = 1;
	}
}

/**
	* @brief  Function adjusting IC1 buffer size (number of edges counted) if user sends a value that cannot be served.
						For instance, if user sends number 13 (number of edges counted) and IC prescaler is 4, it is impossible to serve this value
						due to this prescaler. Therefore, instead (13/4)*4 = 3*4 + 4 = 16 will be set.
	* @param  ic1buffSize: buffer size (e.g. sent from user PC app)
  * @retval none 
  * @state  NOT USED, OBSOLETE
  */
void counterIc1BufferConfig(uint16_t ic1buffSize)
{
	if((ic1buffSize % counter.counterIc.ic1psc)!=0){	
		//counter.icFlag1 = COUNTER_BUFF_FLAG1;
		counter.counterIc.ic1BufferSize = ((ic1buffSize/counter.counterIc.ic1psc)*counter.counterIc.ic1psc+counter.counterIc.ic1psc);			
	}
}

/**
	* @brief  Function adjusting IC2 buffer size (number of edges counted)
	* @param  ic1buffSize: buffer size (e.g. sent from user PC app)
  * @retval none 
  * @state  NOT USED, OBSOLETE
  */
void counterIc2BufferConfig(uint16_t ic2buffSize)
{
	if((ic2buffSize % counter.counterIc.ic2psc)!=0){	
		//counter.icFlag2 = COUNTER_BUFF_FLAG2;
		counter.counterIc.ic2BufferSize = ((ic2buffSize/counter.counterIc.ic2psc)*counter.counterIc.ic2psc+counter.counterIc.ic2psc);
	}
}

/**
  * @}
  */

/**
  * @}
  */

	#endif //USE_COUNTER

/**
  * @}
  */
