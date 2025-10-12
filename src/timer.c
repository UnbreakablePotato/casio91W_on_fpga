/*
 * interrupt_counter_tut_2B.c
 *
 *  Version 1.2 Author : Edward Todirica
 *
 *  Created on: 	Unknown~~
 *      Author: 	Ross Elliot
 *     Version:		1.1
 */

/********************************************************************************************

* VERSION HISTORY
********************************************************************************************
*   v1.2 - 10.11.2016
*		Fixed some bugs regarding Timer Interrupts and adding some
*       debug messages for the Timer Interrupt Handler
*
* 	v1.1 - 01/05/2015
* 		Updated for Zybo ~ DN
*
*	v1.0 - Unknown
*		First version created.
*******************************************************************************************/

#include <stdio.h>
#include "xparameters.h"
#include "xgpio.h"
#include "xtmrctr.h"
#include "xscugic.h"
#include "xil_exception.h"
#include "xil_printf.h"
#include "xtime_l.h"
//
    char zero[5][5] = {
        {'0','0','0','0','0'},
        {'0',' ',' ',' ','0'},
        {'0',' ',' ',' ','0'},
        {'0',' ',' ',' ','0'},
        {'0','0','0','0','0'}
    };
    char one[5][5]= {
        {' ',' ','0',' ',' '},
        {' ',' ','0',' ',' '},
        {' ',' ','0',' ',' '},
        {' ',' ','0',' ',' '},
        {' ',' ','0',' ',' '}
    };
    char two[5][5]= {
        {' ','0','0','0',' '},
        {'0',' ',' ',' ','0'},
        {' ',' ',' ','0',' '},
        {' ','0',' ',' ',' '},
        {'0','0','0','0','0'}
    };
    char three[5][5]= {
        {'0','0','0',' ',' '},
        {' ',' ','0',' ',' '},
        {'0','0','0',' ',' '},
        {' ',' ','0',' ',' '},
        {'0','0','0',' ',' '}
    };
    char four[5][5]= {
        {' ',' ','0',' ',' '},
        {' ','0','0',' ',' '},
        {'0',' ','0',' ',' '},
        {'0','0','0',' ',' '},
        {' ',' ','0',' ',' '}
    };
    char five[5][5]= {
        {'0','0','0','0','0'},
        {'0',' ',' ',' ',' '},
        {'0','0','0','0','0'},
        {' ',' ',' ',' ','0'},
        {'0','0','0','0',' '}
    };
    char six[5][5]= {
        {'0','0','0',' ',' '},
        {'0',' ',' ',' ',' '},
        {'0','0','0',' ',' '},
        {'0',' ','0',' ',' '},
        {'0','0','0',' ',' '}
    };
    char seven[5][5]= {
        {'0','0','0','0','0'},
        {' ',' ',' ',' ','0'},
        {' ',' ',' ','0',' '},
        {' ',' ','0',' ',' '},
        {' ','0',' ',' ',' '}
    };
    char eight[5][5]= {
        {' ','0','0','0',' '},
        {'0',' ',' ',' ','0'},
        {' ','0','0','0',' '},
        {'0',' ',' ',' ','0'},
        {' ','0','0','0',' '}
    };
    char nine[5][5]= {
        {' ','0','0','0','0'},
        {' ','0',' ',' ','0'},
        {' ','0','0','0','0'},
        {' ',' ',' ',' ','0'},
        {' ',' ',' ',' ','0'}
    };
//
char (*digits[10])[5] = { zero, one, two, three, four, five, six, seven, eight, nine };
//---------------------------------------

//---------------------------------------
// Parameter definitions
#define INTC_DEVICE_ID 		XPAR_PS7_SCUGIC_0_DEVICE_ID
#define TMR_DEVICE_ID		XPAR_TMRCTR_0_DEVICE_ID
#define BTNS_DEVICE_ID		XPAR_AXI_GPIO_0_DEVICE_ID
#define LEDS_DEVICE_ID		XPAR_AXI_GPIO_1_DEVICE_ID
#define INTC_GPIO_INTERRUPT_ID XPAR_FABRIC_AXI_GPIO_0_IP2INTC_IRPT_INTR
#define INTC_TMR_INTERRUPT_ID XPAR_FABRIC_AXI_TIMER_0_INTERRUPT_INTR

#define BTN_INT 			XGPIO_IR_CH1_MASK
//#define TMR_LOAD			0xF8000000
#define TMR_LOAD			1000000

XGpio LEDInst, BTNInst;
XScuGic INTCInst;
//XTmrCtr TMRInst;
int led_data;
int btn_value;
//static int tmr_count;

//global viables for clock
int hunderdel =0;
int seconds =0;
int minutes =0;
int hours	=0;
//
XTime tStart, tEnd;


//--------

