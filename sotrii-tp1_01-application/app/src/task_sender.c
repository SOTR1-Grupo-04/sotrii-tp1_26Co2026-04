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
#include "task_i2c_interface.h"

/********************** macros and definitions *******************************/
#define G_TASK_SENDER_CNT_INI 0ul

#define TASK_SENDER_DEL_MAX (pdMS_TO_TICKS(250ul))

#define LCD_I2C_ADDRESS 0x27u

#define LCD_BACKLIGHT_ON 0x08u
#define LCD_ENABLE_ON 0x04u
#define LCD_ENABLE_OFF 0x00u

#define LCD_INIT_NIBBLE_8BIT 0x30u
#define LCD_INIT_NIBBLE_4BIT 0x20u

#define LCD_CMD_FUNCTION_SET 0x28u
#define LCD_CMD_DISPLAY_ON 0x0Cu
#define LCD_CMD_CLEAR_DISPLAY 0x01u
#define LCD_CMD_ENTRY_MODE_SET 0x06u
#define LCD_CMD_SET_DDRAM_00 0x80u

#define LCD_TEST_MESSAGE "hola mundo"

#define LCD_RS_CMD 0x00u
#define LCD_RS_DATA 0x01u

#define LCD_UPPER_NIBBLE(byte) ((uint8_t)((byte) & 0xF0u))
#define LCD_LOWER_NIBBLE(byte) ((uint8_t)(((byte) << 4) & 0xF0u))

/********************** internal data declaration ****************************/

/********************** internal functions declaration ***********************/
static void lcd_write_nibble(uint16_t dev_address, uint8_t nibble, uint8_t rs);
static void lcd_write_byte(uint16_t dev_address, uint8_t data, uint8_t rs);
static void lcd_init_sequence(uint16_t dev_address);

/********************** internal data definition *****************************/
const char *p_task_sender_wait_250mS = "   ==> Task SENDER - Wait:   250mS";

/********************** external data declaration ****************************/
uint32_t g_task_sender_cnt;

/********************** internal functions definition ************************/
static void lcd_write_nibble(uint16_t dev_address, uint8_t nibble, uint8_t rs)
{
	uint8_t data = (uint8_t)(nibble | LCD_BACKLIGHT_ON | rs);

	write_i2c(&hi2c1, dev_address, (uint8_t)(data | LCD_ENABLE_OFF));
	write_i2c(&hi2c1, dev_address, (uint8_t)(data | LCD_ENABLE_ON));
	write_i2c(&hi2c1, dev_address, (uint8_t)(data | LCD_ENABLE_OFF));
}

static void lcd_write_byte(uint16_t dev_address, uint8_t data, uint8_t rs)
{
	lcd_write_nibble(dev_address, LCD_UPPER_NIBBLE(data), rs);
	lcd_write_nibble(dev_address, LCD_LOWER_NIBBLE(data), rs);
}

static void lcd_init_sequence(uint16_t dev_address)
{
	vTaskDelay(pdMS_TO_TICKS(50ul));

	lcd_write_nibble(dev_address, LCD_INIT_NIBBLE_8BIT, LCD_RS_CMD);
	vTaskDelay(pdMS_TO_TICKS(5ul));

	lcd_write_nibble(dev_address, LCD_INIT_NIBBLE_8BIT, LCD_RS_CMD);
	vTaskDelay(pdMS_TO_TICKS(5ul));

	lcd_write_nibble(dev_address, LCD_INIT_NIBBLE_8BIT, LCD_RS_CMD);
	vTaskDelay(pdMS_TO_TICKS(1ul));

	lcd_write_nibble(dev_address, LCD_INIT_NIBBLE_4BIT, LCD_RS_CMD);
	vTaskDelay(pdMS_TO_TICKS(1ul));

	lcd_write_byte(dev_address, LCD_CMD_FUNCTION_SET, LCD_RS_CMD);
	lcd_write_byte(dev_address, LCD_CMD_DISPLAY_ON, LCD_RS_CMD);
	lcd_write_byte(dev_address, LCD_CMD_CLEAR_DISPLAY, LCD_RS_CMD);
	vTaskDelay(pdMS_TO_TICKS(2ul));
	lcd_write_byte(dev_address, LCD_CMD_ENTRY_MODE_SET, LCD_RS_CMD);
}

/********************** external functions definition ************************/
/* Task thread */
void task_sender(void *parameters)
{
	/* Prevent unused argument(s) compilation warning */
	UNUSED(parameters);

	/*  Declare & Initialize Task Function variables */
	g_task_sender_cnt = G_TASK_SENDER_CNT_INI;

	/* Serial LCD I2C Module–PCF8574
	 * https://alselectro.wordpress.com/2016/05/12/serial-lcd-i2c-module-pcf8574/
	 * https://www.ti.com/product/PCF8574
	 * dev_address = (address base | jumper less address)
	 */
	uint16_t dev_address = LCD_I2C_ADDRESS;
	const char *message = LCD_TEST_MESSAGE;
	uint32_t message_index = 0ul;

	/* Print out: Task Initialized */
	LOGGER_INFO(" ");
	LOGGER_INFO("  %s is running - Tick [mS] = %lu", pcTaskGetName(NULL), xTaskGetTickCount());

	/* As per most tasks, this task is implemented in an infinite loop. */
	for (;;)
	{
		/* Update Task Counter */
		g_task_sender_cnt++;

		if (1ul == g_task_sender_cnt)
		{
			lcd_init_sequence(dev_address);
			lcd_write_byte(dev_address, LCD_CMD_SET_DDRAM_00, LCD_RS_CMD);
		}

		if ('\0' == message[message_index])
		{
			message_index = 0ul;
			lcd_write_byte(dev_address, LCD_CMD_CLEAR_DISPLAY, LCD_RS_CMD);
			vTaskDelay(pdMS_TO_TICKS(2ul));
			lcd_write_byte(dev_address, LCD_CMD_SET_DDRAM_00, LCD_RS_CMD);
		}

		lcd_write_byte(dev_address, (uint8_t)message[message_index], LCD_RS_DATA);
		message_index++;

		/* Print out: Wait 250mS */
		LOGGER_INFO(p_task_sender_wait_250mS);
		vTaskDelay(TASK_SENDER_DEL_MAX);
	}
}

/********************** end of file ******************************************/
