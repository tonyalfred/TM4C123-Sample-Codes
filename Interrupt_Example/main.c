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
	
/* This is code that used Interupts with GPIO, where, we will use a PIN to Toggle the Green LED, 
and another another PIN to turn it OFF. */
	
int main(void)
{
    /* Set bit5 of RCGCGPIO to enable clock to PORTF*/
	  SYSCTL_RCGCGPIO_R |= (1<<5);   
    
	  /* PORTF0 has special function, need to unlock to modify */
	  /* unlock commit register */
    GPIO_PORTF_LOCK_R = 0x4C4F434B; 
	
    /* make PORTF0 configurable */	
    GPIO_PORTF_CR_R = 0x01;    
	
	  /* lock commit register */
    GPIO_PORTF_LOCK_R = 0;            


    /*Initialize PF3 as a digital output, PF0 and PF4 as digital input pins */
	  /* Set PF4 and PF0 as a digital input pins */
    GPIO_PORTF_DIR_R &= ~(1<<4)|~(1<<0); 
	
    /* Set PF3 as digital output to control green LED */	
    GPIO_PORTF_DIR_R |= (1<<3);      
	
	  /* make PORTF4-0 digital pins */
    GPIO_PORTF_DEN_R |= (1<<4)|(1<<3)|(1<<0);       
		
    /* enable pull up for PORTF4, 0 */	
    GPIO_PORTF_PUR_R |= (1<<4)|(1<<0);             
    
    /* configure PORTF4, 0 for falling edge trigger interrupt */
		/* make bit 4, 0 edge sensitive */
    GPIO_PORTF_IS_R  &= ~(1<<4)|~(1<<0);        
		
		/* trigger is controlled by IEV */
    GPIO_PORTF_IBE_R &=~(1<<4)|~(1<<0);    
    
    /* falling edge trigger */		
    GPIO_PORTF_IEV_R &= ~(1<<4)|~(1<<0);     
    
    /* clear any prior interrupt */		
    GPIO_PORTF_ICR_R |= (1<<4)|(1<<0);  
    
    /* unmask interrupt */		
    GPIO_PORTF_IM_R  |= (1<<4)|(1<<0);          
    
    /* enable interrupt in NVIC and set priority to 3 */
		NVIC_PRI7_R = ((NVIC_PRI7_R & 0xFF00FFFF) | 0x00A00000);
		NVIC_EN0_R = 0x40000000;
    
    while(1)
    {
			// do nothing and wait for the interrupt to occur
    }
}

/* SW1 is connected to PF4 pin, SW2 is connected to PF0. */
/* Both of them trigger PORTF falling edge interrupt */
void GPIOF_Handler(void)
{	
  if (GPIO_PORTF_MIS_R & 0x10) /* check if interrupt causes by PF4/SW1*/
    {   
      GPIO_PORTF_DATA_R |= (1<<3);
      GPIO_PORTF_ICR_R |= 0x10; /* clear the interrupt flag */
     } 
    else if (GPIO_PORTF_MIS_R & 0x01) /* check if interrupt causes by PF0/SW2 */
    {   
     GPIO_PORTF_DATA_R &= ~0x08;
     GPIO_PORTF_ICR_R |= 0x01; /* clear the interrupt flag */
    }
}
