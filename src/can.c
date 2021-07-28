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
static volatile can_msg_t buffer[CAN_BUF_SIZE];
static volatile size_t buf_write_pos;
static volatile size_t buf_read_pos;


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

	// configure interrupts
	__HAL_CAN_ENABLE_IT(&hcan, CAN_IT_RX_FIFO0_MSG_PENDING | CAN_IT_RX_FIFO0_FULL | CAN_IT_RX_FIFO0_OVERRUN);
	HAL_NVIC_SetPriority(USB_LP_CAN_RX0_IRQn, 0, 1);
	HAL_NVIC_EnableIRQ(USB_LP_CAN_RX0_IRQn);

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
	HAL_NVIC_DisableIRQ(USB_LP_CAN_RX0_IRQn);
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
	HAL_NVIC_DisableIRQ(USB_LP_CAN_RX0_IRQn);

	if(buf_read_pos == buf_write_pos)
	{
		HAL_NVIC_EnableIRQ(USB_LP_CAN_RX0_IRQn);
		return false;
	}

	// copy message from buffer
	msg->cmd = buffer[buf_read_pos].cmd;
	msg->id = buffer[buf_read_pos].id;
	for(size_t i = 0; i < buffer[buf_read_pos].len; i++) msg->payload[i] = buffer[buf_read_pos].payload[i];
	msg->len = buffer[buf_read_pos].len;

	// increment read position
	buf_read_pos++;
	if(buf_read_pos >= CAN_BUF_SIZE) buf_read_pos = 0;

	HAL_NVIC_EnableIRQ(USB_LP_CAN_RX0_IRQn);

	// blink status LED on activity
	HAL_GPIO_WritePin(STATUS_PORT, STATUS_PIN, GPIO_PIN_RESET);
	HAL_Delay(STATUS_BLINK_TIME);
	HAL_GPIO_WritePin(STATUS_PORT, STATUS_PIN, GPIO_PIN_SET);

	return true;
}


//------------------------------------------------------------------------------
// ISRs
//------------------------------------------------------------------------------

// CAN FIFO0 receive ISR
void USB_LP_CAN_RX0_IRQHandler(void)
{
	CAN_RxHeaderTypeDef msg_header;
	uint8_t msg_payload[8];

	if(HAL_CAN_GetRxFifoFillLevel(&hcan, CAN_RX_FIFO0) > 0)
	{
		if(HAL_CAN_GetRxMessage(&hcan, CAN_RX_FIFO0, &msg_header, msg_payload) == HAL_OK)
		{
			buffer[buf_write_pos].cmd = msg_header.ExtId >> 8;
			buffer[buf_write_pos].id = msg_header.ExtId & 0xFF;
			for(size_t i = 0; i < msg_header.DLC; i++) buffer[buf_write_pos].payload[i] = msg_payload[i];
			buffer[buf_write_pos].len = msg_header.DLC;

			// increment write position
			buf_write_pos++;
			if(buf_write_pos >= CAN_BUF_SIZE) buf_write_pos = 0;

			// bump read position if buffer is full
			if(buf_read_pos == buf_write_pos)
			{
				buf_read_pos++;
				if(buf_read_pos >= CAN_BUF_SIZE) buf_read_pos = 0;
			}
		}
		else
		{
			//debug_printf("Error receiving CAN message");
		}
	}

	HAL_CAN_IRQHandler(&hcan);
}
