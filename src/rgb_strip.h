//==============================================================================
// Addressable RGB LED Strip Driver
// Ian Glen <ian@ianglen.me>
//==============================================================================

#ifndef ATLC_RGB_STRIP_H
#define ATLC_RGB_STRIP_H


//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------

#include "config.h"


//------------------------------------------------------------------------------
// Definitions
//------------------------------------------------------------------------------

typedef enum
{
	RGB_STRIP_DISABLED = 0,
	RGB_STRIP_COLOR = 1,
	RGB_STRIP_RAINBOW = 2,
} rgb_strip_mode_t;


//------------------------------------------------------------------------------
// Public Functions
//------------------------------------------------------------------------------

void rgb_strip_init(void);
void rgb_strip_deinit(void);
void rgb_strip_disable(uint8_t strip);
void rgb_strip_set_color(uint8_t strip, uint8_t r, uint8_t g, uint8_t b);
void rgb_strip_set_rainbow(uint8_t strip);
void rgb_strip_task(void);


#endif // ATLC_RGB_STRIP_H
