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
#include "task_adc_attribute.h"

/********************** macros and definitions *******************************/
#define G_TASK_ADC_RX_CNT_INI			0ul
#define G_TASK_ADC_RX_RUNTIME_US_INI	0ul

/********************** internal data declaration ****************************/

/********************** internal functions declaration ***********************/

/********************** internal data definition *****************************/

/********************** external data declaration ****************************/
uint32_t g_task_adc_cnt;
uint32_t g_task_adc_runtime_us;

/********************** external functions definition ************************/
/**
 * Task que funciona como Gatekeeper del recurso adc_device.adc_value
 */
void task_adc(void *parameters)
{
	UNUSED(parameters);

	g_task_adc_cnt = G_TASK_ADC_RX_CNT_INI;
	g_task_adc_runtime_us = G_TASK_ADC_RX_RUNTIME_US_INI;

	LOGGER_INFO(" ");
	LOGGER_INFO("  %s is running - Tick [mS] = %lu", pcTaskGetName(NULL), xTaskGetTickCount());

	for (;;)
	{
		// Se bloquea esperando al semaforo que sera dado desde la interrupcion cuando el DMA termina la conversion del ADC
		if (xSemaphoreTake(adc_device.h_semaphore, portMAX_DELAY) == pdTRUE)
		{
			g_task_adc_cnt++;
			cycle_counter_reset();

			// Vaciar el input spooler: puede haber varios valores acumulados. Lee desde tail
			adc_spooler_t *input_spooler = &adc_device.input_spooler;
			while (input_spooler->tail != input_spooler->head)
			{
				uint32_t value = input_spooler->buffer[input_spooler->tail];
				input_spooler->tail = (input_spooler->tail + 1) % ADC_SPOOLER_SIZE; // Tail solo se escribe aca, no hace falta exclusion mutua

				xQueueSend(adc_device.h_queue, &value, portMAX_DELAY);
				LOGGER_INFO("   ==> Task ADC - Value: %lu", value);
			}

			// Se relanza la conversion DMA
			HAL_ADC_Start_DMA(adc_device.h_adc, &adc_device.adc_value, 1);
			g_task_adc_runtime_us = cycle_counter_get_time_us();
		}
	}
}

/********************** end of file ******************************************/
