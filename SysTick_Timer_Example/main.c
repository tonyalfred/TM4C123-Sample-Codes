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

void SysTick_Timer (uint32_t delay)
{
	/* Disable teh countting, untill the options of the systic timer is updated. */
	NVIC_ST_CTRL_R = 0;
	/* Set the number of ticks that the timer would need to reach to end the delay. */
	NVIC_ST_RELOAD_R = delay - 1;     
  /* Clear the control register, as to reset our count from ZERO. */	
	NVIC_ST_CURRENT_R = 0;
	/* Enable the systic timer, with no interrupt, and with use of System Clock */
	NVIC_ST_CTRL_R |= 5; 
	/* Wait untill the Count Flag is set. */
	while((NVIC_ST_CTRL_R & 0x00010000) == 0) {}
}

int main(void)
{
	Init_PORTF ();
	while (1)
	{
		/* if SW1 is pressed only. */
		if ((GPIO_PORTF_DATA_R & 0x11) == 0x01)
		{
			/* Toggle All 3 LEDS: RED, GREEN and BLUE, every 1 second. */
			GPIO_PORTF_DATA_R ^= 0x0E;
			SysTick_Timer (16000000);	
		}
		/* if SW2 is pressed only. */
		else if ((GPIO_PORTF_DATA_R & 0x11) == 0x10)
		{
			/* Toggle RED, wait 1 second, Toggle BLUE, wait 1 second, Toggle Green, 
			   and then wait also another 1 second. */
			GPIO_PORTF_DATA_R ^= RED;
			SysTick_Timer (16000000);
			GPIO_PORTF_DATA_R ^= BLUE;
			SysTick_Timer (16000000); 
	    GPIO_PORTF_DATA_R ^= GREEN;
			SysTick_Timer (16000000);
		}
		/* if both SW1 and SW2 are pressed together. */
		else if ((GPIO_PORTF_DATA_R & 0x11) == 0x00)
		{
			/* Toggle 1 LED for 1 second, and then toggle the next LED, with turning 
			OFF the previous LED, in a successive manner. */
			GPIO_PORTF_DATA_R ^= RED;
			SysTick_Timer (16000000);
			GPIO_PORTF_DATA_R ^= RED;
			GPIO_PORTF_DATA_R ^= BLUE;
			SysTick_Timer (16000000); 
			GPIO_PORTF_DATA_R ^= BLUE;
	    GPIO_PORTF_DATA_R ^= GREEN;
			SysTick_Timer (16000000);
			GPIO_PORTF_DATA_R ^= GREEN;
		}
		else 
		{
			/* Turn OFF all LEDS for 1 second, and keep it that way untill further information. */
		  GPIO_PORTF_DATA_R = OFF;
			SysTick_Timer (16000000);
		}
	}
}
