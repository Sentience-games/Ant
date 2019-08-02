#pragma once

#include <stdarg.h>

inline UMM
CStringLength(const char* cstring)
{
    UMM result = 0;
    
    char* scan = (char*) cstring;
    
    while (scan && *scan) ++scan, ++result;
    
    return result;
}

inline String
WrapCString(const char* cstring)
{
    String result = {};
    
    result.data = (U8*) cstring;
    result.size = CStringLength(cstring);
    
    return result;
}

#define CONST_STRING(cstring) {sizeof(cstring) - 1, (U8*) (cstring)}

inline void
Append(char c, U8** dest, UMM* dest_capacity)
{
    // NOTE(soimn): The one off bias is due to null termination
    if (*dest && *dest_capacity > 1)
    {
        **dest = c;
        *dest          += 1;
        *dest_capacity -= 1;
    }
}

inline void
Append(const char* string, U8** dest, UMM* dest_capacity)
{
    UMM string_length = CStringLength(string);
    
    // NOTE(soimn): The one off bias is due to null termination
    if (*dest && *dest_capacity >= string_length + 1)
    {
        Copy((void*) string, *dest, string_length);
        *dest          += string_length;
        *dest_capacity -= string_length;
    }
}

inline UMM
FormatString(char* dest, UMM dest_capacity, const char* format, UMM format_length, va_list arg_list)
{
    UMM length = 0;
    U8* buffer = (U8*) dest;
    UMM buffer_capacity = dest_capacity;
    
    char* scan = (char*) format;
    
    while (scan < (char*) format + format_length)
    {
        if (*scan == '%')
        {
            ++scan;
            Assert(scan);
            
            switch (*scan)
            {
                case 's':
                {
                    char* string = va_arg(arg_list, char*);
                    
                    while (*string)
                    {
                        Append(*(string++), &buffer, &buffer_capacity);
                        ++length;
                    }
                } break;
                
                case 'S':
                {
                    String string = va_arg(arg_list, String);
                    
                    for (UMM i = 0; i < string.size; ++i)
                    {
                        Append(string.data[i], &buffer, &buffer_capacity);
                        ++length;
                    }
                } break;
                
                case 'u':
                case 'i':
                {
                    U64 num = 0;
                    
                    if (scan + 2 < (char*) format + format_length && scan[1] == '6' && scan[2] == '4')
                    {
                        num = va_arg(arg_list, U64);
                    }
                    
                    else
                    {
                        num = va_arg(arg_list, U32);
                    }
                    
                    if (*scan == 'i' && (I64) num < 0)
                    {
                        num = ~num + 1;
                        Append('-', &buffer, &buffer_capacity);
                        ++length;
                    }
                    
                    U64 num_cpy    = num / 10;
                    U64 multiplier = 1;
                    
                    while (num_cpy)
                    {
                        multiplier *= 10;
                        num_cpy    /= 10;
                    }
                    
                    while (multiplier)
                    {
                        U64 scaled_num = num / multiplier;
                        multiplier /= 10;
                        
                        U8 digit = scaled_num % 10;
                        
                        char c = '0' + digit;
                        
                        Append(c, &buffer, &buffer_capacity);
                        ++length;
                    }
                } break;
                
                case 'b':
                {
                    bool value = va_arg(arg_list, bool);
                    
                    if (value)
                    {
                        Append("true", &buffer, &buffer_capacity);
                        length += 4;
                    }
                    
                    else
                    {
                        Append("false", &buffer, &buffer_capacity);
                        length += 5;
                    }
                } break;
                
                case '%':
                {
                    Append('%', &buffer, &buffer_capacity);
                    ++length;
                } break;
                
                INVALID_DEFAULT_CASE;
            }
            
            ++scan;
        }
        
        else
        {
            Append(*scan, &buffer, &buffer_capacity);
            ++length;
            ++scan;
        }
    }
    
    return length;
}

inline UMM
FormatString(char* dest, UMM dest_capacity, const char* format, UMM format_length, ...)
{
    UMM result = 0;
    
    va_list arg_list;
    
    va_start(arg_list, &format_length);
    result = FormatString(dest, dest_capacity, format, format_length, arg_list);
    va_end(arg_list);
    
    return result;
}

inline UMM
FormatString(char* dest, UMM dest_capacity, const char* format, va_list arg_list)
{
    UMM result = 0;
    
    UMM format_length = CStringLength(format);
    
    result = FormatString(dest, dest_capacity, format, format_length, arg_list);
    
    return result;
}

inline UMM
FormatString(char* dest, UMM dest_capacity, const char* format, ...)
{
    UMM result = 0;
    
    va_list arg_list;
    
    UMM format_length = CStringLength(format);
    
    va_start(arg_list, &format);
    result = FormatString(dest, dest_capacity, format, format_length, arg_list);
    va_end(arg_list);
    
    return result;
}

inline UMM
FormatString(char* dest, UMM dest_capacity, String format, va_list arg_list)
{
    UMM result = 0;
    
    result = FormatString(dest, dest_capacity, (const char*) format.data, format.size, arg_list);
    
    return result;
}

inline UMM
FormatString(char* dest, UMM dest_capacity, String format, ...)
{
    UMM result = 0;
    
    va_list arg_list;
    
    va_start(arg_list, &format);
    result = FormatString(dest, dest_capacity, (const char*) format.data, format.size, arg_list);
    va_end(arg_list);
    
    return result;
}

inline bool
StringCompare(String s0, String s1)
{
    while (s0.size && s1.size && s0.data[0] == s1.data[0])
        --s0.size, ++s0.data, --s1.size, ++s1.data;
    
    return s0.size == s1.size && !s0.size;
}