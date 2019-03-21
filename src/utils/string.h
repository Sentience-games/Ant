#pragma once 

#include "ant_shared.h"

#define CONST_STRING(STR) { sizeof(STR) - 1, (u8*)(STR) }

inline string
WrapCString(const char* cstring)
{
	string result = {};
    
	result.size = strlength(cstring);
	result.data = (u8*) cstring;
    
	return result;
}
