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
#include "queue.h"
#include "timers.h"
#include "task.h"

/* Main Code #defines */
#define PINS     GPIO_PIN_0 | GPIO_PIN_4
#define LEDS     GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3
#define SW1      GPIO_PIN_4
#define SW2      GPIO_PIN_0
#define LEDS_OFF 0
#define RED      2
#define BLUE     4
#define GREEN    8

/* FreeRTOS #defines */
#define STACK_SIZE_TASK_1 200
#define STACK_SIZE_TASK_2 150

/*****************************************************************************/
/*              Global Variables and Needed FreeRTOS Declarations            */
/*****************************************************************************/

uint16_t milli_seconds [5] = {1000, 2000, 3000, 4000, 5000};
static volatile int8_t global_counter = 0;

/* Queue used to send initials of LEDS to be used. */
QueueHandle_t  xQueue1;

/* Handles for the tasks. */
TaskHandle_t First_Handle, Second_Handle;

/* Handle for the Software Timer. */
TimerHandle_t xTimer;

/*****************************************************************************/
/*                           Function Prototypes                             */
/*****************************************************************************/

void SystemInit (void){}

void UART0_Init (void);
void PORTF_Init (void);

void GPIOFIntHandler (void);
void vTimerCallback(TimerHandle_t xTimer);

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
/*                         Initialization Functions                          */
/*****************************************************************************/

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

/*****************************************************************************/
/*                        Interrupt and Timer Functions                      */
/*****************************************************************************/

void GPIOFIntHandler ()
{
    /* Volatile Variable to store the status of the which Pin is used. */
    volatile uint32_t status = GPIOIntStatus(GPIO_PORTF_BASE, true);

    /* Pin responsible for Timer is pressed. */
    if( (status & SW1) == SW1)
    {

        if( xTimerIsTimerActive(xTimer) != pdFALSE )
        {
            /* xTimer is active, therefore, change its period and decrease the toggling rate of the LEDS. */
            if (global_counter == 4)
            {
                global_counter = -1;
            }
            global_counter ++;
            xTimerChangePeriodFromISR (xTimer, pdMS_TO_TICKS(milli_seconds[global_counter]), (TickType_t)0);
        }
        else
        {
            /* xTimer is not active, Start it. */
            xTimerStartFromISR(xTimer, (TickType_t)0);
        }
    }

    /* Pin responsible for turning the LEDS OFF is pressed. */
    if( (status & SW2) == SW2)
    {
        GPIOPinWrite(GPIO_PORTF_BASE, LEDS, LEDS_OFF);
        UART_sendString (UART0_BASE, "All LEDS should be OFF. \n\r");
    }

    /* Clear the interrupts so that we dont re-enter the Interrupt handler endlessly. */
    GPIOIntClear(GPIO_PORTF_BASE, status);
}

void vTimerCallback(TimerHandle_t xTimer)
{
    /* As the Pin is connected as PULL UP, therefore, wait until the value gives ZERO. */
    if ( GPIOPinRead(GPIO_PORTF_BASE, SW1) == 0)
    {
        /*delay to avoid bouncing effect*/
        vTaskDelay(pdMS_TO_TICKS(300));

        if (GPIOPinRead(GPIO_PORTF_BASE, SW1) == 0)
        {
            GPIOPinWrite(GPIO_PORTF_BASE, LEDS, ~GPIOPinRead(GPIO_PORTF_BASE, LEDS));
        }
    }
}

/*****************************************************************************/
/*                              Task Definition                             */
/*****************************************************************************/

void vTask1 (void * pvParameters)
{
    uint8_t ReceivedCharacter;
    xQueue1 = xQueueCreate (10, sizeof(uint8_t));
    if( xQueue1 == NULL )
    {
        UART_sendString (UART0_BASE, "An error has occurred that ruins the system, Please Debug and then Reset. \n\r");
    }

    UART_sendString (UART0_BASE, "Please enter r, g or b at any given moment: \n\r");

    while (1)
    {
        ReceivedCharacter = UARTCharGet(UART0_BASE);
        switch (ReceivedCharacter)
        {
        case 'r':
        case 'g':
        case 'b':
            if( xQueueSend(xQueue1, (void *) &ReceivedCharacter, (TickType_t) 100) != pdPASS )
            {
                UART_sendString (UART0_BASE, "The last color preference will be unable to be used. \n\r");
            }
            vTaskSuspend(NULL);
        }
    }
}

void vTask2 (void * pvParameters)
{
    uint8_t Leds_State = LEDS_OFF;

    while (1)
    {
        xQueueReceive(xQueue1, &(Leds_State), (TickType_t) 0);
        switch (Leds_State)
        {
        case 'r':
            GPIOPinWrite(GPIO_PORTF_BASE, LEDS, RED);
            UART_sendString (UART0_BASE, "The RED LED should be ON. \n\r");
            break;
        case 'b':
            GPIOPinWrite(GPIO_PORTF_BASE, LEDS, BLUE);
            UART_sendString (UART0_BASE, "The BLUE LED should be ON. \n\r");
            break;
        case 'g':
            GPIOPinWrite(GPIO_PORTF_BASE, LEDS, GREEN);
            UART_sendString (UART0_BASE, "All GREEN LED should be ON. \n\r");
            break;
        default:
            GPIOPinWrite(GPIO_PORTF_BASE, LEDS, LEDS_OFF);
            UART_sendString (UART0_BASE, "All LEDS should be OFF. \n\r");
            break;
        }
        vTaskResume(First_Handle);
    }
}

/*****************************************************************************/
/*                               Main Function                               */
/*****************************************************************************/

int main (void)
{
    /* Initialize Clock with 80 Mhz. */
    SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);

    /* Initialize Switches and LEDS. */
    PORTF_Init ();

    /* Initialize UART with PC Communications. */
    UART0_Init ();

    /* Create a dormant timer, to be used to toggle LEDS at different intervals. */
    xTimer = xTimerCreate("Toggle Timer", pdMS_TO_TICKS(milli_seconds[global_counter]), pdTRUE, (void *) 0, vTimerCallback);

    /* Create Tasks that receives, filters, then, sets the specific needed LEDS. */
    xTaskCreate(vTask1, "Task_Recive_Filter", STACK_SIZE_TASK_1, NULL, 2, &First_Handle);
    xTaskCreate(vTask2, "Task_Toggle_LEDS"  , STACK_SIZE_TASK_2, NULL, 1, &Second_Handle);

    vTaskStartScheduler();

    while (1)
    { }

    return 0;
}
