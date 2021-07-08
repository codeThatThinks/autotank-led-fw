//==============================================================================
// CAN Driver
// Ian Glen <ian@ianglen.me>
//==============================================================================

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------

#include <stdbool.h>
#include <stdint.h>
#include <stm32f3xx_hal.h>

#include "can.h"
#include "config.h"
#include "debug.h"
#include "gpio.h"


//------------------------------------------------------------------------------
// Definitions
//------------------------------------------------------------------------------

/*
	PA11: CAN_RX
	PA12: CAN_TX
*/

#define CAN_PORT	GPIOA
#define CAN_PINS	GPIO_PIN_12 | GPIO_PIN_11


//------------------------------------------------------------------------------
// Private Variables
//------------------------------------------------------------------------------

static CAN_HandleTypeDef hcan;


//------------------------------------------------------------------------------
// Public Functions
//------------------------------------------------------------------------------

// Initialize CAN peripheral
void can_init(void)
{
	/*
		CAN baud rate configured to 100kbps
		Determine bit timings with a calculator such as http://www.bittiming.can-wiki.info/
	*/

	// configure gpio pins
	GPIO_InitTypeDef gpio_config = {0};
	gpio_config.Pin = CAN_PINS;
	gpio_config.Mode = GPIO_MODE_AF_PP;
	gpio_config.Pull = GPIO_NOPULL;
	gpio_config.Speed = GPIO_SPEED_FREQ_HIGH;
	gpio_config.Alternate = GPIO_AF9_CAN;
	HAL_GPIO_Init(CAN_PORT, &gpio_config);

	// configure peripheral
	hcan.Instance = CAN;
	hcan.Init.Prescaler = 20;
	hcan.Init.Mode = CAN_MODE_NORMAL;
	hcan.Init.SyncJumpWidth = CAN_SJW_1TQ;
	hcan.Init.TimeSeg1 = CAN_BS1_15TQ;
	hcan.Init.TimeSeg2 = CAN_BS2_2TQ;
	hcan.Init.TimeTriggeredMode = DISABLE;
	hcan.Init.AutoBusOff = DISABLE;
	hcan.Init.AutoWakeUp = DISABLE;
	hcan.Init.AutoRetransmission = DISABLE;
	hcan.Init.ReceiveFifoLocked = DISABLE;
	hcan.Init.TransmitFifoPriority = DISABLE;
	debug_assert(HAL_CAN_Init(&hcan) == HAL_OK, "Failed to configure CAN");

	// filter all msgs except extended-id with our can id
	CAN_FilterTypeDef filter_config = {0};
	filter_config.FilterIdLow = (CAN_ID << 3) | (1 << 2);
	filter_config.FilterMaskIdLow = (0xFF << 3) | (1 << 2);
	filter_config.FilterFIFOAssignment = CAN_FILTER_FIFO0;
	filter_config.FilterBank = 0;
	filter_config.FilterMode = CAN_FILTERMODE_IDMASK;
	filter_config.FilterScale = CAN_FILTERSCALE_32BIT;
	filter_config.FilterActivation = CAN_FILTER_ENABLE;
	debug_assert(HAL_CAN_ConfigFilter(&hcan, &filter_config) == HAL_OK, "Failed to configure CAN filter");

	// start receiving messages
	debug_assert(HAL_CAN_Start(&hcan) == HAL_OK, "Failed to start CAN");
}

// Deinitialize CAN peripheral
void can_deinit(void)
{
	HAL_CAN_Stop(&hcan);
	HAL_GPIO_DeInit(CAN_PORT, CAN_PINS);
}

// Send a CAN message
void can_send(uint8_t id, can_cmd_t cmd, uint8_t *payload, uint8_t len) {
	CAN_TxHeaderTypeDef msg_header = {0};
	msg_header.ExtId = cmd << 8 | id;
	msg_header.IDE = CAN_ID_EXT;
	msg_header.DLC = len;

	// wait for a tx to become free
	while(HAL_CAN_GetTxMailboxesFreeLevel(&hcan) == 0);

	// send it
	uint32_t mailbox;
	if(HAL_CAN_AddTxMessage(&hcan, &msg_header, payload, &mailbox) != HAL_OK)
	{
		debug_printf("Error sending CAN message");
		return;
	}

	// blink status LED on activity
	HAL_GPIO_WritePin(STATUS_PORT, STATUS_PIN, GPIO_PIN_RESET);
	HAL_Delay(STATUS_BLINK_TIME);
	HAL_GPIO_WritePin(STATUS_PORT, STATUS_PIN, GPIO_PIN_SET);
}

// Receive a CAN message
bool can_receive(can_msg_t *msg) {
	CAN_RxHeaderTypeDef msg_header;

	if(HAL_CAN_GetRxFifoFillLevel(&hcan, CAN_RX_FIFO0) == 0) return false;

	if(HAL_CAN_GetRxMessage(&hcan, CAN_RX_FIFO0, &msg_header, msg->payload) != HAL_OK)
	{
		debug_printf("Error receiving CAN message");
		return false;
	}

	msg->cmd = msg_header.ExtId >> 8;
	msg->id = msg_header.ExtId & 0xFF;
	msg->len = msg_header.DLC;

	// blink status LED on activity
	HAL_GPIO_WritePin(STATUS_PORT, STATUS_PIN, GPIO_PIN_RESET);
	HAL_Delay(STATUS_BLINK_TIME);
	HAL_GPIO_WritePin(STATUS_PORT, STATUS_PIN, GPIO_PIN_SET);

	return true;
}
