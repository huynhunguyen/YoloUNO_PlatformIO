#include <Arduino.h>

// ==============================================================
// ARCHITECTURE SELECTION:
// Uncomment the line below to run the RTOS (Multi-threading) mode.
// Comment it out to run the traditional Super Loop mode.
// ==============================================================
#define USE_RTOS_MODE


#ifdef USE_RTOS_MODE
  // Include RTOS style implementation
  #include "style_rtos.hpp"
#else
  // Include traditional Super Loop implementation
  #include "style_superloop.hpp"
#endif


void setup() {
#ifdef USE_RTOS_MODE
  setup_rtos();
#else
  setup_superloop();
#endif
}

void loop() {
#ifdef USE_RTOS_MODE
  loop_rtos();
#else
  loop_superloop();
#endif
}
