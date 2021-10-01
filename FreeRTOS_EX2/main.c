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
#define STACK_SIZE_TASK_1 200
#define STACK_SIZE_TASK_2 200

/* Queue used to send initials of LEDS to be used. */
QueueHandle_t  xQueue1;

/* Handles for the tasks. */
TaskHandle_t First_Handle, Second_Handle;

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
/*                              Task Definition                             */
/*****************************************************************************/

/* Task1 with priority 1 */
static void MyTask1(void* pvParameters)
{
  while(1)
  {
    UART_sendString (UART0_BASE, "Task 1. \n\r");
    vTaskDelay(100/portTICK_PERIOD_MS);
  }
}


/* Task2 with priority 2 */
static void MyTask2(void* pvParameters)
{
  while(1)
  {
    UART_sendString (UART0_BASE, "Task 2. \n\r");
    vTaskDelay(150/portTICK_PERIOD_MS);
  }
}


/* Idle Task with priority Zero */
static void MyIdleTask(void* pvParameters)
{
  while(1)
  {
    UART_sendString (UART0_BASE, "Idle State. \n\r");
    vTaskDelay(50);
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

    /* Create Tasks that send string to UART. */
    xTaskCreate(MyIdleTask, "IdleTask", 100, NULL, 0, NULL);
    xTaskCreate(MyTask1, "Task1", 100, NULL, 1, NULL);
    xTaskCreate(MyTask2, "Task2", 100, NULL, 2, NULL);

    vTaskStartScheduler();

    while (1)
    { }

    return 0;
}
