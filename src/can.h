//==============================================================================
// CAN Driver
// Ian Glen <ian@ianglen.me>
//==============================================================================

#ifndef ATLC_CAN_H
#define ATLC_CAN_H


//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------

#include <stdbool.h>
#include <stdint.h>
#include <stm32f3xx_hal.h>


//------------------------------------------------------------------------------
// Definitions
//------------------------------------------------------------------------------

typedef enum {
	CAN_CMD_READ_PINS = 0,
	CAN_CMD_WRITE_PINS = 1,
	CAN_CMD_WRITE_PIN = 2,
	CAN_CMD_TRUTH_TABLE = 3,
	CAN_CMD_PIN_INTERRUPT = 4,
	CAN_CMD_RGB_STRIP_1 = 5,
	CAN_CMD_RGB_STRIP_2 = 6
} can_cmd_t;

typedef struct {
	can_cmd_t cmd;
	uint8_t id;
	uint8_t payload[8];
	uint8_t len;
} can_msg_t;


//------------------------------------------------------------------------------
// Public Functions
//------------------------------------------------------------------------------

void can_init(void);
void can_deinit(void);
void can_send(uint8_t id, can_cmd_t cmd, uint8_t *payload, uint8_t len);
bool can_receive(can_msg_t *msg);


#endif  // ATLC_CAN_H
