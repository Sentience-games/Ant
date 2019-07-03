#pragma once 

#include "ant_types.h"
#include "ant_shared.h"

#include "utils/cstring.h"
#include "math/utils.h"

#include <stdarg.h>

#define CONST_STRING(STR) { sizeof(STR) - 1, (U8*)(STR) }

inline String
WrapCString (const char* cstring)
{
	String result = {};
    
	result.size = StrLength(cstring);
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
IsEndOfLine(char c)
{
    return (c == '\n' || c == '\r');
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

inline bool
IsHex(char c)
{
    return IsNumeric(c) || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
}

inline void
Advance (char** dest, UMM* dest_capacity)
{
    Assert(dest && dest_capacity);
    
    if (*dest_capacity != 0)
    {
        ++(*dest);
        --(*dest_capacity);
    }
}

inline void
Advance(String* string, UMM amount = 1)
{
    if (string->size > amount)
    {
        string->data += amount;
        string->size -= amount;
    }
    
    else
    {
        *string = {};
    }
}

inline void
Append (char** dest, UMM* dest_capacity, char data)
{
    Assert(dest && dest_capacity);
    
    if (dest_capacity != 0)
    {
        **dest = data;
        Advance(dest, dest_capacity);
    }
}

inline void
AppendCString (char** dest, UMM* dest_capacity, const char* cstring)
{
    Assert(dest && dest_capacity && cstring);
    
    char* at = (char*) cstring;
    
    while (dest_capacity && *at)
    {
        Append(dest, dest_capacity, *(at++));
    }
}

// TODO(soimn): UTF-8 unicode support
// TODO(soimn): Ensure this works properly on 32-Bit systems, int literal size
// TODO(soimn): Find a better way to report the required size than the if (dest_capacity) {} ++required_size
inline UMM
FormatString (char* dest, UMM dest_capacity, const char* format, va_list arg_list)
{
    UMM required_size = 0;
    
    for (char* scan = (char*) format; *scan; ++scan)
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
                        if (dest_capacity)
                        {
                            Append(&dest, &dest_capacity, *scan);
                        }
                    } break;
                    
                    case 'S':
                    {
                        String string = va_arg(arg_list, String);
                        char* string_scan = (char*) string.data;
                        
                        if (dest_capacity)
                        {
                            while (string_scan < (char*) string.data + string.size)
                            {
                                Append(&dest, &dest_capacity, *string_scan);
                                ++string_scan;
                            }
                        }
                        
                        required_size += string.size;
                    } break;
                    
                    case 's':
                    {
                        const char* cstring = va_arg(arg_list, const char*);
                        
                        // TODO(soimn): consider asserting cstring is not NULL instead of jumping over the string
                        while(cstring && *cstring)
                        {
                            if (dest_capacity)
                            {
                                Append(&dest, &dest_capacity, *cstring);
                            }
                            
                            ++cstring;
                            ++required_size;
                        }
                    } break;
                    
                    case 'I':
                    case 'U':
                    {
                        U32 num = va_arg(arg_list, U32);
                        
                        if (*scan == 'I' && (I32)num < 0)
                        {
                            if (dest_capacity)
                            {
                                Append(&dest, &dest_capacity, '-');
                            }
                            
                            num = (U32)((I32) num * -1);
                            ++required_size;
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
                            
                            if (dest_capacity)
                            {
                                Append(&dest, &dest_capacity, (char)(largest_place_num + '0'));
                            }
                            
                            ++required_size;
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
                            if (dest_capacity)
                            {
                                AppendCString(&dest, &dest_capacity, "true");
                            }
                            
                            required_size += 4;
                        }
                        
                        else
                        {
                            if (dest_capacity)
                            {
                                AppendCString(&dest, &dest_capacity, "false");
                            }
                            
                            required_size += 5;
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
            if (dest_capacity)
            {
                Append(&dest, &dest_capacity,*scan);
            }
            
            ++required_size;
        }
    }
    
    return required_size;
}

inline UMM
FormatString (char* dest, UMM dest_capacity, const char* format, ...)
{
    va_list arg_list;
    va_start(arg_list, format);
    UMM required_size = FormatString(dest, dest_capacity, format, arg_list);
    va_end(arg_list);
    
    return required_size;
}

inline bool
StringCompare(String string_0, String string_1)
{
    while ((string_0.size && string_1.size) && (string_0.data[0] == string_1.data[0]))
    {
        Advance(&string_0);
        Advance(&string_1);
    }
    
    return !string_0.size && string_0.size == string_1.size;
}

inline void
EatAllWhitespace(String* input)
{
    while (input->size && IsWhitespace(input->data[0]))
    {
        Advance(input);
    }
}

inline String
GetWord(String* input)
{
    String result = {};
    
    EatAllWhitespace(input);
    
    result.data = input->data;
    
    while (input->size && !(IsWhitespace(input->data[0]) || IsEndOfLine(input->data[0])))
    {
        Advance(input);
        ++result.size;
    }
    
    return result;
}

inline bool
ParseInt(String input, U8 base, I64* result)
{
    bool is_erroneous = false;
    
    Assert(base == 2 || base == 10 || base == 16);
    
    I64 acc = 0;
    
    U8 sign = 0;
    if (input.size && input.data[0] == '-')
    {
        sign = 1;
    }
    
    for (UMM i = sign; i < input.size; ++i)
    {
        if (!IsNumeric(input.data[i]) && !(base == 16 && IsHex(input.data[i])))
        {
            is_erroneous = true;
        }
        
        I64 prev_acc = acc;
        acc *= base;
        acc += input.data[i] - (input.data[i] >= 'a' ? 'a' : (input.data[i] >= 'A' ? 'A' : '0'));
        
        if (prev_acc > acc)
        {
            is_erroneous = true;
            break;
        }
    }
    
    if (!is_erroneous)
    {
        *result = acc * (sign ? -1 : 1);
    }
    
    return !is_erroneous;
}

inline bool
ParseFloat(String input, F32* result)
{
    bool is_erroneous = false;
    
    U64 acc = 0;
    
    U8 sign = 0;
    if (input.size && input.data[0] == '-')
    {
        sign = 1;
    }
    
    bool encountered_point = false;
    UMM point_place        = 0;
    
    for (UMM i = sign; i < input.size; ++i)
    {
        if (!IsNumeric(input.data[i]) && (encountered_point && input.data[i] == '.'))
        {
            is_erroneous = true;
            break;
        }
        
        else
        {
            if (input.data[i] == '.')
            {
                point_place      = i;
                encountered_point = true;
                continue;
            }
            
            else
            {
                U64 prev_acc = acc;
                acc *= 10;
                acc += input.data[i] - '0';
                
                if (prev_acc > acc)
                {
                    is_erroneous = true;
                    break;
                }
            }
        }
    }
    
    if (!is_erroneous)
    {
        *result = (sign ? -1.0f : 1.0f) * (F32)acc;
        
        if (encountered_point)
        {
            *result = *result / (F32)(input.size - point_place);
        }
    }
    
    return !is_erroneous;
}

inline bool
FindChar(String input, char c, UMM* result)
{
    bool found_char = false;
    
    for (UMM i = 0; i < input.size; ++i)
    {
        if (input.data[i] == c)
        {
            found_char = true;
            *result = i;
        }
    }
    
    return found_char;
}