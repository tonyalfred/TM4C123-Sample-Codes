/*
 * Use UART0 to PC Communication to control LEDs using FreeRTOS.
 * Author: Tony Alfred
 */

/*****************************************************************************/
/*                                 HEADER FILES                              */
/*****************************************************************************/

/* Libraries includes.  */
#include <stdint.h>
#include <stdbool.h>

/* Header Files Includes.  */


/* TivaWare includes.  */
#include "inc/hw_memmap.h"
#include "inc/tm4c123gh6pm.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/uart.h"
#include "utils/uartstdio.h"

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/* Main Code #defines */
#define PINS     GPIO_PIN_0 | GPIO_PIN_1
#define LEDS     GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3
#define LEDS_OFF 0
#define RED      2
#define BLUE     4
#define GREEN    8

/* FreeRTOS #defines */
#define STACK_SIZE_TASK_1 1500
#define STACK_SIZE_TASK_2 1250
#define STACK_SIZE_TASK_3 1000
#define STACK_SIZE_TASK_4 800

/*****************************************************************************/
/*                               Function Prototypes                         */
/*****************************************************************************/

void SystemInit (void){}
void UART0_Init (void);
void PORTF_Init (void);

/*****************************************************************************/
/*                                 Main Functions                            */
/*****************************************************************************/

void PORTF_Init (void)
{
    /* Enable Clock for the GPIO Port F peripheral. */
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    /* Wait for the GPIOF module to be ready. */
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOF)) {}
    /* Initialize the GPIO pin configuration. */
    /* Unlocking closed GPIO pins (PF1, PF2, PF3) that we will use as LEDS. */
    GPIOUnlockPin(GPIO_PORTF_BASE, LEDS);
    /* Set Pins (PF1, PF2, PF3) as Output Pins. */
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, LEDS);
}

void UART0_Init (void)
{
    /* Enable Clock for the UART0 module. */
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    /* Wait for the UART0 module to be ready. */
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_UART0)) {}
    /* Enable Clock for the GPIO Port A. */
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    /* Wait for the GPIOA module to be ready. */
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA)){}
    /*Configure PA0 for UART0 Rx and PA1 for UART0 Tx*/
    GPIOPinConfigure (GPIO_PA0_U0RX);
    GPIOPinConfigure (GPIO_PA1_U0TX);
    GPIOPinTypeUART  (GPIO_PORTA_BASE, PINS);
    /* Initialize the UART, by choosing UART0, baud rate as 128000,
     * 8 bits mode, 1 stop bit, and no parity bit. */
    UARTConfigSetExpClk (UART0_BASE, SysCtlClockGet(), 128000, (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |UART_CONFIG_PAR_NONE));
    /* Finalize the initialization process by enabling the module to transmit and receive FIFOs. / */
    UARTEnable(UART0_BASE);
}

/*******************************************************************************
 *                            UART Functions Prototypes                        *
 *******************************************************************************/

void UART_receiveString (uint32_t ui32Base, uint8_t *Str);
void UART_sendString (uint32_t ui32Base, const uint8_t *Str);

/*****************************************************************************/
/*                                 UART Functions                            */
/*****************************************************************************/

/*
 * The reason in the next two UART functions, we added the ui32Base argument
 * is because it would allow those functions to be generic to all UART modules
 * in our MC.
 */

void UART_receiveString(uint32_t ui32Base, uint8_t *Str)
{
    /* Counter Declaration to be used to loop until string is sent. */
    uint8_t i = 0;

    /* Receive the first character to use it as initial sentinel for the loop. */
    Str[i] = UARTCharGet (ui32Base);

    /* Receive characters until the ENTER key is pressed. */
    /* 13 is the ASCII equivalent for the ENTER button. */
    while(Str[i] != 13)
    {
        /* Receive the next character that follows the initial sentinel we used. */
        i++;
        Str[i] = UARTCharGet (ui32Base);
    }
    /* Enter the NULL at the end, so the string would be valid. */
    Str[i] = '\0';
}

void UART_sendString(uint32_t ui32Base, const uint8_t *Str)
{
    /* Counter Declaration to be used to loop until string is sent. */
    uint8_t i = 0;

    while(Str[i] != '\0')
    {
        /* Send the characters one by one. */
        UARTCharPut(ui32Base, Str[i]);
        i++;
    }
}

/*****************************************************************************/
/*                      Task Entry Function Definition                       */
/*****************************************************************************/

/* Queue used to send initials of LEDS to be used. */
QueueHandle_t xQueue1 = NULL;

