/* Header file includes. */
#include "cyhal.h"
#include "cybsp.h"
#include "cy_retarget_io.h"

/* RTOS header file. */
#if defined (COMPONENT_FREERTOS)
#include <FreeRTOS.h>
#include <task.h>
#endif

/* Task header file. */
#include "tcp_client.h"
#include "radar.h"
#include "voice_activate.h"

/*******************************************************************************
* Macros
********************************************************************************/
/* RTOS related macros. */
#if defined (COMPONENT_FREERTOS)
#define TCP_CLIENT_TASK_STACK_SIZE		(5 * 1024)
#define TCP_CLIENT_TASK_PRIORITY		(2)
#define RADAR_TASK_STACK_SIZE			(5 * 1024)
#define RADAR_TASK_PRIORITY				(3)
#define VOICE_ACTIVATE_TASK_STACK_SIZE	(5 * 1024)
#define VOICE_ACTIVATE_TASK_PRIORITY	(1)
#endif

/*******************************************************************************
* Global Variables
********************************************************************************/
/* This enables RTOS aware debugging. */
volatile int uxTopUsedPriority;

int main()
{
    cy_rslt_t result;

    /* This enables RTOS aware debugging in OpenOCD. */
    uxTopUsedPriority = configMAX_PRIORITIES - 1;

    /* Initialize the board support package. */
    result = cybsp_init() ;
    CY_ASSERT(result == CY_RSLT_SUCCESS);

    /* To avoid compiler warnings. */
    (void) result;

    /* Enable global interrupts. */
    __enable_irq();

    /* Initialize retarget-io to use the debug UART port. */
    cy_retarget_io_init(CYBSP_DEBUG_UART_TX, CYBSP_DEBUG_UART_RX, CY_RETARGET_IO_BAUDRATE);

    /* Initialize the User LED. */
    cyhal_gpio_init(CYBSP_USER_LED, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, CYBSP_LED_STATE_OFF);

    /* Initialize Radar output pin. */
//    cyhal_gpio_init(CYBSP_USER_LED2, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, CYBSP_LED_STATE_ON);

    /* \x1b[2J\x1b[;H - ANSI ESC sequence to clear screen */
    printf("\x1b[2J\x1b[;H");
    printf("============================================================\n");
    printf("Start Program\n");
    printf("============================================================\n\n");

    /* Create the tasks. */
    xTaskCreate(tcp_client_task, "Network task", TCP_CLIENT_TASK_STACK_SIZE, NULL, TCP_CLIENT_TASK_PRIORITY, NULL);
    xTaskCreate(voice_activate_task, "Voice task", VOICE_ACTIVATE_TASK_STACK_SIZE, NULL, VOICE_ACTIVATE_TASK_PRIORITY, NULL);

    /* Start the FreeRTOS scheduler. */
    vTaskStartScheduler();

    /* Should never get here. */
    CY_ASSERT(0);

}



 /* [] END OF FILE */
