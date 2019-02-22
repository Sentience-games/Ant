#pragma once

#ifdef ASSERTION_ENABLED
#define Assert(EX) if ((EX)) {} else { *(int*)0 = 0; }
#define StaticAssert(EX) static_assert(EX, "Assertion failed: " #EX);
#else
#define Assert(EX)
#define StaticAssert(EX)
#endif