void vTask1 (void * pvParameters)
{
    /* Create a character that stores the initials of the color of LEDS required. */
    uint8_t ReceivedCharacter;

    /* Initialize a queue of 10 slots, that send these initials to the task that changes the LEDS. */
    xQueue1 = xQueueCreate (10, sizeof(uint8_t));

    if( xQueue1 == NULL )
    {
        /* Queue was not created and must not be used.
         * Send a message to the PC indicating Error that requires debugging.
         * Suspend this current task until further notice, which should allow other tasks to work if needed be.
         */
        UART_sendString (UART0_BASE, "An error has occurred that ruins the system, Please Debug and then Reset. \n\r");
        vTaskSuspend (NULL);
    }

    /* Continue here if Queue was initialized successfully and print this on PC screen. */
    UART_sendString (UART0_BASE, "Please enter r, g or b at any given moment to toggle the LED accordingly: \n\r");

    while (1)
    {
        ReceivedCharacter = UARTCharGet(UART0_BASE);
        switch (ReceivedCharacter)
            {
            case 'r':
            case 'g':
            case 'b':
                /* if the queue is full, wait for upwards to 100 ticks, until there is an empty slot in the queue.*/
                if( xQueueSend(xQueue1, (void *) &ReceivedCharacter, (TickType_t) 100) != pdPASS )
                {
                    /* if the queue wasn't freed for over 100 ticks, then send this error message on the screen and carry on. */
                    UART_sendString (UART0_BASE, "The last color preference will be unable to be used. \n\r");
                }
            }
    }
}

void vTask2 (void * pvParameters)
{
    /* Create a variable that stores the initial of the color to be used. */
    uint8_t Leds_State = LEDS_OFF;

    while (1)
    {
        /* Receive the initial from the queue and store in the variable Leds_State.
         * These queues will not block if empty, as its not a rule that the user has to constantly change the color
         * and also we wont use the if statement here, we used in the xQueueSend for the same reason.
         */
        xQueueReceive(xQueue1, &(Leds_State), (TickType_t) 0);

        /* Constantly check if the Leds_State was updated. */
        switch (Leds_State)
        {
        case 'r':
            GPIOPinWrite(GPIO_PORTF_BASE, LEDS, RED);
            break;
        case 'b':
            GPIOPinWrite(GPIO_PORTF_BASE, LEDS, BLUE);
            break;
        case 'g':
            GPIOPinWrite(GPIO_PORTF_BASE, LEDS, GREEN);
            break;
        }
    }
}



/*

while (1)
{
    vTaskDelayUntil (&xLastWakeTime, 1000*Rate);
    GPIOPinWrite(GPIO_PORTF_BASE, LEDS, LEDS_STATE);
    LEDS_STATE ^= 0xFF;
}


void vTask3 (void * pvParameters)
{

    TickType_t xLastWakeTime;
    xLastWakeTime = xTaskGetTickCount ();

    while (1)
    {
        vTaskDelayUntil (&xLastWakeTime, 1000);
    }
}

void vTask4 (void * pvParameters)
{

    TickType_t xLastWakeTime;
    xLastWakeTime = xTaskGetTickCount ();

    while (1)
    {
        vTaskDelayUntil (&xLastWakeTime, 1000);
    }
}
*/

/*****************************************************************************/
/*                               Main Function                               */
/*****************************************************************************/

int main (void)
{
    /* Enable Clock of MCU with no Pre-Scalar, use OSC, main oscillator source, 16MHz external crystal frequency*/
    SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);

    /* Initialize PORTF through PINS PF1, PF2 and PF3 to be used as LEDS. */
    PORTF_Init ();

    /* Initialize UART0 through PINS PA0 and PA2 as Rx and Tx. / */
    UART0_Init ();

    /* Create 4 handles for our 4 tasks. */
    TaskHandle_t First_Handle, Second_Handle; // Third_Handle, Fourth_Handle;

    /*
     * Create 4 different tasks.
     */

    xTaskCreate(vTask1, "Task_UART_RECEIVE",  STACK_SIZE_TASK_1, NULL, 2, &First_Handle);
    xTaskCreate(vTask2, "Task_LEDS_COLOUR",   STACK_SIZE_TASK_2, NULL, 1, &Second_Handle);

    /*
    xTaskCreate(vTask3, "Task_BUTTON_TOGGLE", STACK_SIZE_TASK_3, NULL, 3, &Third_Handle);
    xTaskCreate(vTask4, "Task_BUTTON_OFF",    STACK_SIZE_TASK_4, NULL, 4, &Fourth_Handle);
    */

    /* Start the scheduler responsible for switching between tasks in FreeRTOS. */
    vTaskStartScheduler();

    while (1)
    { }

    return 0;
}
