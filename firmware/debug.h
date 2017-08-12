#define _DEBUG 1

#ifndef debug_h
#include <Arduino.h>
#define debug_h
#if _DEBUG
#define DEBUG_PRINT(x) Serial.println(x)
#else
#define DEBUG_PRINT(x)
#endif
#endif
