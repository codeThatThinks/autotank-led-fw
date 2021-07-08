//==============================================================================
// UART Driver
// Ian Glen <ian@ianglen.me>
//==============================================================================

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------

#include <stdbool.h>
#include <errno.h>
#include <stdint.h>
#include <sys/unistd.h>
#include <stm32f3xx_hal.h>

#include "debug.h"
#include "uart.h"


//------------------------------------------------------------------------------
// Definitions
//------------------------------------------------------------------------------

/*
	PC10: UART_TXO
	PC11: UART_RXI
*/

#define UART_PORT	GPIOC
#define UART_PINS	GPIO_PIN_10 | GPIO_PIN_11;


//------------------------------------------------------------------------------
// Private Variables
//------------------------------------------------------------------------------

static UART_HandleTypeDef huart;


//------------------------------------------------------------------------------
// Public Functions
//------------------------------------------------------------------------------

void uart_init(void)
{
	// configure gpio pins
	GPIO_InitTypeDef gpio_config = {0};
	gpio_config.Pin = UART_PINS;
	gpio_config.Mode = GPIO_MODE_AF_PP;
	gpio_config.Pull = GPIO_NOPULL;
	gpio_config.Speed = GPIO_SPEED_FREQ_HIGH;
	gpio_config.Alternate = GPIO_AF5_UART4;
	HAL_GPIO_Init(UART_PORT, &gpio_config);

	// configure peripheral
	huart.Instance = UART4;
	huart.Init.BaudRate = 115200;
	huart.Init.WordLength = UART_WORDLENGTH_8B;
	huart.Init.StopBits = UART_STOPBITS_1;
	huart.Init.Parity = UART_PARITY_NONE;
	huart.Init.Mode = UART_MODE_TX_RX;
	huart.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart.Init.OverSampling = UART_OVERSAMPLING_16;
	huart.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
	huart.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
	debug_assert(HAL_UART_Init(&huart) == HAL_OK, "Failed to configure UART");
}

// Redirect printf() to UART
int _write(int file, char *data, int len)
{
   if(file != STDOUT_FILENO && file != STDERR_FILENO)
   {
      errno = EBADF;
      return -1;
   }

   return (HAL_UART_Transmit(&huart, (uint8_t *)data, len, 1000) == HAL_OK) ? len : 0;
}
