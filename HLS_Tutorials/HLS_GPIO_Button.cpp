//By Ritesh Kanjee
//Initialization of Libraries
#include <stdio.h>
#include "platform.h"
#include "xgpio.h"
#include "xparameters.h"
#include "xscugic.h"
#include "xil_exception.h"
//Lab5 addition start

//Lab5 addition end


#define INTC_DEVICE_ID 			XPAR_PS7_SCUGIC_0_DEVICE_ID
#define BTNS_DEVICE_ID			XPAR_AXI_GPIO_0_DEVICE_ID
#define INTC_GPIO_INTERRUPT_ID  XPAR_FABRIC_AXI_GPIO_0_IP2INTC_IRPT_INTR
#define BTN_INT 				XGPIO_IR_CH1_MASK // This is the interrupt mask for channel one


//Lab5 addition start
#define TMR_DEVICE_ID			XPAR_TMRCTR_0_DEVICE_ID
#define INTC_TMR_INTERRUPT_ID 	XPAR_FABRIC_AXI_TIMER_0_INTERRUPT_INTR

#define TMR_LOAD				0xFD050F7F
/* IMPORTANT NOTE: The AXI timer frequency is set to 50Mhz
 * the timer is set up to be counting UP, these two facts affect the value selected for
 * the load register to generate 1 Hz interrupts
*/


int main()
{
	//Initialization of Variables
    init_platform();
    XGpio push, led; // instance pointer for the LEDs
    int i,push_check;
	int Status;
	int count;
	int delay;
	int val=1;

    print("Hello World Ritz1\n\r");

    //AXI GPIO Initialization
        XGpio_Initialize(&push,XPAR_AXI_GPIO_BUT_DEVICE_ID);
    	XGpio_Initialize(&led,XPAR_AXI_GPIO_0_DEVICE_ID);

    //AXI GPIO Set Direction
    	XGpio_SetDataDirection(&push,1,0xffffffff);
    	XGpio_SetDataDirection(&led,1,0x00000000);

    //Reading from the push buttons
    	while (1){
    	push_check= XGpio_DiscreteRead(&push,1);
    	xil_printf("Push Buttons Status: %d \r \n",push_check);
    			switch (push_check){

    			//Checking if BTNC was pressed
    			case 1:
    				val= 2;
    				break;
    			//Checking if BTND was pressed
    			case 2:
    				val = -1;
    				break;

    			default:
    			break;
    			}

    	xil_printf("Value of val: %d \r \n",val);
    	}
	//Code to Blink LED
/*	for(count = 0; count < 500; count++)
	{
		XGpio_DiscreteWrite(&led,1,4);

		for (delay=0; delay< 2000; delay++)
		{
			print(".");
		}
		XGpio_DiscreteWrite(&led,1,0);

		for (delay=0; delay< 2000; delay++)
		{
			print(".");
		}
	}*/

	print("Done\n\r");

    //cleanup_platform();
    return 0;
}
