#pragma once

#include "ant_shared.h"

global_variable U32 npos = ~(0UL);

inline int
StrLength (const char* cstring, I32 max_value = I32_MAX - 1)
{
	Assert(cstring);
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
WStrLength (const wchar_t* wcstring, I32 max_value = I32_MAX - 1)
{
	Assert(wcstring);
	Assert(0 <= max_value);
    
	int length = 0;
    
	for (wchar_t* it = (wchar_t*)wcstring; *it; ++it)
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
StrCompare (const char* cstring_1, const char* cstring_2)
{
	Assert(cstring_1 && cstring_2);
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

inline bool
StrCompare (const char* cstring_1, const char* cstring_2, UMM cutoff)
{
    Assert(cstring_1 && cstring_2);
    char* p1 = (char*) cstring_1;
    char* p2 = (char*) cstring_2;
    
    UMM i = 0;
    for (; i < cutoff && *p1 == *p2; ++i, ++p1, ++p2);
    
	return (i == cutoff && *p1 == *p2);
}

inline bool
WStrCompare(wchar_t* wstring_0, wchar_t* wstring_1)
{
    Assert(wstring_0 && wstring_1);
	int result = -1;
	wchar_t* p0 = (wchar_t*)wstring_0;
	wchar_t* p1 = (wchar_t*)wstring_1;
    
	while(*p0 && *p0 == *p1
		  && (size_t)(p0 - wstring_0) <= npos)
	{
		++p0;
		++p1;
	}
    
	if ((p0 - wstring_0) != npos)
	{
		if (*p0 == *p1)
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
StrCopy (const char* source_str, char* dest_str, int dest_capacity)
{
	Assert(source_str && dest_str);
	Assert(dest_capacity >= 0);
    
	int result = -1;
	int source_length = StrLength(source_str);
    
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

inline int
StrConcat (const char* cstring_1, const char* cstring_2,
		   char* dest_str, int dest_capacity)
{
	Assert(cstring_1 && cstring_2 && dest_str);
	Assert(dest_capacity >= 0);
    
	int result = -1;
	int str_1_length = StrLength(cstring_1, dest_capacity);
	int str_2_length = StrLength(cstring_2, dest_capacity);
    
	if (str_1_length != -1 && str_2_length != -1)
	{
		result = StrCopy(cstring_1, dest_str, dest_capacity);
        
		if (result == str_1_length)
		{
			result += StrCopy(cstring_2, dest_str + str_1_length, MIN(dest_capacity - str_1_length, str_2_length + 1));
		}
        
		result = (result == str_1_length - 1 ? -1 : result);
	}
    
	return result;
}

inline int
StrConcat3 (const char* cstring_1, const char* cstring_2,
			const char* cstring_3,
			char* dest_str, int dest_capacity)
{
	Assert(cstring_1 && cstring_2 && cstring_3 && dest_str);
	Assert(dest_capacity >= 0);
    
	int result = -1;
	int str_1_length = StrLength(cstring_1, dest_capacity);
	int str_2_length = StrLength(cstring_2, dest_capacity);
	int str_3_length = StrLength(cstring_3, dest_capacity);
    
	if (str_1_length != -1 && str_2_length != -1
		&& str_3_length != -1)
	{
		result = StrCopy(cstring_1, dest_str, dest_capacity);
        
		if (result == str_1_length)
		{
			result += StrCopy(cstring_2, dest_str + str_1_length, MIN(dest_capacity - str_1_length, str_2_length + 1));
			result = (result == str_1_length - 1 ? -1 : result);
            
			if (result == str_1_length + str_2_length)
			{
				result += StrCopy(cstring_3, dest_str + str_1_length + str_2_length,
								  MIN(dest_capacity - str_1_length - str_2_length, str_3_length + 1));
				result = (result == str_1_length + str_2_length - 1 ? -1 : result);
			}
		}
        
	}
    
	return result;
}

inline int
SubstrCopy (const char* source_str, int num_chars,
			char* dest_str, int dest_capacity)
{
	Assert(source_str && dest_str);
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

inline int
SubstrConcat (const char* cstring_1, int num_chars_1,
			  const char* cstring_2, int num_chars_2,
			  char* dest_str, int dest_capacity)
{
	Assert(cstring_1 && cstring_2 && dest_str);
	Assert(0 <= num_chars_1 && num_chars_1 <= StrLength(cstring_1) &&
		   0 <= num_chars_2 && num_chars_2 <= StrLength(cstring_2) &&
		   dest_capacity > 0);
    
	int result = -1;
	num_chars_1 = (num_chars_1 == 0 ? StrLength(cstring_1, dest_capacity) : num_chars_1);
	num_chars_2 = (num_chars_2 == 0 ? StrLength(cstring_2, dest_capacity) : num_chars_2);
    
	result  = SubstrCopy(cstring_1, num_chars_1, dest_str, dest_capacity);
    
	if (result == num_chars_1)
	{
		result += SubstrCopy(cstring_2, num_chars_2, dest_str + num_chars_1, MIN(dest_capacity - num_chars_1, num_chars_2 + 1));
	}
    
	result = (result == num_chars_1 - 1 ? -1 : result);
    
	return result;
}

inline int
SubstrConcat3 (const char* cstring_1, int num_chars_1,
			   const char* cstring_2, int num_chars_2,
			   const char* cstring_3, int num_chars_3,
			   char* dest_str, int dest_capacity)
{
	Assert(cstring_1 && cstring_2 && cstring_3 && dest_str);
	Assert(0 <= num_chars_1 && num_chars_1 <= StrLength(cstring_1) &&
		   0 <= num_chars_2 && num_chars_2 <= StrLength(cstring_2) &&
		   0 <= num_chars_3 && num_chars_3 <= StrLength(cstring_3) &&
		   dest_capacity > 0);
    
	int result = -1;
    
	num_chars_1 = (num_chars_1 == 0 ? StrLength(cstring_1, dest_capacity) : num_chars_1);
	num_chars_2 = (num_chars_2 == 0 ? StrLength(cstring_2, dest_capacity) : num_chars_2);
	num_chars_3 = (num_chars_3 == 0 ? StrLength(cstring_3, dest_capacity) : num_chars_3);
    
	if (num_chars_1 != -1 && num_chars_2 != -1 && num_chars_3 != -1)
	{
		result  = SubstrCopy(cstring_1, num_chars_1, dest_str, dest_capacity);
        
		if (result == num_chars_1)
		{
			result += SubstrCopy(cstring_2, num_chars_2, dest_str + num_chars_1, MIN(dest_capacity - num_chars_1, num_chars_2 + 1));
			result = (result == num_chars_1 - 1 ? -1 : result);
            
			if (result == num_chars_1 + num_chars_2)
			{
				result += SubstrCopy(cstring_3, num_chars_3, dest_str + num_chars_1 + num_chars_2,
									 MIN(dest_capacity - num_chars_1 - num_chars_2, num_chars_3 + 1));
				result = (result == num_chars_1 + num_chars_2 - 1 ? -1 : result);
			}
		}
	}
    
	return result;
}


// TODO(soimn): clean this up, stupid use of variable: length
inline U32
StrFind (const char* cstr, const char ch, bool find_last = false)
{
	Assert(cstr);
	U32 result = npos;
    
	U32 length = 0;
	for (char* it = (char*) cstr; *it; ++it)
	{
		if (length >= npos - 1)
		{
			result = npos;
			break;
		}
        
		else
		{
			++length;
            
			if (*it == ch)
			{
				result = (U32)(it - cstr);
                
				if (!find_last)
				{
					break;
				}
			}
		}
	}
	
	return result;
}
