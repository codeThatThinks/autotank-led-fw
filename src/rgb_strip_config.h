//==============================================================================
// Addressable RGB LED Strip Configuration
// Ian Glen <ian@ianglen.me>
//==============================================================================

#ifndef ATLC_RGB_STRIP_CONFIG_H
#define ATLC_RGB_STRIP_CONFIG_H


#ifdef RGB_WS2812B

#define FORMAT_GRB
#define PERIOD			1250	// ns
#define ONE_PULSE		800		// ns
#define ZERO_PULSE		400		// ns
#define RESET_PULSE		50000	// ns, multiple of PERIOD

#endif // RGB_WS2812B


#ifdef FORMAT_GRB
#define BYTES_PER_LED	3
#endif //FORMAT_GRB


#endif // ATLC_RGB_STRIP_CONFIG_H
