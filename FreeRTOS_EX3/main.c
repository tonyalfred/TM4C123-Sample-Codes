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
#define PINS     GPIO_PIN_0
#define LEDS     GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3
#define LEDS_OFF 0
#define RED      2
#define BLUE     4
#define GREEN    8

/* FreeRTOS #defines */
#define STACK_SIZE_TASK_1 200
#define STACK_SIZE_TASK_2 200

/* Semaphore used to synchronize turning LEDS ON and OFF. */
SemaphoreHandle_t xBinarySemaphore;

/* Handles for the tasks. */
TaskHandle_t First_Handle, Second_Handle;

/*****************************************************************************/
/*                               Function Prototypes                         */
/*****************************************************************************/

void SystemInit (void){}
void UART0_Init (void);
void PORTF_Init (void);
void GPIOFIntHandler (void);

/*****************************************************************************/
/*                                 Main Functions                            */
/*****************************************************************************/
void UART_receiveString (uint32_t ui32Base, uint8_t *Str);
void UART_sendString (uint32_t ui32Base, const uint8_t *Str);

void PORTF_Init (void)
{
    /* Enable Clock for the GPIO Port F peripheral. */
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    /* Wait for the GPIOF module to be ready. */
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOF)) {}
    /* Unlocking closed GPIO Pins that we will use as both
     * switches (PF0, PF4) and as LEDS (PF1, PF2, PF3). */
    GPIOUnlockPin(GPIO_PORTF_BASE, PINS | LEDS);
    /* Set Pins (PF1, PF2, PF3) as Output Pins. */
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, LEDS);
    /* Set Pins (PF0, PF4) as Input Pins. */
    GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, PINS);
    /* Set the Pad Configuration for the Output Pins. */
    GPIOPadConfigSet(GPIO_PORTF_BASE, PINS, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
    /* Enable interrupt on the enable pins. */
    GPIOIntEnable(GPIO_PORTF_BASE, PINS);
    /* Determine which Interrupt Handler to use upon Interrupt. */
    GPIOIntRegister(GPIO_PORTF_BASE, GPIOFIntHandler);
    /* Determine Interrupt to occur on falling edge. */
    GPIOIntTypeSet(GPIO_PORTF_BASE, PINS, GPIO_FALLING_EDGE);
    /* Enable Interrupts for both pins (PF0, PF4). */
    GPIOIntEnable(GPIO_PORTF_BASE, PINS);
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

void GPIOFIntHandler ()
{
    volatile uint32_t status = GPIOIntStatus(GPIO_PORTF_BASE, true);
    UART_sendString (UART0_BASE, "Semaphore is given. \n\r");
    xSemaphoreGiveFromISR(xBinarySemaphore, (TickType_t) 0);
    GPIOIntClear(GPIO_PORTF_BASE, status);
}
/*******************************************************************************
 *                            UART Functions Prototypes                        *
 *******************************************************************************/



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
/*                              Task Definition                             */
/*****************************************************************************/

void LED_ON_Task (void *pvParameters)
{
    while (1)
    {
        xSemaphoreTake(xBinarySemaphore, portMAX_DELAY);
        UART_sendString (UART0_BASE, "Inside LED_ON_Task. \n\r");
        GPIOPinWrite(GPIO_PORTF_BASE, LEDS, BLUE);
        xSemaphoreGive(xBinarySemaphore);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
void LED_OFF_TASK (void *pvParameters)
{
    while (1)
    {
        xSemaphoreTake(xBinarySemaphore, portMAX_DELAY);
        UART_sendString (UART0_BASE, "Inside LED_OFF_Task. \n\r");
        GPIOPinWrite(GPIO_PORTF_BASE, LEDS, LEDS_OFF);
        xSemaphoreGive(xBinarySemaphore);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

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

    /* Create a Semaphore to allow proper synchronization between two tasks. */
    xBinarySemaphore = xSemaphoreCreateBinary();

    if( xSemaphoreGive( xBinarySemaphore ) != pdTRUE )
    {
        UART_sendString (UART0_BASE, "Failed to Give Semaphore. \n\r");
    }

    /* Create Tasks that receives, filters, then, sets the specific needed LEDS. */
    xTaskCreate(LED_ON_Task,  "LED_ON_TASK" , STACK_SIZE_TASK_1, NULL, 1, &First_Handle);
    xTaskCreate(LED_OFF_TASK, "LED_OFF_TASK", STACK_SIZE_TASK_2, NULL, 1, &Second_Handle);

    vTaskStartScheduler();

    while (1)
    { }

    return 0;
}
