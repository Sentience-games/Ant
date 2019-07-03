#pragma once

#include "ant_shared.h"
#include "utils/string.h"

enum TOKENIZER_TOKEN_TYPE
{
    Token_Unknown = 0,
    
    Token_Identifier, // NOTE(soimn): alpha + special characters such as '_'
    Token_Operator,   // NOTE(soimn): arithmetic, comparative, logical, '%', '.', ','
    Token_Number,     // NOTE(soimn): integer or floating point
    Token_String,     // NOTE(soimn): '"' delimeted string
    
    Token_Equals,
    Token_IsEqual,
    Token_IsNotEqual,
    Token_And,
    Token_Or,
    
    Token_Brace,
    Token_Bracket,
    Token_Paren,
    
    Token_Colon,
    Token_Semicolon,
    
    Token_Comment,    // NOTE(soimn): any letter after a "//" and before '\n'
    Token_Whitespace,
    Token_EndOfLine,
    Token_EndOfStream,
};

struct Tokenizer
{
    U32 line_nr;
    U32 column_nr;
    
    String input;
    char peek[2];
};

struct Token
{
    Enum8(TOKENIZER_TOKEN_TYPE) type;
    
    char value_c;
    
    bool is_float;
    I32 value_i32;
    F32 value_f32;
    
    String string;
};

inline void
Refill(Tokenizer* tokenizer)
{
    tokenizer->peek[0] = 0;
    tokenizer->peek[1] = 0;
    
    if (tokenizer->input.size == 1)
    {
        tokenizer->peek[0] = tokenizer->input.data[0];
    }
    
    else if (tokenizer->input.size >= 2)
    {
        tokenizer->peek[0] = tokenizer->input.data[0];
        tokenizer->peek[1] = tokenizer->input.data[1];
    }
}

inline void
Advance(Tokenizer* tokenizer, U32 amount = 1)
{
    ++tokenizer->column_nr;
    
    if (tokenizer->input.size < amount)
    {
        tokenizer->input.size = 0;
        tokenizer->input.data = 0;
    }
    
    else
    {
        tokenizer->input.size -= amount;
        tokenizer->input.data += amount;
    }
    
    Refill(tokenizer);
}

