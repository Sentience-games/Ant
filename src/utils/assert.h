#pragma once

#ifdef ANT_ASSERTION_ENABLED
#define Assert(EX) if (!(EX)) *(int*)0 = 0
#else
#define Assert(EX)
#endif
