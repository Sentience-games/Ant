#pragma once 

#include "ant_types.h"
#include "ant_shared.h"

#include "utils/cstring.h"

#define CONST_STRING(STR) { sizeof(STR) - 1, (u8*)(STR) }

inline String
WrapCString (const char* cstring)
{
	String result = {};
    
	result.size = strlength(cstring);
	result.data = (U8*) cstring;
    
	return result;
}

inline bool
IsWhitespace (char c)
{
    return (   c == ' '
            || c == '\t'
            || c == '\v');
}

inline bool
IsAlpha (char c)
{
    return (   (c >= 'A' && c <= 'Z') 
            || (c >= 'a' && c <= 'z'));
}

inline bool
IsNumeric (char c)
{
    return (c >= '0' && c <= '9');
}

inline void
Advance (char** dest, Memory_Index* dest_capacity)
{
    Assert(dest && dest_capacity);
    
    if (*dest_capacity != 0)
    {
        ++(*dest);
        --(*dest_capacity);
    }
}

inline void
Append (char** dest, Memory_Index* dest_capacity, char data)
{
    Assert(dest && dest_capacity);
    
    if (dest_capacity != 0)
    {
        **dest = data;
        Advance(dest, dest_capacity);
    }
}

inline void
AppendCString (char** dest, Memory_Index* dest_capacity, const char* cstring)
{
    Assert(dest && dest_capacity && cstring);
    
    char* at = (char*) cstring;
    
    while (dest_capacity && *at)
    {
        Append(dest, dest_capacity, *(at++));
    }
}

// TODO(soimn): UTF-8 unicode support
// TODO(soimn): ensure this works properly on 32-Bit systems, int literal size
inline Memory_Index
FormatString (char* dest, Memory_Index dest_capacity, const char* format, va_list arg_list)
{
    Assert(dest && format);
    
    Memory_Index chars_written = dest_capacity;
    
    for (char* scan = (char*) format; *scan && dest_capacity; ++scan)
    {
        if (*scan == '%')
        {
            ++scan;
            
            if (*scan == 0)
            {
                Assert(!"Trailing percentage");
            }
            
            else
            {
                switch(*scan)
                {
                    case '%':
                    {
                        Append(&dest, &dest_capacity, *scan);
                    } break;
                    
                    case 'S':
                    {
                        String string = va_arg(arg_list, String);
                        char* string_scan = (char*) string.data;
                        
                        while (string_scan < (char*) string.data + string.size)
                        {
                            Append(&dest, &dest_capacity, *string_scan);
                            ++string_scan;
                        }
                    } break;
                    
                    case 's':
                    {
                        const char* cstring = va_arg(arg_list, const char*);
                        
                        // TODO(soimn): consider asserting cstring is not NULL instead of jumping over the string
                        while(cstring && *cstring)
                        {
                            Append(&dest, &dest_capacity, *cstring);
                            ++cstring;
                        }
                    } break;
                    
                    case 'I':
                    case 'U':
                    {
                        U32 num = va_arg(arg_list, U32);
                        
                        if (*scan == 'I' && (I32)num < 0)
                        {
                            Append(&dest, &dest_capacity, '-');
                            num = (U32)((I32) num * -1);
                        }
                        
                        U32 largest_place = 1;
                        U32 num_copy      = num / 10;
                        while (num_copy)
                        {
                            num_copy      /= 10;
                            largest_place *= 10;
                        }
                        
                        do
                        {
                            U32 largest_place_num = num / largest_place;
                            
                            num -= largest_place_num * largest_place;
                            largest_place /= 10;
                            
                            Append(&dest, &dest_capacity, (char)(largest_place_num + '0'));
                        } while (num != 0);
                    } break;
                    
                    case 'B':
                    case 'b':
                    {
                        bool boolean;
                        
                        if (*scan == 'b')
                        {
                            boolean = va_arg(arg_list, bool);
                        }
                        
                        else
                        {
                            boolean = va_arg(arg_list, B32);
                        }
                        
                        
                        if (boolean)
                        {
                            AppendCString(&dest, &dest_capacity, "true");
                        }
                        
                        else
                        {
                            AppendCString(&dest, &dest_capacity, "false");
                        }
                    } break;
                    
                    default:
                    Assert(!"Invalid format specifier");
                    break;
                }
            }
        }
        
        else
        {
            Append(&dest, &dest_capacity,*scan);
        }
    }
    
    chars_written -= dest_capacity;
    
    return chars_written;
}

inline Memory_Index
FormatString (char* dest, Memory_Index dest_capacity, const char* format, ...)
{
    va_list arg_list;
    va_start(arg_list, format);
    Memory_Index chars_written = FormatString(dest, dest_capacity, format, arg_list);
    va_end(arg_list);
    
    return chars_written;
}