/********************************************************************************************

* VERSION HISTORY
********************************************************************************************
*
*   v1.3 07/10/2025 author: Sander Lange, Roni Khalil, Jacob Søgaard
*
*   v1.2 - 10.11.2016 author: Edward Todirica
*		Fixed some bugs regarding Timer Interrupts and adding some
*       debug messages for the Timer Interrupt Handler
*
* 	v1.1 - 01/05/2015 Ross Elliot
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

// Parameter definitions
#define INTC_DEVICE_ID 		XPAR_PS7_SCUGIC_0_DEVICE_ID
#define TMR_DEVICE_ID		XPAR_TMRCTR_0_DEVICE_ID
#define BTNS_DEVICE_ID		XPAR_AXI_GPIO_0_DEVICE_ID
#define LEDS_DEVICE_ID		XPAR_AXI_GPIO_1_DEVICE_ID
#define SW_DEVICE_ID		XPAR_AXI_GPIO_2_DEVICE_ID

#define INTC_GPIO_INTERRUPT_ID XPAR_FABRIC_AXI_GPIO_0_IP2INTC_IRPT_INTR
#define INTC_TMR_INTERRUPT_ID XPAR_FABRIC_AXI_TIMER_0_INTERRUPT_INTR

#define BTN_INT 			XGPIO_IR_CH1_MASK
//#define TMR_LOAD			0xF8000000
#define TMR_LOAD			100000000

//Vores defines
#define INTC_SWGPIO_INTERRUPT_ID XPAR_FABRIC_AXI_GPIO_2_IP2INTC_IRPT_INTR
#define	SW_INT				XGPIO_IR_CH1_MASK
#define A	0x1
//#define C	0x2
#define C	0x4
#define L	0x8
#define BTN_DEBOUNCE 1000000

XGpio LEDInst, BTNInst, SWInst;
XScuGic INTCInst;
//XTmrCtr TMRInst;
int SW_TMR_DELAY;
int ACTUAL_TIMER;
static int led_data;
static int btn_value;
static int btn_count;
static int sw_value;
//static int tmr_count;

XTime tStart, tEnd;

//----------------------------------------------------
// PROTOTYPE FUNCTIONS
//----------------------------------------------------
void BTN_Intr_Handler(void *baseaddr_p);
void TMR_Intr_Handler(void *InstancePtr, u8 TmrCtrNumber);
void SW_Intr_Handler(void *baseaddr_p);
int InterruptSystemSetup(XScuGic *XScuGicInstancePtr);
int IntcInitFunction(u16 DeviceId, XTmrCtr *TmrInstancePtr, XGpio *GpioInstancePtr, XGpio *GpioInstancePtr2);
int InterruptSwitchSystemSetup(XScuGic *XScuGicInstancePtr);


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


void SW_Intr_Handler(void *InstancePtr){
	XGpio_InterruptDisable(&SWInst, SW_INT);

	if((XGpio_InterruptGetStatus(&SWInst) & SW_INT) !=
			SW_INT) {
				return;
			}
	(void)XGpio_InterruptClear(&SWInst, SW_INT);

	sw_value = XGpio_DiscreteRead(&SWInst, 1);


	int dontCare3 = 0x8 & sw_value; //Værdi 1xxx på switches ved at maske med and operator

	switch	(sw_value) {
	case 0x0:
		SW_TMR_DELAY = 1;
	case 0x1:
		SW_TMR_DELAY = 10;
		break;
	case 0x2:
		SW_TMR_DELAY = 20;
		break;
	case 0x4:
		SW_TMR_DELAY = 30;
		break;
	case 0x8:
		SW_TMR_DELAY = 60;
		break;
	}

	if(0x8 == dontCare3){
		SW_TMR_DELAY = 60;
	}



	ACTUAL_TIMER = TMR_LOAD * SW_TMR_DELAY;

	//XGpio_DiscreteWrite(&LEDInst, 1, led_data);
    // Enable GPIO interrupts
    XGpio_InterruptEnable(&SWInst, SW_INT);
}


void BTN_Intr_Handler(void *InstancePtr)
{

	volatile int btn_delay;

	// Disable GPIO interrupts
	XGpio_InterruptDisable(&BTNInst, BTN_INT);
	// Ignore additional button presses
	if ((XGpio_InterruptGetStatus(&BTNInst) & BTN_INT) !=
			BTN_INT) {
			return;
		}

	for(btn_delay = 0; btn_delay < BTN_DEBOUNCE; btn_delay++);

	btn_value = XGpio_DiscreteRead(&BTNInst, 1);
	// Increment counter based on button value
	// Reset if centre button pressed
	//led_data = led_data + btn_value;

	if(btn_value == C){ //KNAP C
		btn_count++;
	}

	if(btn_count > 4){
		btn_count = 1;
	}

	switch (btn_count) {
	case 1:
		led_data = 0x8;
		break;
	case 2:
		led_data=0x4;
		break;
	case 3:
		led_data=0x2;
		break;
	case 4:
		led_data=0x1;
		break;
	}

	if(btn_count == 1){
		if(btn_value == A){
			//SKIFT MELLEM 24H/PM
		} else if(btn_value == L){
			printf("Light is on\n");
		}
	}else if(btn_count == 2){

	}else if(btn_count == 3){

	}else if(btn_count == 4){

	}

    XGpio_DiscreteWrite(&LEDInst, 1, led_data);
    (void)XGpio_InterruptClear(&BTNInst, BTN_INT);
    // Enable GPIO interrupts
    XGpio_InterruptEnable(&BTNInst, BTN_INT);
}

void TMR_Intr_Handler(void *InstancePtr, u8 TmrCtrNumber)
{
	double duration;
	static int tmr_count;
	XTime_GetTime(&tEnd);
	XTmrCtr* pTMRInst = (XTmrCtr *) InstancePtr;

	xil_printf("Timer %d interrupt \n", TmrCtrNumber);

	if (TmrCtrNumber==0) { //Handle interrupts generated by timer 0
		duration = ((double)(tEnd-tStart))/COUNTS_PER_SECOND;
		printf("Tmr_interrupt, tmr_count= %d, duration=%.6f s\n\r", tmr_count, (double)duration);

		tStart=tEnd;

		if (XTmrCtr_IsExpired(pTMRInst,0)){
			// Once timer has expired 3 times, stop, increment counter
			// reset timer and start running again
			if(tmr_count == 3){
				XTmrCtr_Stop(pTMRInst,0);
				tmr_count = 0;
				//led_data++;
				//XGpio_DiscreteWrite(&LEDInst, 1, led_data);
				XTmrCtr_Reset(pTMRInst,0);
				XTmrCtr_Start(pTMRInst,0);

			}
			else tmr_count++;
		}
	}
	else {  //Handle interrupts generated by timer 1

	}

	XTmrCtr_ClearInterruptFlag(pTMRInst, TmrCtrNumber);
}



//----------------------------------------------------
// MAIN FUNCTION
//----------------------------------------------------
int main (void)
{
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
  //Initialiser switches
  status = XGpio_Initialize(&SWInst, SW_DEVICE_ID);
  if(status != XST_SUCCESS) return XST_FAILURE;
  // Set LEDs direction to outputs
  XGpio_SetDataDirection(&LEDInst, 1, 0x00);
  // Set all buttons direction to inputs
  XGpio_SetDataDirection(&BTNInst, 1, 0xFF);
  // set all switches direction to inputs
  XGpio_SetDataDirection(&SWInst, 1, 0xFF);


  //----------------------------------------------------
  // SETUP THE TIMER
  //----------------------------------------------------
  status = XTmrCtr_Initialize(&TMRInst, TMR_DEVICE_ID);
  if(status != XST_SUCCESS) return XST_FAILURE;
  XTmrCtr_SetHandler(&TMRInst, TMR_Intr_Handler, &TMRInst);
  XTmrCtr_SetResetValue(&TMRInst, 0, TMR_LOAD);
  XTmrCtr_SetOptions(&TMRInst, 0, XTC_INT_MODE_OPTION | XTC_AUTO_RELOAD_OPTION | XTC_DOWN_COUNT_OPTION);

  // Initialize interrupt controller
  status = IntcInitFunction(INTC_DEVICE_ID, &TMRInst, &BTNInst, &SWInst);
  if(status != XST_SUCCESS) return XST_FAILURE;


  /*//Initialize interrupt controller
   status = SWIntcInitFunction(INTC_DEVICE_ID,&SWInst);
   if(status != XST_SUCCESS) return XST_FAILURE;*/

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
	// Enable interrupt for switches
	/*XGpio_InterruptEnable(&SWInst, SW_INT);
	XGpio_InterruptGlobalEnable(&SWInst);*/

	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
			 	 	 	 	 	 (Xil_ExceptionHandler)XScuGic_InterruptHandler,
			 	 	 	 	 	 XScuGicInstancePtr);
	Xil_ExceptionEnable();


	return XST_SUCCESS;

}

int InterruptSwitchSystemSetup(XScuGic *XScuGicInstancePtr){

	XGpio_InterruptEnable(&SWInst, SW_INT);
	XGpio_InterruptGlobalEnable(&SWInst);

	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
				 	 	 	 	 	 (Xil_ExceptionHandler)XScuGic_InterruptHandler,
				 	 	 	 	 	 XScuGicInstancePtr);
		Xil_ExceptionEnable();


		return XST_SUCCESS;

}



int IntcInitFunction(u16 DeviceId, XTmrCtr *TmrInstancePtr, XGpio *GpioInstancePtr, XGpio *GpioInstancePtr2)
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

	status = XScuGic_Connect(&INTCInst,
						  	  	 INTC_SWGPIO_INTERRUPT_ID,
						  	  	 (Xil_ExceptionHandler)SW_Intr_Handler,
						  	  	 (void *)GpioInstancePtr2);
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

	// Enable GPIO interrupts interrupt SWITCHES
	XGpio_InterruptEnable(GpioInstancePtr2, 1);
	XGpio_InterruptGlobalEnable(GpioInstancePtr2);

	// Enable GPIO and timer interrupts in the controller
	XScuGic_Enable(&INTCInst, INTC_SWGPIO_INTERRUPT_ID);

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
