/*
 * FreeRTOS Kernel V10.2.0
 * Copyright (C) 2019 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */

/* 
	NOTE : Tasks run in system mode and the scheduler runs in Supervisor mode.
	The processor MUST be in supervisor mode when vTaskStartScheduler is 
	called.  The demo applications included in the FreeRTOS.org download switch
	to supervisor mode prior to main being called.  If you are not using one of
	these demo application projects then ensure Supervisor mode is used.
*/


/*
 * Creates all the demo application tasks, then starts the scheduler.  The WEB
 * documentation provides more details of the demo application tasks.
 * 
 * Main.c also creates a task called "Check".  This only executes every three 
 * seconds but has the highest priority so is guaranteed to get processor time.  
 * Its main function is to check that all the other tasks are still operational.
 * Each task (other than the "flash" tasks) maintains a unique count that is 
 * incremented each time the task successfully completes its function.  Should 
 * any error occur within such a task the count is permanently halted.  The 
 * check task inspects the count of each task to ensure it has changed since
 * the last time the check task executed.  If all the count variables have 
 * changed all the tasks are still executing error free, and the check task
 * toggles the onboard LED.  Should any task contain an error at any time 
 * the LED toggle rate will change from 3 seconds to 500ms.
 *
 */

/* Standard includes. */
#include <stdlib.h>
#include <stdio.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "lpc21xx.h"

/* Peripheral includes. */
#include "serial.h"
#include "GPIO.h"


/*-----------------------------------------------------------*/

/* Constants to setup I/O and processor. */
#define mainBUS_CLK_FULL	( ( unsigned char ) 0x01 )

/* Constants for the ComTest demo application tasks. */
#define mainCOM_TEST_BAUD_RATE	( ( unsigned long ) 115200 )

/*----------------            TASK PERIODS            -------------------*/


#define B1_PERIOD    50
#define B2_PERIOD    50
#define TX_PERIOD    100
#define RX_PERIOD    20
#define L1_PERIOD    10
#define L2_PERIOD    100

/*-----------------------------------------------------------*/

/*----------------             TASK HANDLERS         -------------------*/
TaskHandle_t B1_Handler = NULL;
TaskHandle_t B2_Handler = NULL;
TaskHandle_t Tx_Handler = NULL;
TaskHandle_t Rx_Handler = NULL;
TaskHandle_t L1_Handler = NULL;
TaskHandle_t L2_Handler = NULL;
QueueHandle_t GLB_Queue = NULL;
/*-----------------------------------------------------------*/

/*----------------          TRACING VARIABLES        -------------------*/
int B1_in_time = 0, B1_out_time = 0, B1_total_time = 0;
int B2_in_time = 0, B2_out_time = 0, B2_total_time = 0;
int Tx_in_time = 0, Tx_out_time = 0, Tx_total_time = 0;
int Rx_in_time = 0, Rx_out_time = 0, Rx_total_time = 0;
int L1_in_time = 0, L1_out_time = 0, L1_total_time = 0;
int L2_in_time = 0, L2_out_time = 0, L2_total_time = 0;
int System_Time = 0;
int CPU_Load = 0;
/*-----------------------------------------------------------*/

/*----------------           TASKS PROTOTYPES        -------------------*/
void Button_1_Monitor    (void * pvParameters);
void Button_2_Monitor    (void * pvParameters);
void Periodic_Transmitter(void * pvParameters);
void Uart_Receiver       (void * pvParameters);
void Load_1_Simulation   (void * pvParameters);
void Load_2_Simulation   (void * pvParameters);
/*-----------------------------------------------------------*/

/*
 * Configure the processor for use with the Keil demo board.  This is very
 * minimal as most of the setup is managed by the settings in the project
 * file.
 */
static void prvSetupHardware( void );
/*
 * Application entry point:
 * Starts all the other tasks, then starts the scheduler. 
 */
