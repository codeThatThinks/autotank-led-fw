//==============================================================================
// Debugging Functions
// Ian Glen <ian@ianglen.me>
//==============================================================================

#ifndef ATLC_DEBUG_H
#define ATLC_DEBUG_H


//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------

#include <stdio.h>

#include "config.h"


//------------------------------------------------------------------------------
// Definitions
//------------------------------------------------------------------------------

#ifdef DEBUG

#define debug_printf(fmt, ...) printf("%s:%d: "fmt"\r\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define debug_assert(condition, fmt, ...) if(!(condition)) { printf("%s:%d: "fmt" -- aborting!\r\n", __FILE__, __LINE__, ##__VA_ARGS__); error_state(); }
#define debug_abort(fmt, ...) printf("%s:%d: "fmt" -- aborting!\r\n", __FILE__, __LINE__, ##__VA_ARGS__); error_state()

#else

#define debug_printf(fmt, ...)
#define debug_assert(condition, fmt, ...) if(!(condition)) { error_state(); }
#define debug_abort(fmt, ...) error_state()

#endif // DEBUG


#define TO_STR(var) #var


//------------------------------------------------------------------------------
// Public Functions
//------------------------------------------------------------------------------

void error_state(void);


#endif	// ATLC_DEBUG_H
