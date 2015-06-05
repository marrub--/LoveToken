/*
Copyright (c) 2015 Benjamin Moir
Copyright (c) 2015 Marrub <marrub@greyserv.net>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdbool.h>
#include "lt.h"

FILE *LT_ParseFile;
bool LT_AssertError = false;
LT_GarbageList *gbHead, *gbRover;

static bool escapeChars = true;

static char *tokenTypes[] = {
	// [marrub] So, this was an interesting bug. This was completely misordered from the enum.
	//          As can be guessed, this caused many issues. Seriously, all of them.
	"TOK_Colon", "TOK_Comma", "TOK_Div", "TOK_Mod", "TOK_Mul", "TOK_Query",
	"TOK_BraceO", "TOK_BraceC", "TOK_BrackO", "TOK_BrackC", "TOK_ParenO", "TOK_ParenC",
	"TOK_LnEnd", "TOK_Add2", "TOK_Add", "TOK_And2", "TOK_And", "TOK_CmpGE",
	"TOK_ShR", "TOK_CmpGT", "TOK_CmpLE", "TOK_ShL", "TOK_CmpNE", "TOK_CmpLT",
	"TOK_CmpEQ", "TOK_Equal", "TOK_Not", "TOK_OrI2", "TOK_OrI", "TOK_OrX2",
	"TOK_OrX", "TOK_Sub2", "TOK_Sub", "TOK_String", "TOK_Charac", "TOK_Number",
	"TOK_Identi", "TOK_EOF", "TOK_ChrSeq"
};

void LT_Init(LT_InitInfo initInfo)
{
	escapeChars = initInfo.escapeChars;
	
	gbHead = malloc(sizeof(LT_GarbageList));
	gbHead->next = NULL;
	gbHead->ptr = NULL;
	
	gbRover = gbHead;
}

void LT_Quit()
{
	gbRover = gbHead;
	
	while(gbRover != NULL)
	{
		LT_GarbageList *next = gbRover->next;
		
		if(gbRover->ptr != NULL)
		{
			free(gbRover->ptr);
			gbRover->ptr = NULL;
		}
		
		free(gbRover);
		
		gbRover = next;
	}
	
	gbRover = NULL;
	gbHead = NULL;
}

bool LT_Assert(bool assertion, const char *str)
{
	if(assertion)
	{
		LT_AssertError = true;
		fprintf(stderr, "LT_Assert: %s", str);
	}
	
	return assertion;
}

bool LT_OpenFile(const char *filePath)
{
	LT_ParseFile = fopen(filePath, "r");
	
	if(LT_ParseFile == NULL)
	{
		perror("LT_OpenFile");
		return false;
	}
	
	return true;
}

void LT_CloseFile()
{
	if(LT_ParseFile != NULL)
	{
		fclose(LT_ParseFile);
	}
}

char *LT_ReadNumber()
{
	size_t i = 0, str_blocks = 1;
	char c, *str = malloc(TOKEN_STR_BLOCK_LENGTH);
	
	while(!feof(LT_ParseFile))
	{
		fread(&c, 1, 1, LT_ParseFile);
		
		if(!isalnum(c))
		{
			fseek(LT_ParseFile, -1, SEEK_CUR);
			break;
		}
		
		if(i > TOKEN_STR_BLOCK_LENGTH)
		{
			realloc(str, TOKEN_STR_BLOCK_LENGTH * str_blocks++);
		}
		
		str[i++] = c;
	}
	
	str[i++] = '\0';
	
	gbRover->next = malloc(sizeof(LT_GarbageList));
	gbRover = gbRover->next;
	gbRover->ptr = realloc(str, i);
	gbRover->next = NULL;
	
	return gbRover->ptr;
}

char *LT_ReadString(char term)
{
	size_t i = 0, str_blocks = 1;
	char c, *str = malloc(TOKEN_STR_BLOCK_LENGTH);
	static char *emptyString = "";
	
	while(true)
	{
		fread(&c, 1, 1, LT_ParseFile);
		
		if(c == term)
		{
			break;
		}
		
		if(LT_Assert(feof(LT_ParseFile) || c == '\n', "unterminated string literal"))
		{
			return emptyString;
		}
		
		if(c == '\\' && escapeChars)
		{
			fread(&c, 1, 1, LT_ParseFile);
			
			if(LT_Assert(feof(LT_ParseFile) || c == '\n', "unterminated string literal"))
			{
				str[i] = '\0';
				return str;
			}
			
			if(i > TOKEN_STR_BLOCK_LENGTH)
			{
				realloc(str, TOKEN_STR_BLOCK_LENGTH * str_blocks++);
			}
			
			str = LT_Escaper(str, i++, c);
		}
		else
		{
			if(i > TOKEN_STR_BLOCK_LENGTH)
			{
				realloc(str, TOKEN_STR_BLOCK_LENGTH * str_blocks++);
			}
			
			str[i++] = c;
		}
	}
	
	str[i++] = '\0';
	
	gbRover->next = malloc(sizeof(LT_GarbageList));
	gbRover = gbRover->next;
	gbRover->ptr = realloc(str, i);
	gbRover->next = NULL;
	
	return gbRover->ptr;
}

char *LT_Escaper(char *str, size_t pos, char escape)
{
	switch(escape)
	{
		case '\\': case '\'': case '"':  str[pos] = escape; break;
		case 'C': str[pos] = '\x1C'; break;
		case 'a': str[pos] = '\a'; break;
		case 'b': str[pos] = '\b'; break;
		case 'f': str[pos] = '\f'; break;
		case 'n': str[pos] = '\n'; break;
		case 'r': str[pos] = '\r'; break;
		case 't': str[pos] = '\t'; break;
		case 'v': str[pos] = '\v'; break;
		case 'x': // [marrub] THIS ONE IS FUN
			for(unsigned int i = 0;;)
			{
				char c;
				fread(&c, 1, 1, LT_ParseFile);
				
				switch(c)
				{
					case '0': i = i * 16 + 0x0; break;
					case '1': i = i * 16 + 0x1; break;
					case '2': i = i * 16 + 0x2; break;
					case '3': i = i * 16 + 0x3; break;
					case '4': i = i * 16 + 0x4; break;
					case '5': i = i * 16 + 0x5; break;
					case '6': i = i * 16 + 0x6; break;
					case '7': i = i * 16 + 0x7; break;
					case '8': i = i * 16 + 0x8; break;
					case '9': i = i * 16 + 0x9; break;
					case 'a': case 'A': i = i * 16 + 0xA; break;
					case 'b': case 'B': i = i * 16 + 0xB; break;
					case 'c': case 'C': i = i * 16 + 0xC; break;
					case 'd': case 'D': i = i * 16 + 0xD; break;
					case 'e': case 'E': i = i * 16 + 0xE; break;
					case 'f': case 'F': i = i * 16 + 0xF; break;
					
					default:
						fseek(LT_ParseFile, -1, SEEK_CUR);
						str[pos] = i;
						break;
				}
			}
			
			break;
		
		case '0': case '1': case '2': case '3':
		case '4': case '5': case '6': case '7':
			{
				char c = escape;
				unsigned int i = 0;
				
				for(unsigned int n = 2; n != 0; n--)
				{
					switch(c)
					{
						case '0': i = i * 8 + 00; break;
						case '1': i = i * 8 + 01; break;
						case '2': i = i * 8 + 02; break;
						case '3': i = i * 8 + 03; break;
						case '4': i = i * 8 + 04; break;
						case '5': i = i * 8 + 05; break;
						case '6': i = i * 8 + 06; break;
						case '7': i = i * 8 + 07; break;
						default:
							fseek(LT_ParseFile, -1, SEEK_CUR);
							str[pos] = i;
							return str;
					}
					
					fread(&c, 1, 1, LT_ParseFile);
				}
				
				str[pos] = i;
				break;
			}
			
			break;
		default: LT_Assert(true, "unknown escape character"); break;
	}
	
	return str;
}

LT_Token LT_GetToken()
{
	char c;
	LT_Token tk = { 0 };
	
	fread(&c, 1, 1, LT_ParseFile);
	
	if(feof(LT_ParseFile))
	{
		tk.token = tokenTypes[TOK_EOF];
		tk.string = NULL;
		tk.pos = ftell(LT_ParseFile);
		return tk;
	}
	
	while(isspace(c) && c != '\n')
	{
		fread(&c, 1, 1, LT_ParseFile);
		
		if(feof(LT_ParseFile)) // [marrub] This could have caused issues if there was whitespace before EOF.
		{
			tk.token = tokenTypes[TOK_EOF];
			tk.string = NULL;
			tk.pos = ftell(LT_ParseFile);
			return tk;
		}
	}
	
	tk.pos = ftell(LT_ParseFile);
	
	switch(c)
	{
	case ':':  tk.token = tokenTypes[TOK_Colon];  return tk;
	case ',':  tk.token = tokenTypes[TOK_Comma];  return tk;
	case '/':  tk.token = tokenTypes[TOK_Div];    return tk;
	case '%':  tk.token = tokenTypes[TOK_Mod];    return tk;
	case '*':  tk.token = tokenTypes[TOK_Mul];    return tk;
	case '?':  tk.token = tokenTypes[TOK_Query];  return tk;
	case '{':  tk.token = tokenTypes[TOK_BraceO]; return tk;
	case '}':  tk.token = tokenTypes[TOK_BraceC]; return tk;
	case '[':  tk.token = tokenTypes[TOK_BrackO]; return tk;
	case ']':  tk.token = tokenTypes[TOK_BrackC]; return tk;
	case '(':  tk.token = tokenTypes[TOK_ParenO]; return tk;
	case ')':  tk.token = tokenTypes[TOK_ParenC]; return tk;
	case '\n': tk.token = tokenTypes[TOK_LnEnd];  return tk;
	
	// [marrub] Sorry, I wouldn't normally do a quick and dirty hack like this,
	//          but sometimes I really do care about my sanity. And wrists.
#define DoubleTokDef(ch, t1, t2) \
	case ch: \
		fread(&c, 1, 1, LT_ParseFile); \
		\
		if(c == ch) \
		{ \
			tk.token = tokenTypes[t2]; \
		} \
		else \
		{ \
			tk.token = tokenTypes[t1]; \
			fseek(LT_ParseFile, -1, SEEK_CUR); \
		} \
		\
		return tk;
	
	DoubleTokDef('+', TOK_Add, TOK_Add2);
	DoubleTokDef('-', TOK_Sub, TOK_Sub2);
	DoubleTokDef('&', TOK_And, TOK_And2);
	DoubleTokDef('=', TOK_Equal, TOK_CmpEQ);
	DoubleTokDef('^', TOK_OrX, TOK_OrX2);
	DoubleTokDef('|', TOK_OrI, TOK_OrI2);
	
#undef DoubleTokDef
	
	// [marrub] Special god damn snowflakes
	case '>':
		fread(&c, 1, 1, LT_ParseFile);
		
		if(c == '=')
		{
			tk.token = tokenTypes[TOK_CmpGE];
		}
		else if(c == '>')
		{
			tk.token = tokenTypes[TOK_ShR];
		}
		else
		{
			tk.token = tokenTypes[TOK_CmpGT];
			fseek(LT_ParseFile, -1, SEEK_CUR);
		}
		
		return tk;
	case '<':
		fread(&c, 1, 1, LT_ParseFile);
		
		if(c == '=')
		{
			tk.token = tokenTypes[TOK_CmpLE];
		}
		else if(c == '<')
		{
			tk.token = tokenTypes[TOK_ShL];
		}
		else if(c == '>')
		{
			tk.token = tokenTypes[TOK_CmpNE];
		}
		else
		{
			tk.token = tokenTypes[TOK_CmpLT];
			fseek(LT_ParseFile, -1, SEEK_CUR);
		}
		
		return tk;
	case '!':
		fread(&c, 1, 1, LT_ParseFile);
		
		if(c == '=')
		{
			tk.token = tokenTypes[TOK_CmpNE];
		}
		else
		{
			tk.token = tokenTypes[TOK_Not];
			fseek(LT_ParseFile, -1, SEEK_CUR);
		}
		
		return tk;
	case '~':
		fread(&c, 1, 1, LT_ParseFile);
		
		if(c == '=')
		{
			tk.token = tokenTypes[TOK_CmpNE];
		}
		else
		{
			fseek(LT_ParseFile, -1, SEEK_CUR);
			LT_Assert(true, "syntax error"); // [marrub] Yet more error checking that was forgotten before.
		}
		
		return tk;
	case '"': case '\'':
		tk.string = LT_ReadString(c);
		
		if(c == '"')
		{
			tk.token = tokenTypes[TOK_String];
		}
		else
		{
			tk.token = tokenTypes[TOK_Charac];
		}
		
		return tk;
	}
	
	if(isdigit(c))
	{
		fseek(LT_ParseFile, -1, SEEK_CUR);
		
		tk.string = LT_ReadNumber();
		tk.token = tokenTypes[TOK_Number];
		return tk;
	}
	
	if(isalpha(c) || c == '_')
	{
		size_t i = 0, str_blocks = 1;
		char *str = malloc(TOKEN_STR_BLOCK_LENGTH);
		
		while(!(feof(LT_ParseFile)) && (isalnum(c) || c == '_'))
		{
			if(i > TOKEN_STR_BLOCK_LENGTH)
			{
				realloc(str, TOKEN_STR_BLOCK_LENGTH * str_blocks++);
			}
			
			str[i++] = c;
			fread(&c, 1, 1, LT_ParseFile);
		}
		
		str[i++] = '\0'; // [marrub] Completely forgot this line earlier. Really screwed up everything.
		
		gbRover->next = malloc(sizeof(LT_GarbageList));
		gbRover = gbRover->next;
		gbRover->ptr = realloc(str, i);
		gbRover->next = NULL;
		
		fseek(LT_ParseFile, -1, SEEK_CUR);
		
		tk.string = gbRover->ptr;
		tk.token = tokenTypes[TOK_Identi];
		return tk;
	}
	
	return tk;
}

