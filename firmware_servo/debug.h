#define _DEBUG 0

#ifndef debug_h
#include <Arduino.h>
#define debug_h
#if _DEBUG
#define DEBUG_PRINT(x) Serial.println(x)
#define DEBUG_PRINT_(x) Serial.print(x)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINT_(x)
#endif
#endif