inline Token
GetTokenRaw(Tokenizer* tokenizer)
{
    Token token = {};
    token.type = Token_Unknown;
    
    char c0 = tokenizer->peek[0];
    char c1 = tokenizer->peek[1];
    
    if (c0)
    {
        switch (c0)
        {
            case '\r':
            case '\n':
            {
                U32 count = 0;
                while (IsEndOfLine(tokenizer->peek[0]) && IsEndOfLine(tokenizer->peek[1]))
                {
                    if (tokenizer->peek[0] == '\n')
                    {
                        ++tokenizer->line_nr;
                        ++count;
                    }
                    
                    Advance(tokenizer);
                }
                
                if (tokenizer->peek[0] == '\n')
                {
                    ++tokenizer->line_nr;
                    ++count;
                }
                
                // TODO(soimn): This is a bit missleading. Find a better way to reset the column number, other 
                //              than overflowing the integer
                tokenizer->column_nr = (U32) -1;
                
                token.type = Token_EndOfLine;
                token.value_i32 = count;
            } break;
            
            case ':': token.type = Token_Colon;     break;
            case ';': token.type = Token_Semicolon; break;
            
            case '+':
            case '*':
            case '<':
            case '>':
            case '%':
            case '~':
            case '^':
            case ',':
            token.type = Token_Operator;
            token.value_c = c0;
            break;
            
            case '(':
            case ')':
            token.type = Token_Paren;
            token.value_c = c0;
            break;
            
            case '{':
            case '}':
            token.type = Token_Brace;
            token.value_c = c0;
            break;
            
            case '[':
            case ']':
            token.type = Token_Bracket;
            token.value_c = c0;
            break;
            
            case '/':
            {
                if (c1 == '/')
                {
                    Advance(tokenizer, 2);
                    
                    token.type = Token_String;
                    token.string.data = tokenizer->input.data;
                    
                    UMM size = 0;
                    
                    if (!IsEndOfLine(tokenizer->peek[0]))
                    {
                        ++size;
                        for (;;)
                        {
                            if (tokenizer->peek[1] && !IsEndOfLine(tokenizer->peek[1]))
                            {
                                Advance(tokenizer);
                                ++size;
                            }
                            
                            else break;
                        }
                    }
                    
                    token.string.size = size;
                }
                
                else
                {
                    token.type = Token_Operator;
                    token.value_c = c0;
                }
            } break;
            
            case '"':
            {
                token.type = Token_String;
                token.string.data = &tokenizer->input.data[1];
                
                Advance(tokenizer);
                
                while (tokenizer->peek[0] && tokenizer->peek[0] != '"')
                {
                    Advance(tokenizer);
                    ++token.string.size;
                }
                
                if (!tokenizer->peek[0])
                {
                    //// ERROR
                    token.type = Token_Unknown;
                }
                
            } break;
            
            default:
            {
                if (IsAlpha(c0) || c0 == '_')
                {
                    token.type = Token_Identifier;
                    token.string.data = tokenizer->input.data;
                    
                    UMM size = 1;
                    for (;;)
                    {
                        if (IsAlpha(tokenizer->peek[1]) || tokenizer->peek[1] == '_')
                        {
                            Advance(tokenizer);
                            ++size;
                        }
                        
                        else break;
                    }
                    
                    token.string.size = size;
                }
                
                else if (IsNumeric(c0) || c0 == '.' || c0 == '-')
                {
                    if ((c0 == '.' && !IsNumeric(c1)) || (c0 == '-' && !(c1 == '.' || IsNumeric(c1))))
                    {
                        token.type = Token_Operator;
                        token.value_c = c0;
                    }
                    
                    else
                    {
                        token.type = Token_Number;
                        
                        if (c0 == '0' && (c1 == 'x' ||  c1 == 'b'))
                        {
                            // HEX OR BINARY
                            
                            bool is_hex = (c1 == 'x');
                            
                            Advance(tokenizer, 2);
                            
                            String number = {};
                            number.data = tokenizer->input.data;
                            
                            if (IsNumeric(tokenizer->peek[0]) || (is_hex && IsHex(tokenizer->peek[0])))
                            {
                                UMM size = 1;
                                while (IsNumeric(tokenizer->peek[1]) || (is_hex && IsHex(tokenizer->peek[1])))
                                {
                                    ++size;
                                    Advance(tokenizer);
                                }
                                
                                number.size = size;
                            }
                            
                            I64 result = 0;
                            if (ParseInt(number, (is_hex ? 16 : 2), &result))
                            {
                                token.value_i32 = (I32) MAX(result, I32_MAX);
                                token.value_f32 = (F32) result;
                            }
                            
                            else
                            {
                                token.type = Token_Unknown;
                            }
                        }
                        
                        else
                        {
                            // FLOAT OR INT
                            I8 sign = 1;
                            if (c0 == '-')
                            {
                                sign = -1;
                                Advance(tokenizer);
                            }
                            
                            String number = {};
                            number.data = tokenizer->input.data;
                            
                            UMM size = 1;
                            while (IsNumeric(tokenizer->peek[1]) || tokenizer->peek[1] == '.')
                            {
                                ++size;
                                Advance(tokenizer);
                            }
                            
                            number.size = size;
                            
                            I64 result   = 0;
                            F32 result_f = 0.0f;
                            if (ParseInt(number, 10, &result))
                            {
                                token.value_i32 = (I32)(sign * MIN(result, I32_MAX));
                                token.value_f32 = (F32)(sign * result);
                            }
                            
                            else if (ParseFloat(number, &result_f))
                            {
                                token.value_i32 = (I32)(sign * MIN(result_f, I32_MAX));
                                token.value_f32 = (F32)(sign * result_f);
                                token.is_float  = true;
                            }
                            
                            else
                            {
                                // TODO(soimn): Error handling
                                token.type = Token_Unknown;
                            }
                        }
                    }
                }
                
                else if (IsWhitespace(c0))
                {
                    token.type = Token_Whitespace;
                    token.string.data = tokenizer->input.data;
                    
                    UMM size = 1;
                    for (;;)
                    {
                        if (IsWhitespace(tokenizer->peek[1]))
                        {
                            Advance(tokenizer);
                            ++size;
                        }
                        
                        else break;
                    }
                    
                    token.string.size = size;
                }
                
                else if (c0 == '!' && c1 == '=')
                {
                    token.type = Token_IsNotEqual;
                    Advance(tokenizer);
                }
                
                else if (c0 == '=')
                {
                    if (c1 == '=')
                    {
                        token.type = Token_IsEqual;
                        Advance(tokenizer);
                    }
                    
                    else
                    {
                        token.type = Token_Equals;
                    }
                }
                
                else if (c0 == '&')
                {
                    if (c1 == '&')
                    {
                        token.type = Token_And;
                        Advance(tokenizer);
                    }
                    
                    else
                    {
                        token.type = Token_Operator;
                        token.value_c = c0;
                    }
                }
                
                else if (c0 == '|')
                {
                    if (c1 == '|')
                    {
                        token.type = Token_Or;
                        Advance(tokenizer);
                    }
                    
                    else
                    {
                        token.type = Token_Operator;
                        token.value_c = c0;
                    }
                }
            } break;
        }
        
        Advance(tokenizer);
    }
    
    else
    {
        token.type = Token_EndOfStream;
    }
    
    return token;
}

inline Token
GetToken(Tokenizer* tokenizer)
{
    Token result = {};
    
    do
    {
        result = GetTokenRaw(tokenizer);
    } while (result.type == Token_Whitespace 
             || result.type == Token_Comment 
             || result.type == Token_EndOfLine);
    
    return result;
}

inline bool
RequireToken(Tokenizer* tokenizer, Enum8(TOKENIZER_TOKEN_TYPE) required_type)
{
    return GetToken(tokenizer).type == required_type;
}


inline Token
PeekToken(Tokenizer* tokenizer)
{
    Tokenizer temp_tokenizer = *tokenizer;
    Token result = GetToken(&temp_tokenizer);
    
    return result;
}

inline Tokenizer
Tokenize(String input, U32 initial_line = 0, U32 initial_column = 0)
{
    Tokenizer result = {};
    
    result.line_nr   = initial_line;
    result.column_nr = initial_column;
    
    result.input = input;
    Refill(&result);
    
    return result;
}