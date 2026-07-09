/*
 * Copyright (c) 2026 Juan Manuel Cruz <jcruz@fi.uba.ar> <jcruz@frba.utn.edu.ar>.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * @author : Juan Manuel Cruz <jcruz@fi.uba.ar> <jcruz@frba.utn.edu.ar>
 */

#ifndef TASK_ADC_ATTRIBUTE_H_
#define TASK_ADC_ATTRIBUTE_H_

/********************** CPP guard ********************************************/
#ifdef __cplusplus
extern "C" {
#endif

/********************** inclusions *******************************************/
#include "main.h"
#include "cmsis_os.h"

/********************** macros ***********************************************/
#define ADC_QUEUE_LENGTH		10u
#define ADC_QUEUE_ITEM_SIZE		sizeof(uint32_t)
#define ADC_TASK_STACK_SIZE		(2 * configMINIMAL_STACK_SIZE)
#define ADC_SPOOLER_SIZE		8u

/********************** typedef **********************************************/
// Spooler tipo buffer circular
typedef struct {
	uint32_t             buffer[ADC_SPOOLER_SIZE];
	volatile uint32_t    head;
	volatile uint32_t    tail;
} adc_spooler_t;

typedef struct {
	ADC_HandleTypeDef    *h_adc;
	QueueHandle_t        h_queue;
	SemaphoreHandle_t    h_semaphore;
	TaskHandle_t         h_task;
	uint32_t             adc_value;

	adc_spooler_t        input_spooler;
	adc_spooler_t        output_spooler;

	StaticQueue_t        queue_cb;
	uint8_t              queue_storage[ADC_QUEUE_LENGTH * ADC_QUEUE_ITEM_SIZE];

	StaticSemaphore_t    semaphore_cb;

	StaticTask_t         task_cb;
	StackType_t          task_stack[ADC_TASK_STACK_SIZE];
} adc_device_t;

/********************** external data declaration ****************************/
extern adc_device_t adc_device;

/********************** external functions declaration ***********************/

/********************** End of CPP guard *************************************/
#ifdef __cplusplus
}
#endif

#endif /* TASK_ADC_ATTRIBUTE_H_ */

/********************** end of file ******************************************/
