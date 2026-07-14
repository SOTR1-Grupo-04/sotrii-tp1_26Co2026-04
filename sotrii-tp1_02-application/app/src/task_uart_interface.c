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
#include <string.h>

/* Demo includes */
#include "logger.h"
#include "dwt.h"

/* Application & Tasks includes */
#include "board.h"
#include "app.h"
#include "app_it.h"
#include "task_uart.h"
#include "task_uart_attribute.h"
#include "task_uart_interface.h"

/********************** macros and definitions *******************************/

/********************** internal data declaration ****************************/

/********************** internal data declaration ****************************/

/********************** internal functions declaration ***********************/
void tx_uart_gatekeeper(void* uart_device_t);
void rx_uart_gatekeeper(void* uart_device_t);

/********************** internal data definition *****************************/

/********************** external data declaration ****************************/

/********************** external functions definition ************************/
/* Interface functions */
void open_uart(uart_device_t *h_uart_device)
{
	h_uart_device->tx_queue = NULL;
	h_uart_device->rx_queue = NULL;
	h_uart_device->tx_sendEnded = NULL;
	h_uart_device->rx_newData = NULL;
	h_uart_device->rx_size = 0u;

	/* Check huart is not NULL. */
	configASSERT(NULL != h_uart_device->huart);

	BaseType_t ret;

    /* Task Sender thread at priority 1 */
	    ret = xTaskCreate(tx_uart_gatekeeper,						/* Pointer to the function thats implement the task. */
					  "UART Tx gatekeeper",						/* Text name for the task. This is to facilitate debugging only. */
					  (2 * configMINIMAL_STACK_SIZE),			/* Stack depth in words. */
					  (void*) h_uart_device,				/* We are not using the task parameter. */
					  (tskIDLE_PRIORITY + 1ul),					/* This task will run at priority 1. */
					  &h_uart_device->tx_gatekeeper);			/* We are using a variable as task handle. */

	/* Check the thread was created successfully. */
    configASSERT(pdPASS == ret);

	/* Task Sender thread at priority 1 */
	    ret = xTaskCreate(rx_uart_gatekeeper,						/* Pointer to the function thats implement the task. */
					  "UART Tx gatekeeper",						/* Text name for the task. This is to facilitate debugging only. */
					  (2 * configMINIMAL_STACK_SIZE),			/* Stack depth in words. */
					  (void*) h_uart_device,				/* We are not using the task parameter. */
					  (tskIDLE_PRIORITY + 1ul),					/* This task will run at priority 1. */
					  &h_uart_device->rx_gatekeeper);			/* We are using a variable as task handle. */


    /* Check the thread was created successfully. */
    configASSERT(pdPASS == ret);

	h_uart_device->tx_queue = xQueueCreate(10, sizeof(dynamic_data_spooler));
	/* Check the queue was created successfully. */
    configASSERT(NULL != h_uart_device->tx_queue);

	h_uart_device->rx_queue = xQueueCreate(10, sizeof(dynamic_data_spooler));
	/* Check the queue was created successfully. */
    configASSERT(NULL != h_uart_device->rx_queue);

	h_uart_device->tx_sendEnded = xSemaphoreCreateBinary();
	/* Check the semaphore was created successfully. */
	configASSERT(NULL != h_uart_device->tx_sendEnded);

	h_uart_device->rx_newData = xSemaphoreCreateBinary();
	/* Check the semaphore was created successfully. */
	configASSERT(NULL != h_uart_device->rx_newData);
}

void release_uart(uart_device_t *h_uart_device)
{
	if (h_uart_device == NULL) {
		return;
	}

	if (h_uart_device->tx_gatekeeper != NULL) {
		vTaskDelete(h_uart_device->tx_gatekeeper);
	}

	if (h_uart_device->rx_gatekeeper != NULL) {
		vTaskDelete(h_uart_device->rx_gatekeeper);
	}

	if (h_uart_device->tx_queue != NULL) {
		vQueueDelete(h_uart_device->tx_queue);
	}

	if (h_uart_device->rx_queue != NULL) {
		vQueueDelete(h_uart_device->rx_queue);
	}

	if (h_uart_device->tx_sendEnded != NULL) {
		vSemaphoreDelete(h_uart_device->tx_sendEnded);
	}

	if (h_uart_device->rx_newData != NULL) {
		vSemaphoreDelete(h_uart_device->rx_newData);
	}

	memset(h_uart_device, 0, sizeof(*h_uart_device));
}

void write_uart(uart_device_t *h_uart_device, uint8_t* buff, size_t buffSize)
{
	if (h_uart_device == NULL || buff == NULL || buffSize == 0) {
		return;
	}

	dynamic_data_spooler message;
	message.buffer = pvPortMalloc(buffSize);

    if (message.buffer == NULL)
    {
        return;
    }

    memcpy(message.buffer, buff, buffSize);
    message.size = buffSize;

    if (xQueueSend(h_uart_device->tx_queue,
                   &message,
                   1000) != pdPASS)
    {
        vPortFree(message.buffer);
    }
	
}

bool read_uart(uart_device_t *h_uart_device, dynamic_data_spooler *message)
{
	if (h_uart_device == NULL || message == NULL) {
		return false;
	}

	if (xQueueReceive(h_uart_device->rx_queue, message, 0) != pdPASS) {
		return false;
	}
	return true;
}

void tx_uart_gatekeeper(void* uart_device)
{
	uart_device_t *dev = uart_device;
	if (dev->huart == NULL) {
		return;
	}
	
	while (1) {
		dynamic_data_spooler message;

		if (xQueueReceive(dev->tx_queue, &message, portMAX_DELAY) == pdPASS) {
			if (HAL_UART_Transmit_IT(dev->huart, message.buffer, message.size) == HAL_OK) {
				xSemaphoreTake(dev->tx_sendEnded, portMAX_DELAY);
			}
			vPortFree(message.buffer);
		}
	}
}

void rx_uart_gatekeeper(void* uart_device)
{
	uart_device_t *dev = uart_device;
	if (dev->huart == NULL) {
		return;
	}

	dynamic_data_spooler message;

	while (1)
	{
		configASSERT(HAL_OK == HAL_UARTEx_ReceiveToIdle_IT(dev->huart,
													   		dev->rx_it_buffer,
													   		UART_RX_IT_BUFFER_SIZE));

		xSemaphoreTake(dev->rx_newData, portMAX_DELAY);

		message.size = dev->rx_size;
		message.buffer = pvPortMalloc(message.size);

		configASSERT(message.buffer != NULL);

		memcpy(message.buffer, dev->rx_it_buffer, message.size);

		if (xQueueSend(dev->rx_queue, &message, pdMS_TO_TICKS(1000u)) != pdPASS) {
			vPortFree(message.buffer);
		}

		dev->rx_size = 0u;		
	}	
}

/********************** end of file ******************************************/
