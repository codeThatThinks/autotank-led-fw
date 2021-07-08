//==============================================================================
// Addressable RGB LED Strip Driver
// Ian Glen <ian@ianglen.me>
//==============================================================================

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stm32f3xx_hal.h>

#include "config.h"
#include "debug.h"
#include "gpio.h"
#include "rgb_strip.h"


//------------------------------------------------------------------------------
// Definitions
//------------------------------------------------------------------------------

/*
	PA6: RGB1
	PA7: RGB2
*/

#define RGB_PORT			GPIOA
#define RGB1_PIN			GPIO_PIN_6
#define RGB2_PIN			GPIO_PIN_7

#define DISABLED_INTERVAL	1000UL	// ms
#define COLOR_INTERVAL		1000UL	// ms
#define RAINBOW_INTERVAL	100UL	// ms

typedef enum
{
	STATE_INIT = 0,
	STATE_START_RESET,
	STATE_DATA,
	STATE_END_RESET
} rgb_strip_state_t;

typedef enum
{
	BUF_FIRST_HALF = 0,
	BUF_SECOND_HALF = 1
} dma_buffer_half_t;

typedef struct
{
	rgb_strip_mode_t mode;
	uint8_t red;
	uint8_t green;
	uint8_t blue;
	uint8_t wheel;
	uint32_t last_update;
} rgb_strip_t;


//------------------------------------------------------------------------------
// Private Function Definitions
//------------------------------------------------------------------------------

static void timer_init(uint8_t strip, TIM_TypeDef *timer);
static void dma_init(uint8_t strip, DMA_Channel_TypeDef *channel);
static void set_rgb(uint8_t strip, uint8_t r, uint8_t g, uint8_t b);
static void update(uint8_t strip);
static void load_next_led(uint8_t strip, uint8_t index, dma_buffer_half_t half);
static void dma_process_halfcomplete(uint8_t strip);
static void dma_process_complete(uint8_t strip);


//------------------------------------------------------------------------------
// Private Variables
//------------------------------------------------------------------------------

static DMA_HandleTypeDef hdmas[RGB_NUM_STRIPS];
static TIM_HandleTypeDef htims[RGB_NUM_STRIPS];

static uint8_t buffer[RGB_NUM_STRIPS][RGB_NUM_LEDS * BYTES_PER_LED];
//static uint16_t dma_buffer[RGB_NUM_STRIPS][RESET_PULSE / PERIOD];
static uint16_t dma_buffer[RGB_NUM_STRIPS][2 * 8 * BYTES_PER_LED];
static rgb_strip_state_t state[RGB_NUM_STRIPS];
static uint8_t led_index[RGB_NUM_STRIPS];

static uint16_t timer_arr_period;
static uint16_t timer_ccr_one;
static uint16_t timer_ccr_zero;
static uint16_t timer_ccr_reset;

static rgb_strip_t strips[RGB_NUM_STRIPS];


//------------------------------------------------------------------------------
// Public Functions
//------------------------------------------------------------------------------

// Initialize timers and DMA for RGB strips
void rgb_strip_init(void)
{
	// calculate timer values based on selected config options
	timer_arr_period = HAL_RCC_GetPCLK2Freq() / 1000000 * PERIOD / 1000;
	timer_ccr_one = HAL_RCC_GetPCLK2Freq() / 1000000 * ONE_PULSE / 1000;
	timer_ccr_zero = HAL_RCC_GetPCLK2Freq() / 1000000 * ZERO_PULSE / 1000;
	timer_ccr_reset = HAL_RCC_GetPCLK2Freq() / 1000000 * RESET_PULSE / 1000;

	// configure gpio pins
	GPIO_InitTypeDef gpio_config = {0};
	gpio_config.Pin = RGB1_PIN;
	gpio_config.Mode = GPIO_MODE_AF_PP;
	gpio_config.Pull = GPIO_NOPULL;
	gpio_config.Speed = GPIO_SPEED_FREQ_HIGH;
	gpio_config.Alternate = GPIO_AF1_TIM16;
	HAL_GPIO_Init(RGB_PORT, &gpio_config);
	gpio_config.Pin = RGB2_PIN;
	gpio_config.Alternate = GPIO_AF1_TIM17;
	HAL_GPIO_Init(RGB_PORT, &gpio_config);

	// configure timers
	timer_init(0, TIM16);
	timer_init(1, TIM17);

	// configure DMA
	dma_init(0, DMA1_Channel3);
	dma_init(1, DMA1_Channel1);

	// configure interrupts
	HAL_NVIC_SetPriority(DMA1_Channel3_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(DMA1_Channel3_IRQn);
	HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);
}

