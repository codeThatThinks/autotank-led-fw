//==============================================================================
// GPIO Driver
// Ian Glen <ian@ianglen.me>
//==============================================================================

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------

#include <stdbool.h>
#include <stdint.h>
#include <stm32f3xx_hal.h>

#include "config.h"
#include "debug.h"
#include "gpio.h"


//------------------------------------------------------------------------------
// Definitions
//------------------------------------------------------------------------------

#ifdef TRUTH_TABLE

typedef struct
{
	bool enabled;
	uint16_t table;
} truth_table_t;

#endif //TRUTH_TABLE


#ifdef PIN_INTERRUPT

typedef enum
{
	INT_DISABLED = 0,
	INT_RISING_EDGE = 1,
	INT_FALLING_EDGE = 2,
	INT_ANY_CHANGE = 3,
	NUM_MODES
} pin_interrupt_mode_t;

typedef struct
{
	pin_interrupt_mode_t mode;
	uint8_t can_id;
} pin_interrupt_t;

#endif // PIN_INTERRUPT


//------------------------------------------------------------------------------
// Private Variables
//------------------------------------------------------------------------------

#ifdef TRUTH_TABLE

static truth_table_t out_truth_tables[4] = {0};

#endif // TRUTH_TABLE


#ifdef PIN_INTERRUPT

static pin_interrupt_t in_interrupts[4] = {0};

#endif // PIN_INTERRUPT


//------------------------------------------------------------------------------
// Public Functions
//------------------------------------------------------------------------------

// Initialize GPIO pins
void gpio_init(void)
{
	GPIO_InitTypeDef gpio_config = {0};

	// config status pin
	HAL_GPIO_WritePin(STATUS_PORT, STATUS_PIN, GPIO_PIN_SET);
	gpio_config.Pin = STATUS_PIN;
	gpio_config.Mode = GPIO_MODE_OUTPUT_PP;
	gpio_config.Pull = GPIO_NOPULL;
	gpio_config.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(STATUS_PORT, &gpio_config);

	// configure outputs
	HAL_GPIO_WritePin(OUT_PORT, OUT_PINS, GPIO_PIN_RESET);
	gpio_config.Pin = OUT_PINS;
	gpio_config.Mode = GPIO_MODE_OUTPUT_PP;
	gpio_config.Pull = GPIO_NOPULL;
	gpio_config.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(OUT_PORT, &gpio_config);

	// configure inputs
	gpio_config.Pin = IN_PINS;
	gpio_config.Mode = GPIO_MODE_INPUT;
	gpio_config.Pull = GPIO_PULLDOWN;
	HAL_GPIO_Init(IN_PORT, &gpio_config);

	// configure unused pins
	gpio_config.Mode = GPIO_MODE_ANALOG;
	gpio_config.Pull = GPIO_NOPULL;
	gpio_config.Pin = UNUSED_PORTA;
	HAL_GPIO_Init(GPIOA, &gpio_config);
	gpio_config.Pin = UNUSED_PORTB;
	HAL_GPIO_Init(GPIOB, &gpio_config);
	gpio_config.Pin = UNUSED_PORTC;
	HAL_GPIO_Init(GPIOC, &gpio_config);
	gpio_config.Pin = UNUSED_PORTD;
	HAL_GPIO_Init(GPIOD, &gpio_config);
	gpio_config.Pin = UNUSED_PORTF;
	HAL_GPIO_Init(GPIOF, &gpio_config);
}

// Return a 4-bit value representing input states
uint8_t gpio_read_inputs(void)
{
	return HAL_GPIO_ReadPin(IN_PORT, IN1_PIN)
		| (HAL_GPIO_ReadPin(IN_PORT, IN2_PIN) << 1)
		| (HAL_GPIO_ReadPin(IN_PORT, IN3_PIN) << 2)
		| (HAL_GPIO_ReadPin(IN_PORT, IN4_PIN) << 3);
}

// Return a 4-bit value representing output states
uint8_t gpio_read_outputs(void)
{
	return HAL_GPIO_ReadPin(OUT_PORT, OUT1_PIN)
		| (HAL_GPIO_ReadPin(OUT_PORT, OUT2_PIN) << 1)
		| (HAL_GPIO_ReadPin(OUT_PORT, OUT3_PIN) << 2)
		| (HAL_GPIO_ReadPin(OUT_PORT, OUT4_PIN) << 3);
}

// Write a 4-bit value to the outputs
void gpio_write_outputs(uint8_t states)
{
	HAL_GPIO_WritePin(OUT_PORT, OUT1_PIN, (states & 0x1) ? GPIO_PIN_SET : GPIO_PIN_RESET);
	HAL_GPIO_WritePin(OUT_PORT, OUT2_PIN, (states & 0x2) ? GPIO_PIN_SET : GPIO_PIN_RESET);
	HAL_GPIO_WritePin(OUT_PORT, OUT3_PIN, (states & 0x4) ? GPIO_PIN_SET : GPIO_PIN_RESET);
	HAL_GPIO_WritePin(OUT_PORT, OUT4_PIN, (states & 0x8) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

// Write a value to an output
void gpio_write_output(uint8_t pin, uint8_t state)
{
	if(pin == 1) HAL_GPIO_WritePin(OUT_PORT, OUT1_PIN, state ? GPIO_PIN_SET : GPIO_PIN_RESET);
	else if(pin == 2) HAL_GPIO_WritePin(OUT_PORT, OUT2_PIN, state ? GPIO_PIN_SET : GPIO_PIN_RESET);
	else if(pin == 3) HAL_GPIO_WritePin(OUT_PORT, OUT3_PIN, state ? GPIO_PIN_SET : GPIO_PIN_RESET);
	else if(pin == 4) HAL_GPIO_WritePin(OUT_PORT, OUT4_PIN, state ? GPIO_PIN_SET : GPIO_PIN_RESET);
}


#ifdef TRUTH_TABLE

// Set truth table function for an output
void gpio_set_truth_table(uint8_t pin, bool enabled, uint16_t table)
{
	out_truth_tables[pin - 1].enabled = enabled;
	out_truth_tables[pin - 1].table = table;
}

// Update outputs based on truth tables
void gpio_process_truth_tables(void)
{
	uint8_t in_states = gpio_read_inputs();
	for(int i = 0; i < 4; i++)
	{
		// todo: fixme
		if(out_truth_tables[i].enabled) HAL_GPIO_WritePin(OUT_PORT, OUT1_PIN, out_truth_tables[i].table & (1 << in_states) ? GPIO_PIN_SET : GPIO_PIN_RESET);
	}
}

#endif // TRUTH_TABLE


#ifdef PIN_INTERRUPT

// Set pin interrupt function for an input
void gpio_set_interrupt(uint8_t pin, uint8_t mode, uint8_t can_id)
{
	if(mode >= NUM_MODES) return;

	in_interrupts[pin - 1].mode = mode;
	in_interrupts[pin - 1].can_id = can_id;
}

// Send can message based on input state
void gpio_process_interrupts(void)
{
	// todo: switch polling to pin change interrupt, otherwise we need to track pin state
}

#endif // PIN_INTERRUPT