//----------------------------------------------------
// PROTOTYPE FUNCTIONS
//----------------------------------------------------
void BTN_Intr_Handler(void *baseaddr_p);
void TMR_Intr_Handler(void *InstancePtr, u8 TmrCtrNumber);
int InterruptSystemSetup(XScuGic *XScuGicInstancePtr);
int IntcInitFunction(u16 DeviceId, XTmrCtr *TmrInstancePtr, XGpio *GpioInstancePtr);
void printDigits(int hour, int minute, int second);
/*****************************************************************************/
/**
* This function should be part of the device driver for the timer device
* Clears the interrupt flag of the specified timer counter of the device.
* This is necessary to do in the interrupt routine after the interrupt was handled.
*
* @param	InstancePtr is a pointer to the XTmrCtr instance.
* @param	TmrCtrNumber is the timer counter of the device to operate on.
*		Each device may contain multiple timer counters. The timer
*		number is a zero based number  with a range of
*		0 - (XTC_DEVICE_TIMER_COUNT - 1).
*
* @return	None.
*
* @note		None.
*
******************************************************************************/
//
void printDigits(int hour, int minute, int second) {
    int h1 = hour / 10;
    int h2 = hour % 10;
    int m1 = minute / 10;
    int m2 = minute % 10;
    int s1 = second / 10;
    int s2 = second % 10;

    xil_printf("\033[H\033[J"); // fjerne alt på skærmen, virker ikke i sdk terminal, kun på puTTy terminal
    for (int i = 0; i < 5; i++) {
        // Timer
        for (int j = 0; j < 5; j++) xil_printf("%c", digits[h1][i][j]);
        xil_printf(" ");
        for (int j = 0; j < 5; j++) xil_printf("%c", digits[h2][i][j]);
        xil_printf("   |   ");

        // Minutter
        for (int j = 0; j < 5; j++) xil_printf("%c", digits[m1][i][j]);
        xil_printf(" ");
        for (int j = 0; j < 5; j++) xil_printf("%c", digits[m2][i][j]);
        xil_printf("   |   ");

        // Sekunder
        for (int j = 0; j < 5; j++) xil_printf("%c", digits[s1][i][j]);
        xil_printf(" ");
        for (int j = 0; j < 5; j++) xil_printf("%c", digits[s2][i][j]);

        xil_printf("\r\n"); // det går helt galt hvis /r ikke er der
    }

}
//
void XTmrCtr_ClearInterruptFlag(XTmrCtr * InstancePtr, u8 TmrCtrNumber)
{
	u32 CounterControlReg;

	Xil_AssertVoid(InstancePtr != NULL);
	Xil_AssertVoid(TmrCtrNumber < XTC_DEVICE_TIMER_COUNT);
	Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

	/*
	 * Read current contents of the CSR register so it won't be destroyed
	 */
	CounterControlReg = XTmrCtr_ReadReg(InstancePtr->BaseAddress,
					       TmrCtrNumber, XTC_TCSR_OFFSET);
	/*
	 * Reset the interrupt flag
	 */
	XTmrCtr_WriteReg(InstancePtr->BaseAddress, TmrCtrNumber,
			  XTC_TCSR_OFFSET,
			  CounterControlReg | XTC_CSR_INT_OCCURED_MASK);
}



//----------------------------------------------------
// INTERRUPT HANDLER FUNCTIONS
// - called by the timer, button interrupt, performs
// - LED flashing
//----------------------------------------------------

//----------------------------------------------------
void BTN_Intr_Handler(void *InstancePtr)
{
	// Disable GPIO interrupts
	XGpio_InterruptDisable(&BTNInst, BTN_INT);
	// Ignore additional button presses
	if ((XGpio_InterruptGetStatus(&BTNInst) & BTN_INT) !=
			BTN_INT) {
			return;
		}
	btn_value = XGpio_DiscreteRead(&BTNInst, 1);
	// Increment counter based on button value
	// Reset if centre button pressed
	led_data = led_data + btn_value;

    XGpio_DiscreteWrite(&LEDInst, 1, led_data);
    (void)XGpio_InterruptClear(&BTNInst, BTN_INT);
    // Enable GPIO interrupts
    XGpio_InterruptEnable(&BTNInst, BTN_INT);
}
void TMR_Intr_Handler(void *InstancePtr, u8 TmrCtrNumber)
{
    XTmrCtr_ClearInterruptFlag((XTmrCtr *)InstancePtr, TmrCtrNumber);

    if (TmrCtrNumber == 0) {
        ++hunderdel;
        if (hunderdel >= 100) {
            hunderdel = 0;
            seconds++;

            if (seconds >= 60) {
                seconds = 0;
                minutes++;

                if (minutes >= 60) {
                    minutes = 0;
                    hours = (hours + 1) % 24;
                }
            }

            printDigits(hours, minutes, seconds);
        }

    }
}