// Deinitialize timers and DMA for RGB strips
void rgb_strip_deinit(void)
{
	// stop any DMA transfers
	for(size_t i = 0; i < RGB_NUM_STRIPS; i++) HAL_TIM_PWM_Stop_DMA(&htims[i], TIM_CHANNEL_1);

	// disable DMA
	for(size_t i = 0; i < RGB_NUM_STRIPS; i++) HAL_DMA_DeInit(&hdmas[i]);

	// disable interrupts
	HAL_NVIC_DisableIRQ(DMA1_Channel3_IRQn);
	HAL_NVIC_DisableIRQ(DMA1_Channel1_IRQn);
}

// Disables strip
void rgb_strip_disable(uint8_t strip)
{
	if(strip >= RGB_NUM_STRIPS) return;

	strips[strip].mode = RGB_STRIP_DISABLED;
	set_rgb(strip, 0, 0, 0);
}

// Set strip to an RGB color value
void rgb_strip_set_color(uint8_t strip, uint8_t r, uint8_t g, uint8_t b)
{
	if(strip >= RGB_NUM_STRIPS) return;

	strips[strip].mode = RGB_STRIP_COLOR;
	strips[strip].red = r;
	strips[strip].green = g;
	strips[strip].blue = b;
	set_rgb(strip, r, g, b);
}

// Set strip to rainbow color mode
void rgb_strip_set_rainbow(uint8_t strip)
{
	if(strip >= RGB_NUM_STRIPS) return;

	strips[strip].mode = RGB_STRIP_RAINBOW;
	strips[strip].wheel = 0;
}

// Update strip color dependineg on moed
void rgb_strip_task(void)
{
	for(size_t i = 0; i < RGB_NUM_STRIPS; i++)
	{
		if(strips[i].mode == RGB_STRIP_DISABLED
			&& HAL_GetTick() >= strips[i].last_update + DISABLED_INTERVAL)
		{
			set_rgb(i, 0, 0, 0);
			strips[i].last_update = HAL_GetTick();
		}
		else if(strips[i].mode == RGB_STRIP_COLOR
			&& HAL_GetTick() >= strips[i].last_update + COLOR_INTERVAL)
		{
			set_rgb(i, strips[i].red, strips[i].green, strips[i].blue);
			strips[i].last_update = HAL_GetTick();
		}
		else if(strips[i].mode == RGB_STRIP_RAINBOW
			&& HAL_GetTick() >= strips[i].last_update + RAINBOW_INTERVAL)
		{
			uint8_t wheel = 255 - strips[i].wheel;

			if(wheel < 85)
			{
				set_rgb(i, 255 - wheel * 3, 0, wheel * 3);
			}
			else if(wheel < 170)
			{
				wheel -= 85;
				set_rgb(i, 0, wheel * 3, 255 - wheel * 3);
			}
			else
			{
				wheel -= 170;
				set_rgb(i, wheel * 3, 255 - wheel * 3, 0);
			}

			strips[i].wheel++;
			strips[i].last_update = HAL_GetTick();
		}
	}
}

//------------------------------------------------------------------------------
// Private Functions
//------------------------------------------------------------------------------

// Initialize an RGB strip timer
static void timer_init(uint8_t strip, TIM_TypeDef *timer)
{
	htims[strip].Instance = timer;
	htims[strip].Init.Prescaler = 0;
	htims[strip].Init.CounterMode = TIM_COUNTERMODE_UP;
	htims[strip].Init.Period = timer_arr_period;
	htims[strip].Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	htims[strip].Init.RepetitionCounter = 0;
	htims[strip].Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
	debug_assert(HAL_TIM_Base_Init(&htims[strip]) == HAL_OK, "Failed to configure RGB%d timer base", strip + 1);
	debug_assert(HAL_TIM_PWM_Init(&htims[strip]) == HAL_OK, "Failed to configure RGB%d timer pwm", strip + 1);

	TIM_OC_InitTypeDef oc_config = {0};
	oc_config.OCMode = TIM_OCMODE_PWM1;
	oc_config.Pulse = 0;
	oc_config.OCPolarity = TIM_OCPOLARITY_HIGH;
	oc_config.OCNPolarity = TIM_OCNPOLARITY_HIGH;
	oc_config.OCFastMode = TIM_OCFAST_DISABLE;
	oc_config.OCIdleState = TIM_OCIDLESTATE_RESET;
	oc_config.OCNIdleState = TIM_OCNIDLESTATE_RESET;

	TIM_BreakDeadTimeConfigTypeDef btd_config = {0};
	btd_config.OffStateRunMode = TIM_OSSR_DISABLE;
	btd_config.OffStateIDLEMode = TIM_OSSI_DISABLE;
	btd_config.LockLevel = TIM_LOCKLEVEL_OFF;
	btd_config.DeadTime = 0;
	btd_config.BreakState = TIM_BREAK_DISABLE;
	btd_config.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
	btd_config.BreakFilter = 0;
	btd_config.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;

	debug_assert(HAL_TIM_PWM_ConfigChannel(&htims[strip], &oc_config, TIM_CHANNEL_1) == HAL_OK
			  || HAL_TIMEx_ConfigBreakDeadTime(&htims[strip], &btd_config) == HAL_OK, "Failed to configure RGB%d timer output", strip + 1);
}

