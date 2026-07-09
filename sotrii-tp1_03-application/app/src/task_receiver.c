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
#include "task_adc_attribute.h"
#include "task_adc_interface.h"

/********************** macros and definitions *******************************/
#define G_TASK_RECEIVER_CNT_INI	0ul

#define TASK_RECEIVER_DEL_ZERO		(pdMS_TO_TICKS(0ul))
#define TASK_RECEIVER_DEL_MAX		(pdMS_TO_TICKS(250ul))

/********************** internal data declaration ****************************/

/********************** internal functions declaration ***********************/

/********************** internal data definition *****************************/
const char *p_task_receiver_wait_250mS		= "   ==> Task RECEIVER - Wait:   250mS";

/********************** external data declaration ****************************/
uint32_t g_task_receiver_cnt;

/********************** external functions definition ************************/
/* Task thread */
void task_receiver(void *parameters)
{
	/*  Declare & Initialize Task Function variables */
	g_task_receiver_cnt = G_TASK_RECEIVER_CNT_INI;

	/* Print out: Task Initialized */
	LOGGER_INFO(" ");
	LOGGER_INFO("  %s is running - Tick [mS] = %lu", pcTaskGetName(NULL), xTaskGetTickCount());

	for (;;)
	{
		uint32_t adc_value;
		adc_spooler_t *output_spooler = &adc_device.output_spooler;

		// Leer de la queue y acumular en el output spooler
		if (read_adc(&hadc1, &adc_value) == pdTRUE)
		{
			uint32_t next_head = (output_spooler->head + 1) % ADC_SPOOLER_SIZE;
			if (next_head != output_spooler->tail)
			{
				output_spooler->buffer[output_spooler->head] = adc_value;
				output_spooler->head = next_head; // Solo esta task escribe head, no hace falta exclusion mutua
			}
		}

		// Si el output spooler esta lleno, proceamos en lote todo lo que hay
		uint32_t spooler_count = (output_spooler->head - output_spooler->tail + ADC_SPOOLER_SIZE) % ADC_SPOOLER_SIZE;
		if (spooler_count >= (ADC_SPOOLER_SIZE - 1))
		{
			// Spooler lleno
			LOGGER_INFO("   ==> Task RECEIVER - Batch of %lu values:", spooler_count);
			while (output_spooler->tail != output_spooler->head)
			{
				uint32_t val = output_spooler->buffer[output_spooler->tail];
				output_spooler->tail = (output_spooler->tail + 1) % ADC_SPOOLER_SIZE;
				g_task_receiver_cnt++;
				LOGGER_INFO("       ADC Value: %lu", val);
			}
		}
	}
}

/********************** end of file ******************************************/
