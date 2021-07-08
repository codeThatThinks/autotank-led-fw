//==============================================================================
// Autotank LED Controller
// Ian Glen <ian@ianglen.me>
//==============================================================================

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------

#include <stdint.h>
#include <stm32f3xx_hal.h>

#include "can.h"
#include "clock.h"
#include "config.h"
#include "debug.h"
#include "gpio.h"
#include "rgb_strip.h"
#include "uart.h"
#include "version.h"


//------------------------------------------------------------------------------
// Public Functions
//------------------------------------------------------------------------------

int main(void)
{
	HAL_Init();

	// init peripherals
	clock_init();
	gpio_init();
	uart_init();
	can_init();


#ifdef RGB_STRIP

	rgb_strip_init();
	rgb_strip_disable(0);
	rgb_strip_disable(1);

#endif // RGB_STRIP


	// startup blink
	for(int i = 0; i < 5; i++) {
		HAL_GPIO_WritePin(STATUS_PORT, STATUS_PIN, GPIO_PIN_RESET);
		HAL_Delay(STATUS_BLINK_TIME * 2);
		HAL_GPIO_WritePin(STATUS_PORT, STATUS_PIN, GPIO_PIN_SET);
		HAL_Delay(STATUS_BLINK_TIME * 2);
	}

	// print firmware string
	printf("%s\r\nFirmware Version %d.%d.%d\r\n\r\n", FW_NAME, VER_MAJOR, VER_MINOR, VER_PATCH);

	can_msg_t msg;

	while(1)
	{
		if(can_receive(&msg))
		{
			// Read Pins command
			if(msg.cmd == CAN_CMD_READ_PINS && msg.len == 1)
			{
				uint8_t payload[] = {gpio_read_inputs(), gpio_read_outputs()};
				can_send(msg.payload[0], CAN_CMD_READ_PINS, payload, sizeof(payload));
			}

			// Write Pins command
			else if(msg.cmd == CAN_CMD_WRITE_PINS && msg.len == 1)
			{
				gpio_write_outputs(msg.payload[0]);
			}

			// Write Pin command
			else if(msg.cmd == CAN_CMD_WRITE_PIN && msg.len == 2)
			{
				gpio_write_output(msg.payload[0], msg.payload[1]);
			}


#ifdef TRUTH_TABLE

			// Truth Table command
			else if(msg.cmd == CAN_CMD_TRUTH_TABLE && msg.len == 4)
			{
				gpio_set_truth_table(msg.payload[0], msg.payload[1], (msg.payload[2] << 8) | msg.payload[3]);
			}

#endif // TRUTH_TABLE


#ifdef PIN_INTERRUPT

			// Pin Interrupt command
			else if(msg.cmd == CAN_CMD_PIN_INTERRUPT && msg.len == 3)
			{
				gpio_set_pin_interrupt(msg.payload[0], msg.payload[1], msg.payload[2]);
			}

#endif // PIN_INTERRUPT


#ifdef RGB_STRIP

			// RGB Strip 1 command
			else if(msg.cmd == CAN_CMD_RGB_STRIP_1 && msg.len == 4)
			{
				if(msg.payload[0] == 0) rgb_strip_disable(0);
				else if(msg.payload[0] == 1) rgb_strip_set_color(0, msg.payload[1], msg.payload[2], msg.payload[3]);
				else if(msg.payload[0] == 2) rgb_strip_set_rainbow(0);
			}

			// RGB Strip 2 command
			else if(msg.cmd == CAN_CMD_RGB_STRIP_2 && msg.len == 4)
			{
				if(msg.payload[0] == 0) rgb_strip_disable(1);
				else if(msg.payload[0] == 1) rgb_strip_set_color(1, msg.payload[1], msg.payload[2], msg.payload[3]);
				else if(msg.payload[0] == 2) rgb_strip_set_rainbow(1);
			}

#endif // RGB_STRIP


		}

#ifdef RGB_STRIP

		rgb_strip_task();

#endif // RGB_STRIP

		HAL_Delay(1);
	}
}

//
