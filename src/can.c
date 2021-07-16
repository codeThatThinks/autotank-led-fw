//==============================================================================
// CAN Driver
// Ian Glen <ian@ianglen.me>
//==============================================================================

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
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

#define CAN_BUF_SIZE	16


//------------------------------------------------------------------------------
// Private Variables
//------------------------------------------------------------------------------

static CAN_HandleTypeDef hcan;
static can_msg_t buffer[CAN_BUF_SIZE];
static size_t buf_write_pos;
static size_t buf_read_pos;


//------------------------------------------------------------------------------
// Public Functions
//------------------------------------------------------------------------------

// Initialize CAN peripheral
void can_init(void)
{
	/*
		Input clock is 36MHz
		CAN baud rate configured to 1Mbps
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

	// configure interrupts
	HAL_NVIC_SetPriority(USB_LP_CAN_RX0_IRQn, 1, 1);
	HAL_NVIC_EnableIRQ(USB_LP_CAN_RX0_IRQn);

	// configure peripheral
	hcan.Instance = CAN;
	hcan.Init.Prescaler = 2;
	hcan.Init.Mode = CAN_MODE_NORMAL;
	hcan.Init.SyncJumpWidth = CAN_SJW_1TQ;
	hcan.Init.TimeSeg1 = CAN_BS1_15TQ;
	hcan.Init.TimeSeg2 = CAN_BS2_2TQ;
	hcan.Init.TimeTriggeredMode = DISABLE;
	hcan.Init.AutoBusOff = ENABLE;
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
	HAL_NVIC_EnableIRQ(USB_LP_CAN_RX0_IRQn);
	HAL_CAN_Stop(&hcan);
	HAL_GPIO_DeInit(CAN_PORT, CAN_PINS);
}

// Send a CAN message
void can_send(uint8_t id, can_cmd_t cmd, uint8_t *payload, uint8_t len)
{
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
bool can_receive(can_msg_t *msg)
{
	if(buf_read_pos != buf_write_pos)
	{
		// copy message from buffer
		msg->cmd = buffer[buf_read_pos].cmd;
		msg->id = buffer[buf_read_pos].id;
		memcpy(msg->payload, buffer[buf_read_pos].payload, msg->len);
		msg->len = buffer[buf_read_pos].len;

		// increment read position
		if(buf_read_pos == (CAN_BUF_SIZE - 1))
			buf_read_pos = 0;
		else
			buf_read_pos++;

		// blink status LED on activity
		HAL_GPIO_WritePin(STATUS_PORT, STATUS_PIN, GPIO_PIN_RESET);
		HAL_Delay(STATUS_BLINK_TIME);
		HAL_GPIO_WritePin(STATUS_PORT, STATUS_PIN, GPIO_PIN_SET);

		return true;
	}

	return false;
}


//------------------------------------------------------------------------------
// ISRs
//------------------------------------------------------------------------------

// CAN FIFO0 receive ISR
void USB_LP_CAN_RX0_IRQHandler(void)
{
	if(HAL_CAN_GetRxFifoFillLevel(&hcan, CAN_RX_FIFO0) > 0)
	{
		CAN_RxHeaderTypeDef msg_header;
		if(HAL_CAN_GetRxMessage(&hcan, CAN_RX_FIFO0, &msg_header, buffer[buf_write_pos].payload) == HAL_OK)
		{
			buffer[buf_write_pos].cmd = msg_header.ExtId >> 8;
			buffer[buf_write_pos].id = msg_header.ExtId & 0xFF;
			buffer[buf_write_pos].len = msg_header.DLC;

			// increment write position
			if(buf_write_pos == (CAN_BUF_SIZE - 1))
				buf_write_pos = 0;
			else
				buf_write_pos++;

			if(buf_read_pos == buf_write_pos)
			{
				if(buf_read_pos == (CAN_BUF_SIZE - 1))
					buf_read_pos = 0;
				else
					buf_read_pos++;
			}
		}
		else
		{
			//debug_printf("Error receiving CAN message");
		}
	}

	HAL_CAN_IRQHandler(&hcan);
}
