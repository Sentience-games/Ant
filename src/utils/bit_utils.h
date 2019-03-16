#pragma once

// TODO(soimn): replace this with an intrisic
inline i8
GetFirstSetBit(u64 value)
{
    i8 result = -1;
    
    u64 copy = value;
    
    for (u64 i = 0; i < 64; ++i)
    {
        if ((copy & 0x1) != 0)
        {
            result = (i8) i;
            break;
        }
        
        copy >>= 1;
    }
    
    return result;
}

inline u64
FlipAround(u64 num)
{
    u64 result  = 0;
    u8 to_shift = 16;
    
    while(num)
    {
        result  |= num & 0x1;
        num    >>= 1;
        result <<= 1;
        --to_shift;
    }
    
    result <<= to_shift;
    
    return result;
}