int main( void )
{
	/* Setup the hardware for use with the Keil demo board. */
	prvSetupHardware();
	
	/*queue is a ptr to (struct queue definition) - create 3*sizeof pointer to characters
	 *QueueHandle_t  Queue    = NULL;
	 *
	 */
	GLB_Queue = xQueueCreate(3, sizeof(char*));
	
    /* Create Tasks here */
	xTaskPeriodicCreate(Button_1_Monitor,
					   (const char *) "B1",
					   configMINIMAL_STACK_SIZE,
					   NULL,
					   1,
					   &B1_Handler,
					   B1_PERIOD );
											
	xTaskPeriodicCreate(Button_2_Monitor,
					   (const char *) "B2",
					   configMINIMAL_STACK_SIZE,
					   NULL,
					   1,
					   &B2_Handler,
					   B2_PERIOD );
											
	xTaskPeriodicCreate(Periodic_Transmitter,
						(const char *) "Tx",
						configMINIMAL_STACK_SIZE,
						NULL,
						1,
						&Tx_Handler,
						TX_PERIOD );
											
	xTaskPeriodicCreate(Uart_Receiver,
					   (const char *) "Rx",
					   configMINIMAL_STACK_SIZE,
					   NULL,
					   1,
					   &Rx_Handler,
					   RX_PERIOD );
	xTaskPeriodicCreate(Load_1_Simulation,
					   (const char *) "L1",
					   configMINIMAL_STACK_SIZE,
					   NULL,
					   1,
					   &L1_Handler,
					   L1_PERIOD );
	xTaskPeriodicCreate(Load_2_Simulation,
					   (const char *) "L2",
					   configMINIMAL_STACK_SIZE,
					   NULL,
					   1,
					   &L2_Handler,
					   L2_PERIOD );


	/* Now all the tasks have been started - start the scheduler.

	NOTE : Tasks run in system mode and the scheduler runs in Supervisor mode.
	The processor MUST be in supervisor mode when vTaskStartScheduler is 
	called.  The demo applications included in the FreeRTOS.org download switch
	to supervisor mode prior to main being called.  If you are not using one of
	these demo application projects then ensure Supervisor mode is used here. */
	vTaskStartScheduler();

	/* Should never reach here!  If you do then there was not enough heap
	available for the idle task to be created. */
	for( ;; );
}
/*-----------------------------------------------------------*/

/* Function to reset timer 1 */
void timer1Reset(void)
{
	T1TCR |= 0x2;
	T1TCR &= ~0x2;
}

/* Function to initialize and start timer 1 */
static void configTimer1(void)
{
	T1PR = 1000;
	T1TCR |= 0x1;
}

static void prvSetupHardware( void )
{
	/* Perform the hardware setup required.  This is minimal as most of the
	setup is managed by the settings in the project file. */

	/* Configure UART */
	xSerialPortInitMinimal(mainCOM_TEST_BAUD_RATE);

	/* Configure GPIO */
	GPIO_init();
	
	/* Config trace timer 1 and read T1TC to get current tick */
	configTimer1();

	/* Setup the peripheral bus to be the same as the PLL output. */
	VPBDIV = mainBUS_CLK_FULL;
}
void vApplicationTickHook(void)
{
	GPIO_write(PORT_0,PIN0,PIN_IS_HIGH);
	GPIO_write(PORT_0,PIN0,PIN_IS_LOW);
}
void vApplicationIdleHook(void)
{
	GPIO_write(PORT_0,PIN7,PIN_IS_HIGH);
}
/*-----------------------------------------------------------*/

