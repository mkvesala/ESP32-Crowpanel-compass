#pragma once

#include <stddef.h>
#include <stdint.h>

// Initialization table for Arduino_RGB_Display instance
// Extends the init table assortment in Arduino_RGB_Display.h of GFX_library_for_Arduino
// Usage in CrowPanelApplication constructor's member initialization:
//   _gfx(480, 480, &_bus, 0, true, &_init_bus, GFX_NOT_DEFINED, crowpanel_st7701_type5_init_operations, crowpanel_st7701_type5_init_operations_len)

extern const uint8_t crowpanel_st7701_type5_init_operations[];
extern const size_t crowpanel_st7701_type5_init_operations_len;