// Initialize RGB strip DMA
static void dma_init(uint8_t strip, DMA_Channel_TypeDef *channel)
{
	hdmas[strip].Instance = channel;
	hdmas[strip].Init.Direction = DMA_MEMORY_TO_PERIPH;
	hdmas[strip].Init.PeriphInc = DMA_PINC_DISABLE;
	hdmas[strip].Init.MemInc = DMA_MINC_ENABLE;
	hdmas[strip].Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
	hdmas[strip].Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
	hdmas[strip].Init.Mode = DMA_CIRCULAR;
	hdmas[strip].Init.Priority = DMA_PRIORITY_LOW;
	debug_assert(HAL_DMA_Init(&hdmas[strip]) == HAL_OK, "Failed to configure RGB%d DMA", strip + 1);

	__HAL_LINKDMA(&htims[strip], hdma[TIM_DMA_ID_CC1], hdmas[strip]);
	__HAL_LINKDMA(&htims[strip], hdma[TIM_DMA_ID_UPDATE], hdmas[strip]);
}

// Set strip buffer to an RGB color value
static void set_rgb(uint8_t strip, uint8_t r, uint8_t g, uint8_t b)
{
	if(strip >= RGB_NUM_STRIPS) return;

	for(size_t i = 0; i < RGB_NUM_LEDS * BYTES_PER_LED; i += BYTES_PER_LED)
	{
#ifdef FORMAT_GRB
		buffer[strip][i + 0] = g;
		buffer[strip][i + 1] = r;
		buffer[strip][i + 2] = b;
#endif
	}

	update(strip);
}

// Send color data to the RGB strips
static void update(uint8_t strip)
{
	if(strip >= RGB_NUM_STRIPS) return;

	// wait for the current update to complete
	while(state[strip] != STATE_INIT);

	// set dma buffer to reset pulse (timer cc reg = 0)
	memset(dma_buffer[strip], 0, sizeof(dma_buffer[0]));

	// move to start reset state and start DMA transfer
	state[strip] = STATE_START_RESET;
	HAL_TIM_PWM_Start_DMA(&htims[strip], TIM_CHANNEL_1, (uint32_t *)dma_buffer[strip], 2 * 8 * BYTES_PER_LED);
}

// Load a single LED's data into the dma buffer
static void load_next_led(uint8_t strip, uint8_t index, dma_buffer_half_t half)
{
	if(index >= RGB_NUM_LEDS) return;

	for(size_t byte = 0; byte < BYTES_PER_LED; byte++)
	{
		dma_buffer[strip][(half ? 8 * BYTES_PER_LED : 0) + byte * 8 + 0] = (buffer[strip][index * BYTES_PER_LED + byte] & (1 << 7)) ? timer_ccr_one : timer_ccr_zero;
		dma_buffer[strip][(half ? 8 * BYTES_PER_LED : 0) + byte * 8 + 1] = (buffer[strip][index * BYTES_PER_LED + byte] & (1 << 6)) ? timer_ccr_one : timer_ccr_zero;
		dma_buffer[strip][(half ? 8 * BYTES_PER_LED : 0) + byte * 8 + 2] = (buffer[strip][index * BYTES_PER_LED + byte] & (1 << 5)) ? timer_ccr_one : timer_ccr_zero;
		dma_buffer[strip][(half ? 8 * BYTES_PER_LED : 0) + byte * 8 + 3] = (buffer[strip][index * BYTES_PER_LED + byte] & (1 << 4)) ? timer_ccr_one : timer_ccr_zero;
		dma_buffer[strip][(half ? 8 * BYTES_PER_LED : 0) + byte * 8 + 4] = (buffer[strip][index * BYTES_PER_LED + byte] & (1 << 3)) ? timer_ccr_one : timer_ccr_zero;
		dma_buffer[strip][(half ? 8 * BYTES_PER_LED : 0) + byte * 8 + 5] = (buffer[strip][index * BYTES_PER_LED + byte] & (1 << 2)) ? timer_ccr_one : timer_ccr_zero;
		dma_buffer[strip][(half ? 8 * BYTES_PER_LED : 0) + byte * 8 + 6] = (buffer[strip][index * BYTES_PER_LED + byte] & (1 << 1)) ? timer_ccr_one : timer_ccr_zero;
		dma_buffer[strip][(half ? 8 * BYTES_PER_LED : 0) + byte * 8 + 7] = (buffer[strip][index * BYTES_PER_LED + byte] & (1 << 0)) ? timer_ccr_one : timer_ccr_zero;
	}
}