//----------------------------------------------------
// MAIN FUNCTION
//----------------------------------------------------
int main (void) {
  int status;
  XTmrCtr TMRInst;
  //----------------------------------------------------
  // INITIALIZE THE PERIPHERALS & SET DIRECTIONS OF GPIO
  //----------------------------------------------------
  // Initialise LEDs
  status = XGpio_Initialize(&LEDInst, LEDS_DEVICE_ID);
  if(status != XST_SUCCESS) return XST_FAILURE;
  // Initialise Push Buttons
  status = XGpio_Initialize(&BTNInst, BTNS_DEVICE_ID);
  if(status != XST_SUCCESS) return XST_FAILURE;
  // Set LEDs direction to outputs
  XGpio_SetDataDirection(&LEDInst, 1, 0x00);
  // Set all buttons direction to inputs
  XGpio_SetDataDirection(&BTNInst, 1, 0xFF);


  //----------------------------------------------------
  // SETUP THE TIMER
  //----------------------------------------------------
  status = XTmrCtr_Initialize(&TMRInst, TMR_DEVICE_ID);
  if(status != XST_SUCCESS) return XST_FAILURE;
  XTmrCtr_SetHandler(&TMRInst, TMR_Intr_Handler, &TMRInst);
  XTmrCtr_SetResetValue(&TMRInst, 0, TMR_LOAD);
  XTmrCtr_SetOptions(&TMRInst, 0, XTC_INT_MODE_OPTION | XTC_AUTO_RELOAD_OPTION | XTC_DOWN_COUNT_OPTION);

  // Initialize interrupt controller
  status = IntcInitFunction(INTC_DEVICE_ID, &TMRInst, &BTNInst);
  if(status != XST_SUCCESS) return XST_FAILURE;

  XTmrCtr_Start(&TMRInst, 0);
  //Here we get the time when the timer first started
  XTime_GetTime(&tStart);

  while(1);

  return 0;
}

//----------------------------------------------------
// INITIAL SETUP FUNCTIONS
//----------------------------------------------------

int InterruptSystemSetup(XScuGic *XScuGicInstancePtr)
{
	// Enable interrupt
	XGpio_InterruptEnable(&BTNInst, BTN_INT);
	XGpio_InterruptGlobalEnable(&BTNInst);

	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
			 	 	 	 	 	 (Xil_ExceptionHandler)XScuGic_InterruptHandler,
			 	 	 	 	 	 XScuGicInstancePtr);
	Xil_ExceptionEnable();


	return XST_SUCCESS;

}



int IntcInitFunction(u16 DeviceId, XTmrCtr *TmrInstancePtr, XGpio *GpioInstancePtr)
{
	XScuGic_Config *IntcConfig;
	int status;
	u8 pri, trig;

	// Interrupt controller initialisation
	IntcConfig = XScuGic_LookupConfig(DeviceId);
	status = XScuGic_CfgInitialize(&INTCInst, IntcConfig, IntcConfig->CpuBaseAddress);
	if(status != XST_SUCCESS) return XST_FAILURE;

	// Call to interrupt setup
	status = InterruptSystemSetup(&INTCInst);
	if(status != XST_SUCCESS) return XST_FAILURE;

	// Connect GPIO interrupt to handler
	status = XScuGic_Connect(&INTCInst,
					  	  	 INTC_GPIO_INTERRUPT_ID,
					  	  	 (Xil_ExceptionHandler)BTN_Intr_Handler,
					  	  	 (void *)GpioInstancePtr);
	if(status != XST_SUCCESS) return XST_FAILURE;


	// Connect timer interrupt to handler
	status = XScuGic_Connect(&INTCInst,
							 INTC_TMR_INTERRUPT_ID,
							// (Xil_ExceptionHandler)TMR_Intr_Handler,
							 (Xil_ExceptionHandler) XTmrCtr_InterruptHandler,
							 (void *)TmrInstancePtr);
	if(status != XST_SUCCESS) return XST_FAILURE;

	// Enable GPIO interrupts interrupt
	XGpio_InterruptEnable(GpioInstancePtr, 1);
	XGpio_InterruptGlobalEnable(GpioInstancePtr);

	// Enable GPIO and timer interrupts in the controller
	XScuGic_Enable(&INTCInst, INTC_GPIO_INTERRUPT_ID);
	XScuGic_Enable(&INTCInst, INTC_TMR_INTERRUPT_ID);

	xil_printf("Getting the Timer interrupt info\n\r");
	XScuGic_GetPriTrigTypeByDistAddr(INTCInst.Config->DistBaseAddress, INTC_TMR_INTERRUPT_ID, &pri, &trig);
	xil_printf("GPIO Interrupt-> Priority:%d, Trigger:%x\n\r", pri, trig);


	//Set the timer interrupt as edge triggered
	//XScuGic_SetPriorityTriggerType(&INTCInst, INTC_TMR_INTERRUPT_ID, )

	return XST_SUCCESS;
}
