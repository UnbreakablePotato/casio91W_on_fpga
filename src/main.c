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
#include "xuartps.h"

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
//#define TMR_LOAD			100000000


//Vores defines
#define INTC_SWGPIO_INTERRUPT_ID XPAR_FABRIC_AXI_GPIO_2_IP2INTC_IRPT_INTR
#define	SW_INT				XGPIO_IR_CH1_MASK
#define A	0x1
//#define C	0x2
#define C	0x4
#define L	0x8
#define BTN_DEBOUNCE 3000000
#define BTN_DEBOUNCE_STOPWATCH  15000000

#define BTN_DEBOUNCE_TIME 7000000
volatile int btn_delay;

XGpio LEDInst, BTNInst, SWInst;
XScuGic INTCInst;
XTmrCtr TMRInst;
int SW_TMR_DELAY;
int ACTUAL_TIMER;
static int led_data;
volatile int btn_value = 0;
volatile int btn_count = 1;
static int sw_value;
volatile int on;
int stopWatchBTN_CNT = 0;
//static int tmr_count;

int TMR_LOAD = 1000000;
//volatile int TMR_STOP_LOAD = 1000000;

//global viables for clock
volatile int seconds = 0;
volatile int minutes = 0;
volatile int hours = 0;

//global variables for stopWatch
volatile int stopWatch100thSecond = 0;
volatile int stopWatchSeconds = 0;
volatile int stopWatchMinutes = 0;
int lastBtnValue = 0;

int flagSec = 0;


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
void stopWatch();
void setTime();
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



void stopWatch()
{
	stopWatch100thSecond = 0;
	stopWatchSeconds = 0;
	stopWatchMinutes = 0;
	printDigits(stopWatchMinutes, stopWatchSeconds, stopWatch100thSecond);


	while(1){
		for(btn_delay = 0; btn_delay < BTN_DEBOUNCE_STOPWATCH; btn_delay++);

		btn_value = XGpio_DiscreteRead(&BTNInst, 1);

		if(btn_count != 3){
			break;
		}

		if(btn_value == A && on != 1){
			on = 1;
		} else if (btn_value == A && on == 1){
			on = 0;
		} else if(btn_value == L){
			stopWatch100thSecond = 0;
			stopWatchSeconds = 0;
			stopWatchMinutes = 0;
			printDigits(stopWatchMinutes, stopWatchSeconds, stopWatch100thSecond);
			on = 0;

		}
	}
}

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
	int dontCare2 = 0x4 & sw_value;
	int dontCare1 = 0x2 & sw_value;
	int dontCare0 = 0x1 & sw_value;

	if(0x8 == dontCare3){
		TMR_LOAD = 16667;
	}else if(0x4 == dontCare2){
		TMR_LOAD = 33333;
	}else if(0x2 == dontCare1){
		TMR_LOAD = 50000;
	}else if(0x1 == dontCare0){
		TMR_LOAD = 100000;
	}else{
		TMR_LOAD = 1000000;
	}

	//1000000
	//1000000

	  XTmrCtr_SetResetValue(&TMRInst, 0, TMR_LOAD);

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

    //XGpio_DiscreteWrite(&LEDInst, 1, led_data);
    (void)XGpio_InterruptClear(&BTNInst, BTN_INT);
    // Enable GPIO interrupts
    XGpio_InterruptEnable(&BTNInst, BTN_INT);
}