// Process state machine when DMA transfer is half complete
static void dma_process_halfcomplete(uint8_t strip)
{
	if(state[strip] == STATE_DATA)
	{
		(led_index[strip])++;

		// stop dma if we've sent all leds, otherwise load next led
		if(led_index[strip] >= RGB_NUM_LEDS)
		{
			// data complete
			HAL_TIM_PWM_Stop_DMA(&htims[strip], TIM_CHANNEL_1);

			// set dma buffer to reset pulse (timer cc reg = 0)
			memset(dma_buffer[strip], 0, sizeof(dma_buffer[0]));

			// move to end reset state and start DMA transfer
			state[strip] = STATE_END_RESET;
			HAL_TIM_PWM_Start_DMA(&htims[strip], TIM_CHANNEL_1, (uint32_t *)dma_buffer[strip], 2 * 8 * BYTES_PER_LED);
		}
		else
		{
			load_next_led(strip, led_index[strip] + 2, BUF_FIRST_HALF);
		}
	}
}

// Process state machine when DMA transfer is complete
static void dma_process_complete(uint8_t strip)
{
	if(state[strip] == STATE_START_RESET)
	{
		// start reset pulse complete
		HAL_TIM_PWM_Stop_DMA(&htims[strip], TIM_CHANNEL_1);

		// preload leds in dma buffer
		load_next_led(strip, 0, BUF_FIRST_HALF);
		load_next_led(strip, 1, BUF_SECOND_HALF);
		led_index[strip] = 0;

		// move to data state and start DMA transfer
		state[strip] = STATE_DATA;
		HAL_TIM_PWM_Start_DMA(&htims[strip], TIM_CHANNEL_1, (uint32_t *)dma_buffer[strip], 2 * 8 * BYTES_PER_LED);
	}
	else if(state[strip] == STATE_DATA)
	{
		(led_index[strip])++;

		// stop dma if we've sent all leds, otherwise load next led
		if(led_index[strip] >= RGB_NUM_LEDS)
		{
			// data complete
			HAL_TIM_PWM_Stop_DMA(&htims[strip], TIM_CHANNEL_1);

			// set dma buffer to reset pulse (timer cc reg = 0)
			memset(dma_buffer[strip], 0, sizeof(dma_buffer[0]));

			// move to end reset state and start DMA transfer
			state[strip] = STATE_END_RESET;
			HAL_TIM_PWM_Start_DMA(&htims[strip], TIM_CHANNEL_1, (uint32_t *)dma_buffer[strip], 2 * 8 * BYTES_PER_LED);
		}
		else
		{
			load_next_led(strip, led_index[strip] + 2, BUF_SECOND_HALF);
		}
	}
	else if(state[strip] == STATE_END_RESET)
	{
		// end reset pulse complete
		HAL_TIM_PWM_Stop_DMA(&htims[strip], TIM_CHANNEL_1);

		// reset state machine
		state[strip] = STATE_INIT;
	}
}


//------------------------------------------------------------------------------
// ISRs
//------------------------------------------------------------------------------

// RGB1 DMA Half Complete / Transfer Complete ISR
void DMA1_Channel3_IRQHandler(void)
{
	if(__HAL_DMA_GET_FLAG(&hdmas[0], DMA_FLAG_HT3))
	{
		// DMA transfer half complete
		dma_process_halfcomplete(0);
	}
	else if(__HAL_DMA_GET_FLAG(&hdmas[0], DMA_FLAG_TC3))
	{
		// DMA transfer complete
		dma_process_complete(0);
	}

	HAL_DMA_IRQHandler(&hdmas[0]);
}

// RGB2 DMA Half Complete / Transfer Complete ISR
void DMA1_Channel1_IRQHandler(void)
{
	if(__HAL_DMA_GET_FLAG(&hdmas[1], DMA_FLAG_HT1))
	{
		// DMA transfer half complete
		dma_process_halfcomplete(1);
	}
	else if(__HAL_DMA_GET_FLAG(&hdmas[1], DMA_FLAG_TC1))
	{
		// DMA transfer complete
		dma_process_complete(1);
	}

	HAL_DMA_IRQHandler(&hdmas[1]);
}
