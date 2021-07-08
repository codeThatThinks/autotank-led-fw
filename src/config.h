//==============================================================================
// Firmware Configuration
// Ian Glen <ian@ianglen.me>
//==============================================================================

#ifndef ATLC_CONFIG_H
#define ATLC_CONFIG_H


//------------------------------------------------------------------------------
// Definitions
//------------------------------------------------------------------------------

// Our 8-bit CAN node id
#define CAN_ID					0xA3

// Status LED blink period
#define STATUS_BLINK_TIME		50	// ms

// Features
#define DEBUG
//#define TRUTH_TABLE
//#define PIN_INTERRUPT
#define RGB_STRIP

// RGB Strip settings
#define RGB_WS2812B
#define RGB_NUM_STRIPS			2
#define RGB_NUM_LEDS			36


#endif	// ATLC_CONFIG_H


//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------

#include "rgb_strip_config.h"
