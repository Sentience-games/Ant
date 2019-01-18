#pragma once

#include "utils/utility_defines.h"
#include "utils/assert.h"
#include "utils/fixed_int.h"

global_variable const size_t npos = ~((size_t)0);

/*
 *	NOTE(soimn):
 *	When a function returns a "ternary" value the following convention is followed
 *	 0 signifies false
 *	 1 signifies true
 *	-1 signifies an error
 */

inline int
strlength (const char* cstring, int32 max_value = INT32_MAX)
{
	Assert(0 <= max_value);

	int length = 0;

	// TODO(soimn): consider validating the string is null terminated
	for (char* it = (char*)cstring; *it; ++it)
	{
		if (length > max_value)
		{
			length = -1;
			break;
		}

		++length;
	}

	return length;
}

inline int
strcompare (const char* cstring_1, const char* cstring_2)
{
	int result = -1;
	char* p1 = (char*)cstring_1;
	char* p2 = (char*)cstring_2;

	while(*p1 && *p1 == *p2
		  && (size_t)(p1 - cstring_1) <= npos)
	{
		++p1;
		++p2;
	}

	if ((p1 - cstring_1) != npos)
	{
		if (*p1 == *p2)
		{
			result = 1;
		}
		else
		{
			result = 0;
		}
	}

	return result;
}
