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

/********************** inclusions *******************************************/
/* Project includes */
#include "main.h"
#include "cmsis_os.h"

/* Demo includes */
#include "logger.h"
#include "dwt.h"

/* Application & Tasks includes */
#include "board.h"
#include "app.h"
#include "app_it.h"
#include "task_adc.h"
#include "task_adc_attribute.h"
#include "task_adc_interface.h"

/********************** macros and definitions *******************************/

/********************** internal data declaration ****************************/

adc_device_t adc_device;

/********************** internal functions declaration ***********************/

/********************** external data declaration ****************************/

/********************** external functions definition ************************/
void open_adc(ADC_HandleTypeDef *h_adc_device)
{
	// Se configura todo el struct adc_device
	adc_device.h_adc = h_adc_device;
	adc_device.adc_value = 0;

	// Inicializar spoolers
	adc_device.input_spooler.head = 0;
	adc_device.input_spooler.tail = 0;
	adc_device.output_spooler.head = 0;
	adc_device.output_spooler.tail = 0;

	// Cola por donde se van a enviar y consumir los valores leidos del ADC
	adc_device.h_queue = xQueueCreateStatic(ADC_QUEUE_LENGTH,
											ADC_QUEUE_ITEM_SIZE,
											adc_device.queue_storage,
											&adc_device.queue_cb);
	configASSERT(adc_device.h_queue != NULL);

	// Semaforo que coordina el acceso a la lectura del ADC
	adc_device.h_semaphore = xSemaphoreCreateBinaryStatic(&adc_device.semaphore_cb);
	configASSERT(adc_device.h_semaphore != NULL);

	// Tarea encargada de leer los valores recibidos del ADC
	adc_device.h_task = xTaskCreateStatic(task_adc,
										  "Task ADC",
										  ADC_TASK_STACK_SIZE,
										  NULL,
										  (tskIDLE_PRIORITY + 2ul),
										  adc_device.task_stack,
										  &adc_device.task_cb);
	configASSERT(adc_device.h_task != NULL);

	// Se inicia la conversion ADC con DMA
	HAL_ADC_Start_DMA(h_adc_device, &adc_device.adc_value, 1);

	LOGGER_INFO("  %s - ADC driver opened", GET_NAME(open_adc));
}

void release_adc(ADC_HandleTypeDef *h_adc_device)
{
	HAL_ADC_Stop_DMA(h_adc_device);

	// Se elimina la task
	if (adc_device.h_task != NULL)
	{
		vTaskDelete(adc_device.h_task);
		adc_device.h_task = NULL;
	}

	// Se elimina la queue
	if (adc_device.h_queue != NULL)
	{
		vQueueDelete(adc_device.h_queue);
		adc_device.h_queue = NULL;
	}

	// Se elimina el semaforo
	if (adc_device.h_semaphore != NULL)
	{
		vSemaphoreDelete(adc_device.h_semaphore);
		adc_device.h_semaphore = NULL;
	}

	adc_device.h_adc = NULL;
	LOGGER_INFO("  %s - ADC driver released", GET_NAME(release_adc));
}

void write_adc(ADC_HandleTypeDef *h_adc_device)
{
	// No usado, solo lectura
	UNUSED(h_adc_device);
}

BaseType_t read_adc(ADC_HandleTypeDef *h_adc_device, uint32_t *value)
{
	UNUSED(h_adc_device);
	// Se bloquea esperando un valor por la queue
	return xQueueReceive(adc_device.h_queue, value, portMAX_DELAY);
}

void ioctl_adc(ADC_HandleTypeDef *h_adc_device)
{
	UNUSED(h_adc_device);
	// Se reinicia la conversion DMA
	HAL_ADC_Stop_DMA(adc_device.h_adc);
	HAL_ADC_Start_DMA(adc_device.h_adc, &adc_device.adc_value, 1);
	LOGGER_INFO("  %s - ADC conversion restarted", GET_NAME(ioctl_adc));
}

/********************** end of file ******************************************/
