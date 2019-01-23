#pragma once

#include "utils/utility_defines.h"
#include "utils/assert.h"
#include "utils/fixed_int.h"

global_variable uint32 npos = ~(0UL);

inline int
strlength (const char* cstring, int32 max_value = INT32_MAX - 1)
{
	Assert(0 <= max_value);

	int length = 0;

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

// NOTE(soimn): this returns -1 on error and the number of characters written on success
inline int
strcopy (const char* source_str, char* dest_str, int dest_capacity)
{
	Assert(dest_capacity >= 0);

	int result = -1;
	int source_length = strlength(source_str);

	if (source_length != -1)
	{
		int num_to_write = MIN(dest_capacity - 1, source_length);
		for (int i = 0; i < num_to_write; ++i)
		{
			dest_str[i] = source_str[i];
		}

		dest_str[num_to_write] = '\0';

		result = num_to_write;
	}

	return result;
}

// NOTE(soimn): this will truncate the result if dest_capacity is not sufficient
inline int
strconcat (const char* cstring_1, const char* cstring_2, char* dest_str, int dest_capacity)
{
	Assert(dest_capacity >= 0);

	int result = -1;
	int str_1_length = strlength(cstring_1, dest_capacity);
	int str_2_length = strlength(cstring_2, dest_capacity);

	if (str_1_length != -1 && str_2_length != -1)
	{
		result = strcopy(cstring_1, dest_str, dest_capacity);

		if (result == str_1_length)
		{
			result += strcopy(cstring_2, dest_str + str_1_length, MIN(dest_capacity - str_1_length, str_2_length));
		}
	}

	return result;
}

inline int
substrcopy (const char* source_str, int num_chars, char* dest_str, int dest_capacity)
{
	Assert(dest_capacity >= 0);

	int result = -1;

	int num_to_write = MIN(dest_capacity - 1, num_chars);
	for (int i = 0; i < num_to_write; ++i)
	{
		dest_str[i] = source_str[i];
	}

	dest_str[num_to_write] = '\0';

	result = num_to_write;

	return result;
}
