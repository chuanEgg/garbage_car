/*
 * voice_activate.h
 *
 *  Created on: Apr 29, 2024
 *      Author: chuan
 */
#include <FreeRTOS.h>
#include "queue.h"
#include "data_queue.h"
#include "message_buffer.h"

#ifndef SOURCE_VOICE_ACTIVATE_H_
#define SOURCE_VOICE_ACTIVATE_H_

void voice_activate_task(void *args);

#endif /* SOURCE_VOICE_ACTIVATE_H_ */
