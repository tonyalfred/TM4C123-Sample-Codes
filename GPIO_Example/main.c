/* Needed Header Files. */
#include <stdint.h>
#include "tm4c123gh6pm.h"

/* Needed #defines. */
#define OFF       0x00
#define RED       0x02
#define BLUE      0x04
#define GREEN     0x08

/* This is a needed function declaration related to the startup code. */
void SystemInit () {}
	
void Init_PORTF ()
{
	/* create a dummy variable in order to provide delay till the clock is provided. */
	volatile uint32_t delay;
	
	/* provide clock to the port. */
	SYSCTL_RCGCGPIO_R = 0x20;
	
	/* the required delay so that no change would occur untill the clock is set. */
	delay = 1;
	
	/* unlock the port so that it would be responsive to upcoming changes. */
  GPIO_PORTF_LOCK_R = 0x4C4F434B;
	
	/* Allow changes to the next ports: PF0, PF1, PF2, PF3, and PF4. */
  GPIO_PORTF_CR_R = 0x1F;
	
	/* Declare PF0 and PF4 as Input Pins, while PF1, PF2 and PF3 as Output Pins. */
	GPIO_PORTF_DIR_R = 0x0E;
	
	/* Disable Analog Mode for all Pins in Port F. */
	GPIO_PORTF_AMSEL_R = 0x00;
	
	/* Enable the digital functions for the used Pins. */
  GPIO_PORTF_DEN_R = 0x1F;
	
	/* Dusable the alternate function option for all Pins in the Port. */
	GPIO_PORTF_AFSEL_R = 0x00;
	
	/* Set to Zero in the Port Control Register, as there is no alternate function used. */
  GPIO_PORTF_PCTL_R = 0x00;
	
	/* Enable the Pull Up resistors for the Input Pins. */
	GPIO_PORTF_PUR_R = 0x11;
}

int main(void)
{
	/* Initialize the Input and Output pins in PORTF. */
	Init_PORTF ();
	
	while (1)
	{
		/* if SW1 is pressed only. */
		if ((GPIO_PORTF_DATA_R & 0x11) == 0x01)
		{
			/* Turn on the RED LED only. */
			GPIO_PORTF_DATA_R = RED;
		}
		/* if SW2 is pressed only. */
		else if ((GPIO_PORTF_DATA_R & 0x11) == 0x10)
		{
		  /* Turn on the BLUE LED only. */
			GPIO_PORTF_DATA_R = BLUE;
		}
		/* if both SW1 and SW2 are pressed together. */
		else if ((GPIO_PORTF_DATA_R & 0x11) == 0x00)
		{
		  /* Turn on the GREEN LED only. */
			GPIO_PORTF_DATA_R = GREEN;
		}
		else
		{
			/* Turn OFF all LEDS. */
			GPIO_PORTF_DATA_R = OFF;
		}
	}
}

