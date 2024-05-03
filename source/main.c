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
#include "data_queue.h"

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
#define RECORD_TASK_PRIORITY			(1)
#define RECORD_TASK_STACK_SIZE			(65534)
#endif

#define MAX_TCP_DATA_PACKET_LENGTH      (65535u)
#define QUEUE_SIZE						(1u)
#define BUFFER_SIZE						(128u)
#define MESSAGE_BUFFER_SIZE				(65535u)
/*******************************************************************************
* Global Variables
********************************************************************************/
/* This enables RTOS aware debugging. */
volatile int uxTopUsedPriority;
MessageBufferHandle_t msg_buffer;

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

    printf("\x1b[2J\x1b[;H");
    printf("============================================================\n");
    printf("Start Program\n");
    printf("============================================================\n\n");

    /* Create queue for inter-task communication */
    send_data_q = xQueueCreate(QUEUE_SIZE, MAX_TCP_DATA_PACKET_LENGTH);
//    msg_buffer = xMessageBufferCreate(MESSAGE_BUFFER_SIZE);
    /* Create the tasks. */
    xTaskCreate(tcp_client_task, "Network task", TCP_CLIENT_TASK_STACK_SIZE, NULL, TCP_CLIENT_TASK_PRIORITY, NULL);
    xTaskCreate(voice_activate_task, "Voice task", VOICE_ACTIVATE_TASK_STACK_SIZE, NULL, VOICE_ACTIVATE_TASK_PRIORITY, NULL);
//    xTaskCreate(record_task, "Record task", RECORD_TASK_STACK_SIZE, NULL, RECORD_TASK_PRIORITY, NULL);

    /* Start the FreeRTOS scheduler. */
    vTaskStartScheduler();

    /* Should never get here. */
    CY_ASSERT(0);

}



 /* [] END OF FILE */
