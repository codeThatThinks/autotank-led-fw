//==============================================================================
// Clock Configuration
// Ian Glen <ian@ianglen.me>
//==============================================================================

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------

#include <stm32f3xx_hal.h>

#include "clock.h"
#include "debug.h"


//------------------------------------------------------------------------------
// Public Functions
//------------------------------------------------------------------------------

// Initialize system clocks
void clock_init(void)
{
	/*
		HSI w/ PLL (no external crystal)

		SYSCLK is 72 MHZ
		APB1 peripheral clocks are 36 MHz
		APB2 peripheral clocks are 72 MHz
		APB1/APB2 Timer clocks are 72 MHz
	*/

	__HAL_RCC_SYSCFG_CLK_ENABLE();
  	__HAL_RCC_PWR_CLK_ENABLE();

	// use internal HSI oscillator and PLL
	RCC_OscInitTypeDef oscillator_config = {0};
	oscillator_config.OscillatorType = RCC_OSCILLATORTYPE_HSI;
	oscillator_config.HSIState = RCC_HSI_ON;
	oscillator_config.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
	oscillator_config.PLL.PLLState = RCC_PLL_ON;
	oscillator_config.PLL.PLLSource = RCC_PLLSOURCE_HSI;
	oscillator_config.PLL.PLLMUL = RCC_PLL_MUL9;
	oscillator_config.PLL.PREDIV = RCC_PREDIV_DIV1;
	debug_assert(HAL_RCC_OscConfig(&oscillator_config) == HAL_OK, "Failed to configure oscillator");

	// config system and peripheral clocks
	RCC_ClkInitTypeDef clock_config = {0};
	clock_config.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	clock_config.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	clock_config.AHBCLKDivider = RCC_SYSCLK_DIV1;
	clock_config.APB1CLKDivider = RCC_HCLK_DIV2;
	clock_config.APB2CLKDivider = RCC_HCLK_DIV1;
	debug_assert(HAL_RCC_ClockConfig(&clock_config, FLASH_LATENCY_2) == HAL_OK, "Failed to configure system clocks");

	RCC_PeriphCLKInitTypeDef peripheral_config = {0};
	peripheral_config.PeriphClockSelection = RCC_PERIPHCLK_UART4;
	peripheral_config.Uart4ClockSelection = RCC_UART4CLKSOURCE_PCLK1;
	debug_assert(HAL_RCCEx_PeriphCLKConfig(&peripheral_config) == HAL_OK, "Failed to config peripheral clocks");

	// enable peripheral clocks
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOD_CLK_ENABLE();
	__HAL_RCC_GPIOF_CLK_ENABLE();
	__HAL_RCC_DMA1_CLK_ENABLE();
	__HAL_RCC_TIM16_CLK_ENABLE();
	__HAL_RCC_TIM17_CLK_ENABLE();
	__HAL_RCC_UART4_CLK_ENABLE();
	__HAL_RCC_CAN1_CLK_ENABLE();
}


//------------------------------------------------------------------------------
// ISRs
//------------------------------------------------------------------------------

// ISR for system ticks -- used by HAL_Delay()
void SysTick_Handler(void)
{
	HAL_IncTick();
}
