//==============================================================================
// Debugging Functions
// Ian Glen <ian@ianglen.me>
//==============================================================================

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------

#include <stm32f3xx_hal.h>

#include "can.h"
#include "config.h"
#include "debug.h"
#include "gpio.h"


//------------------------------------------------------------------------------
// Public Functions
//------------------------------------------------------------------------------

// Enter an infinite loop and flash status led
void error_state(void)
{
	// deinitialize peripherals
	can_deinit();

	while(1)
	{
		HAL_GPIO_WritePin(STATUS_PORT, STATUS_PIN, GPIO_PIN_SET);
		HAL_Delay(STATUS_BLINK_TIME * 4);
		HAL_GPIO_WritePin(STATUS_PORT, STATUS_PIN, GPIO_PIN_RESET);
		HAL_Delay(STATUS_BLINK_TIME * 4);
	}
}