void TMR_Intr_Handler(void *InstancePtr, u8 TmrCtrNumber)
{
	double duration;
	double stopWatchDuration;
	static int tmr_count;
	static int stopWatch_tmrCount;
	XTime_GetTime(&tEnd);
	XTmrCtr* pTMRInst = (XTmrCtr *) InstancePtr;

	//xil_printf("Timer %d interrupt \n", TmrCtrNumber);

	/*if(TmrCtrNumber == 0 && btn_count == 3){
		stopWatchDuration = (((double)(tEnd-tStart))/COUNTS_PER_SECOND)/100;// 1 hundrede dele af et sekund?
		printf("Tmr_interrupt, tmr_count= %d, stopWatchDuration=%.6f s\n\r", tmr_count, (double)stopWatchDuration);

		tStart=tEnd;

		if(XTmrCtr_IsExpired(pTMRInst,0)){
			if(stopWatch_tmrCount == 1){
				XTmrCtr_Stop(pTMRInst,0);
				stopWatch_tmrCount = 0;

				XTmrCtr_Reset(pTMRInst,0);
				XTmrCtr_Start(pTMRInst,0);
			}
			else stopWatch_tmrCount++;
		}

	}*/

	if (TmrCtrNumber==0) { //Handle interrupts generated by timer 0
		duration = ((double)(tEnd-tStart))/COUNTS_PER_SECOND;
		//printf("Tmr_interrupt, tmr_count= %d, duration=%.6f s\n\r", tmr_count, (double)duration);

		tStart=tEnd;

		if (XTmrCtr_IsExpired(pTMRInst,0)){
			// Once timer has expired 3 times, stop, increment counter
			// reset timer and start running again
			if(tmr_count == 100){
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
	if (TmrCtrNumber == 0) {
		stopWatch100thSecond++;
		if(stopWatch100thSecond >= 99){
			stopWatch100thSecond = 0;
			if(++seconds >= 60){
				seconds = 0;
				if(++minutes >= 60){
					minutes = 0;
					hours = (hours + 1) % 24;
				}
			}
			if(flagSec == 0 && btn_count!=3){
				printDigits(hours, minutes, seconds);
			}
		}
		if(flagSec == 1){
						printDigits(hours, minutes, seconds);
		}

	    }

	if(TmrCtrNumber == 0 && btn_count == 3 && on == 1){
			stopWatch100thSecond++;
			if(stopWatch100thSecond >= 99){
				stopWatch100thSecond = 0;
				if(++stopWatchSeconds >= 60){
					stopWatchSeconds = 0;
					if(++stopWatchMinutes >= 60){
						stopWatchMinutes = 0;
					}
				}
			}

			printDigits(stopWatchMinutes, stopWatchSeconds, stopWatch100thSecond);



	}

	XTmrCtr_ClearInterruptFlag(pTMRInst, TmrCtrNumber);
}

void setTime(){
    // determines the setting to adjust, for ex. i = 0 is seconds
	int tmp_seconds;
	int tmptmp_seconds;
	//int flagSec = 0;
    int i = 0;

    while (1)
    {

        // read button value
        btn_value = XGpio_DiscreteRead(&BTNInst, 1);

        // exit if next mode is chosen
        if (btn_count != 4) {
            break;
        }

        // seconds
        if (i == 0) {
        	//increments variable
            if (btn_value == A) {
                seconds++;
                tmp_seconds = seconds;
                tmptmp_seconds = tmp_seconds;
                // debounce so one press = one increment, important. without it we increment several times
                while (XGpio_DiscreteRead(&BTNInst, 1) == A){
                	if(((tmp_seconds -= tmptmp_seconds) >= 3) || flagSec == 1){
                		flagSec = 1;
                		seconds++;
                        if (seconds >= 60) {
                        	minutes++;
                        	seconds = 0;
                        }
                		for(btn_delay = 0; btn_delay < BTN_DEBOUNCE_TIME; btn_delay++);
                	}
                	tmp_seconds = seconds;
                }
                flagSec = 0;
            }
            //change variable/setting to adjust
            if (btn_value == L) {
                i++;
                for(btn_delay = 0; btn_delay < BTN_DEBOUNCE_TIME; btn_delay++);
            }
        }
        // hours
        else if (i == 1) {
            if (btn_value == A) {
                hours++;
                tmp_seconds = seconds;
                tmptmp_seconds = tmp_seconds;
                // debounce so one press = one increment, important. without it we increment several times
                while (XGpio_DiscreteRead(&BTNInst, 1) == A){
                	if(((tmp_seconds -= tmptmp_seconds) >= 3) || flagSec == 1){
                		flagSec = 1;
                		hours++;
                        if (hours >= 24) {
                        	hours = 0;
                        }
                		for(btn_delay = 0; btn_delay < BTN_DEBOUNCE_TIME; btn_delay++);
                	}
                	tmp_seconds = seconds;
                }
                flagSec = 0;
            }
            if (btn_value == L) {
                i++;
                for(btn_delay = 0; btn_delay < BTN_DEBOUNCE_TIME; btn_delay++);
            }
        }
        // minutes
        else if (i == 2) {
            if (btn_value == A) {
                minutes++;
                tmp_seconds = seconds;
                tmptmp_seconds = tmp_seconds;
                // debounce so one press = one increment, important. without it we increment several times
                while (XGpio_DiscreteRead(&BTNInst, 1) == A){
                	if(((tmp_seconds -= tmptmp_seconds) >= 3) || flagSec == 1){
                		flagSec = 1;
                		minutes++;
                        if (minutes >= 60) {
                        	hours++;
                        	minutes = 0;
                        }
                		for(btn_delay = 0; btn_delay < BTN_DEBOUNCE_TIME; btn_delay++);
                	}
                	tmp_seconds = seconds;
                }
                flagSec = 0;
            }
            if (btn_value == L) {
                i = 0;
                for(btn_delay = 0; btn_delay < BTN_DEBOUNCE_TIME; btn_delay++);
            }
        }

        //sets variable to zero if we exceeds wanted value
        if (seconds >= 60) {
        	minutes++;
        	seconds = 0;
        }
        if (hours >= 24) {
        	hours = 0;
        }
        if (minutes >= 60) {
        	hours++;
        	minutes = 0;
        }

    }

    return;

}




//----------------------------------------------------
// MAIN FUNCTION
//----------------------------------------------------
int main (void)
{
  int status;

   XUartPs Uart_PS;
   XUartPs_Config *Config;

   Config = XUartPs_LookupConfig(XPAR_XUARTPS_0_DEVICE_ID);
   XUartPs_CfgInitialize(&Uart_PS, Config, Config->BaseAddress);

   // Override baud rate here
   XUartPs_SetBaudRate(&Uart_PS, 921600);

  //XTmrCtr TMRInst;
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

  while(1){
	  switch (btn_count) {
	  	case 1:
	  		led_data = 0x8;
	  		XGpio_DiscreteWrite(&LEDInst, 1, led_data);
	  		on = 0;
	  		break;
	  	case 2:
	  		led_data=0x4;
	  		XGpio_DiscreteWrite(&LEDInst, 1, led_data);
	  		on = 0;
	  		break;
	  	case 3:
	  		led_data=0x2;
	  		XGpio_DiscreteWrite(&LEDInst, 1, led_data);
	  		stopWatch();
	  		break;
	  	case 4:
	  		led_data=0x1;
	  		XGpio_DiscreteWrite(&LEDInst, 1, led_data);
	  		on = 0;
	  		setTime();
	  		break;
	  	}
  }

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

