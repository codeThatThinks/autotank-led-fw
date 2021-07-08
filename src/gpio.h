//==============================================================================
// GPIO Driver
// Ian Glen <ian@ianglen.me>
//==============================================================================

#ifndef ATLC_GPIO_H
#define ATLC_GPIO_H


//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------

#include <stdbool.h>
#include <stdint.h>
#include <stm32f3xx_hal.h>

#include "config.h"


//------------------------------------------------------------------------------
// Definitions
//------------------------------------------------------------------------------

/*
	Outputs:
	- PC0: Status LED
	- PB12: Output 1
	- PB13: Output 2
	- PB14: Output 3
	- PB15: OUtput 4

	Inputs:
	- PA0: Input 1
	- PA1: Input 2
	- PA2: Input 3
	- PA3: Input 4

	All unused pins set to analog to reduce power consumption.
*/

#define STATUS_PORT		GPIOC
#define STATUS_PIN		GPIO_PIN_0

#define OUT_PORT		GPIOB
#define OUT1_PIN		GPIO_PIN_12
#define OUT2_PIN		GPIO_PIN_13
#define OUT3_PIN		GPIO_PIN_14
#define OUT4_PIN		GPIO_PIN_15
#define OUT_PINS		OUT1_PIN | OUT2_PIN | OUT3_PIN | OUT4_PIN

#define IN_PORT			GPIOA
#define IN1_PIN			GPIO_PIN_0
#define IN2_PIN			GPIO_PIN_1
#define IN3_PIN			GPIO_PIN_2
#define IN4_PIN			GPIO_PIN_3
#define IN_PINS			IN1_PIN | IN2_PIN | IN3_PIN | IN4_PIN

#define UNUSED_PORTA	GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_15
#define UNUSED_PORTB	GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11
#define UNUSED_PORTC	GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15
#define UNUSED_PORTD	GPIO_PIN_2
#define UNUSED_PORTF	GPIO_PIN_0 | GPIO_PIN_1


//------------------------------------------------------------------------------
// Public Functions
//------------------------------------------------------------------------------

void gpio_init(void);
uint8_t gpio_read_inputs(void);
uint8_t gpio_read_outputs(void);
void gpio_write_outputs(uint8_t states);
void gpio_write_output(uint8_t pin, uint8_t state);


#ifdef TRUTH_TABLE

void gpio_set_truth_table(uint8_t pin, bool enabled, uint16_t table);
void gpio_process_truth_tables(void);

#endif // TRUTH_TABLE


#ifdef PIN_INTERRUPT

void gpio_set_interrupt(uint8_t pin, uint8_t mode, uint8_t can_id);
void gpio_process_interrupts(void);

#endif //PIN_INTERRUPT


#endif  // ATLC_GPIO_H
