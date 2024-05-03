/*
 * data_queue.h
 *
 *  Created on: May 3, 2024
 *      Author: chuan
 */

#include <FreeRTOS.h>
#include "queue.h"
#include "message_buffer.h"

#ifndef DATA_QUEUE_H_
#define DATA_QUEUE_H_

extern QueueHandle_t rcv_data_q;
extern QueueHandle_t send_data_q;
extern MessageBufferHandle_t msg_buffer;

#endif /* DATA_QUEUE_H_ */