/********                Tasks                   **********/
void Button_1_Monitor(void * pvParameters)
{
	unsigned int WakeTime = xTaskGetTickCount();
	char B1_Currentstate = 0;
	char B1_Previousstate = 0;
	//ptr to char to be inserted in queue
	char* ptrToMSG =(void*)0; 
	
	vTaskSetApplicationTaskTag(NULL, (void *)1);
	
	for(;;)
	{
		//write your code here
		//read Button1 and store it in( B1 Currentstate)
		B1_Currentstate = GPIO_read(PORT_1, PIN1);
		if(B1_Currentstate == 1 &&  B1_Previousstate == 0)
		{
			//Send a ptr holding the message "B1 Rising Edge"
			if(GLB_Queue != 0)
			{
				ptrToMSG="B1 Rising Edge.";
				xQueueSend(GLB_Queue, (void *) &ptrToMSG, (TickType_t)0);
			}
		}
		else if(B1_Currentstate == 0 &&  B1_Previousstate == 1)
		{
			//Send a ptr holding the message "B1 Falling Edge"
			if(GLB_Queue != 0)
			{
				ptrToMSG="B1 Falling Edge";
				xQueueSend(GLB_Queue, (void *) &ptrToMSG, (TickType_t)0);
			}
		}
		//save the current state in the previous state
		B1_Previousstate = B1_Currentstate;
		
		vTaskDelayUntil(&WakeTime,B1_PERIOD);
		
		
	}
}
void Button_2_Monitor(void * pvParameters)
{
	unsigned int WakeTime = xTaskGetTickCount();
	char B2_Currentstate  = 0;
	char B2_Previousstate = 0;
	//ptr to char to be inserted in queue
	char* ptrToMSG =(void*)0; 
	
	vTaskSetApplicationTaskTag(NULL, (void *)2);
	
	for(;;)
	{
		//write your code here
		//read Button2 and store it in( B2 Currentstate)
		B2_Currentstate = GPIO_read(PORT_1, PIN2);
		if(B2_Currentstate == 1 &&  B2_Previousstate == 0)
		{
			//Send a ptr holding the message "B2 Rising Edge"
			if(GLB_Queue != 0)
			{
				ptrToMSG="B2 Rising Edge.";
				xQueueSend(GLB_Queue, (void *) &ptrToMSG, (TickType_t)0);
			}
		}
		else if(B2_Currentstate == 0 &&  B2_Previousstate == 1)
		{
			//Send a ptr holding the message "B2 Falling Edge"
			if(GLB_Queue != 0)
			{
				ptrToMSG="B2 Falling Edge";
				xQueueSend(GLB_Queue, (void *) &ptrToMSG, (TickType_t)0);
			}
		}
		//save the current state in the previous state
		B2_Previousstate = B2_Currentstate;
		
		vTaskDelayUntil(&WakeTime,B2_PERIOD);
		
		
	}
}
void Periodic_Transmitter(void * pvParameters)
{
	unsigned int WakeTime = xTaskGetTickCount(); 
	char* ptrToMSG =(void*)0;
	
	vTaskSetApplicationTaskTag(NULL, (void *)3);
	
	for(;;)
	{
		if(GLB_Queue != 0)
			{
				ptrToMSG="Tx ...String...";
				xQueueSend(GLB_Queue, (void *) &ptrToMSG, (TickType_t)0);
			}
			
		vTaskDelayUntil(&WakeTime,TX_PERIOD);
	}
}
void Uart_Receiver(void * pvParameters)
{
	unsigned int WakeTime = xTaskGetTickCount(); 
	char* ptrtoMSG = (void*)0;
	
	vTaskSetApplicationTaskTag(NULL, (void *)4);
	
	for(;;)
	{
		//check the queue if not empty, send the data through uart
		if(GLB_Queue != 0)
   {
		 if( xQueueReceive(GLB_Queue, &(ptrtoMSG), (TickType_t)0) == pdPASS )
      {
        /* ptrtoMSG now points to the MSG. 16 is number of characters pointed to by ptrtoMSG*/
				vSerialPutString((const signed char*) ptrtoMSG, 16);
      }
   }
		
		
		vTaskDelayUntil(&WakeTime,RX_PERIOD);
		
	}
}

void Load_1_Simulation(void * pvParameters)
{
	unsigned int WakeTime = xTaskGetTickCount();
	unsigned int i = 0;
	
	vTaskSetApplicationTaskTag(NULL, (void *)5);
	
	for(;;)
	{
		for(i=0; i<37000; i++)
			{
				/*Simulate heavy load*/
			}
			vTaskDelayUntil(&WakeTime, L1_PERIOD);
	}
}
void Load_2_Simulation(void * pvParameters)
{
	unsigned int WakeTime = xTaskGetTickCount();
	unsigned int i = 0;
	
	vTaskSetApplicationTaskTag(NULL, (void *)6);
	
	for(;;)
	{
		for(i=0; i<90000; i++)
			{
				/*Simulate heavy load*/
			}
			vTaskDelayUntil(&WakeTime, L2_PERIOD);
	}
}
/*-----------------------------------------------------------*/
