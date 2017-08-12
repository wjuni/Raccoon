#define _DEBUG 1

#ifndef debug_h
#include <iostream>
#define debug_h
#if _DEBUG
#define DEBUG_PRINT(x) std::cout << (x)
#else
#define DEBUG_PRINT(x)
#endif
#endif
