/*
Copyright (c) 2015 Benjamin Moir <bennyboy.private@hotmail.com.au>
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

#include "lt.h"

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <iconv.h>
#include <stdlib.h>

/*
 * Variables
 */

static LT_GarbageList *gbHead, *gbRover;
static FILE *parseFile;
static LT_InitInfo info;
static iconv_t icDesc;
static bool assertError = false;
static char *assertString;
static char *stringChars = "\"", *charChars = "'";

static const char *errors[] = {
	"LT_Error: Syntax error",
	"LT_Error: Unknown operation",
	"LT_Error: Out of memory"
};

char *LT_TkNames[] = {
	// [marrub] So, this was an interesting bug. This was completely misordered from the enum.
	//          As can be guessed, this caused many issues. Seriously, all of them.
	"TOK_Colon",  "TOK_Comma",  "TOK_Div",    "TOK_Mod",    "TOK_Mul",    "TOK_Query",
	"TOK_BraceO", "TOK_BraceC", "TOK_BrackO", "TOK_BrackC", "TOK_ParenO", "TOK_ParenC",
	"TOK_LnEnd",  "TOK_Add2",   "TOK_Add",    "TOK_And2",   "TOK_And",    "TOK_CmpGE",
	"TOK_ShR",    "TOK_CmpGT",  "TOK_CmpLE",  "TOK_ShL",    "TOK_CmpNE",  "TOK_CmpLT",
	"TOK_CmpEQ",  "TOK_Equal",  "TOK_Not",    "TOK_OrI2",   "TOK_OrI",    "TOK_OrX2",
	"TOK_OrX",    "TOK_Sub2",   "TOK_Sub",    "TOK_String", "TOK_Charac", "TOK_Number",
	"TOK_Identi", "TOK_EOF",    "TOK_ChrSeq", "TOK_Comment","TOK_Period", "TOK_Arrow",
	"TOK_Sigil",  "TOK_Hash",   "TOK_BlkCmtO","TOK_BlkCmtC","TOK_Exp",    "TOK_NstCmtO",
	"TOK_NstCmtC"
};

/*
 * Functions
 */

static void LT_DoConvert(char **str)
{
	size_t i = strlen(*str);
	char *strbuf = calloc((i * 6) + 1, 1);
	char *strbufOrig = strbuf, *strOrig = *str;
	size_t in = i, out = i * 6;
	
	iconv(icDesc, str, &in, &strbuf, &out);
	
	*str = strOrig, strbuf = strbufOrig;
	
	free(*str);
	*str = strbuf;
}

static void *LT_Alloc(size_t size)
{
	void *p = malloc(size);
	
	if(p == NULL)
	{ // [marrub] if we don't error it will try to allocate an assertion, thus breaking horribly
		LT_Error(LTERR_NOMEMORY);
	}
	
	return p;
}

static void *LT_ReAlloc(void *ptr, size_t newSize)
{
	void *p = realloc(ptr, newSize);
	
	if(p == NULL)
	{
		LT_Error(LTERR_NOMEMORY);
	}
	
	return p;
}

static void *LT_SetGarbage(void *p)
{
	gbRover->next = LT_Alloc(sizeof(LT_GarbageList));
	gbRover = gbRover->next;
	gbRover->ptr = p;
	gbRover->next = NULL;
	
	return gbRover->ptr;
}

void LT_Init(LT_InitInfo initInfo)
{
	gbHead = LT_Alloc(sizeof(LT_GarbageList));
	gbHead->next = NULL;
	gbHead->ptr = NULL;
	
	gbRover = gbHead;
	
	info = initInfo;
	
	if(info.doConvert && info.fromCode != NULL && info.toCode != NULL)
	{
		icDesc = iconv_open(info.toCode, info.fromCode);
		
		if(icDesc == (iconv_t) -1)
		{
			LT_Assert(true, "LT_Init: Failure opening iconv");
		}
	}
	else
	{
		info.doConvert = false;
	}
	
	if(info.stripInvalid && info.doConvert)
	{
		info.stripInvalid = false;
	}
	
	if(info.stringChars != NULL)
	{
		int i;
		
		stringChars = LT_Alloc(6);
		
		for(i = 0; i < 6; i++)
		{
			int c = info.stringChars[i];
			
			if(c != '\0')
			{
				stringChars[i] = c;
			}
			else
			{
				break;
			}
		}
		
		stringChars[i] = '\0';
		
		LT_SetGarbage(stringChars);
	}
	
	if(info.charChars != NULL)
	{
		int i;
		
		charChars = LT_Alloc(6);
		
		for(i = 0; i < 6; i++)
		{
			int c = info.charChars[i];
			
			if(c != '\0')
			{
				charChars[i] = c;
			}
			else
			{
				break;
			}
		}
		
		charChars[i] = '\0';
		
		LT_SetGarbage(charChars);
	}
}

void LT_Quit()
{
	if(info.doConvert)
	{
		iconv_close(icDesc);
	}
	
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
		assertError = true;
		assertString = malloc(512);
		
		snprintf(assertString, 512, ":%ld:%s", ftell(parseFile), str);
		
		LT_SetGarbage(LT_ReAlloc(assertString, strlen(assertString) + 1));
	}
	
	return assertion;
}

void LT_Error(int type)
{
	fprintf(stderr, "%s", errors[type]);
	exit(1);
}

LT_AssertInfo LT_CheckAssert()
{
	LT_AssertInfo ltAssertion;
	ltAssertion.failure = assertError;
	ltAssertion.str = assertString;
	return ltAssertion;
}

bool LT_OpenFile(const char *filePath)
{
	parseFile = fopen(filePath, "r");
	
	if(parseFile == NULL)
	{
		char *errorStr = LT_Alloc(256);
		
		snprintf(errorStr, 256, "LT_OpenFile: %s", strerror(errno));
		LT_Assert(true, errorStr);
		
		return false;
	}
	
	return true;
}

void LT_CloseFile()
{
	if(parseFile != NULL)
	{
		fclose(parseFile);
	}
}

char *LT_ReadNumber()
{
	size_t i = 0, strBlocks = 1;
	char *str = LT_Alloc(TOKEN_STR_BLOCK_LENGTH);
	int c = '\0';
	
	while(c != EOF)
	{
		c = fgetc(parseFile);
		
		if(!isalnum(c))
		{
			ungetc(c, parseFile);
			break;
		}
		
		if(i > (TOKEN_STR_BLOCK_LENGTH * strBlocks))
		{
			str = LT_ReAlloc(str, TOKEN_STR_BLOCK_LENGTH * ++strBlocks);
		}
		
		str[i++] = c;
		
		if(info.stripInvalid)
		{
			str[i++] = (isspace(c) || isprint(c)) ? c : ' ';
		}
	}
	
	str[i++] = '\0';
	
	if(info.doConvert)
	{
		LT_DoConvert(&str);
	}
	
	return LT_SetGarbage(LT_ReAlloc(str, i));
}

char *LT_ReadString(char term)
{
	size_t i = 0, strBlocks = 1;
	char *str = LT_Alloc(TOKEN_STR_BLOCK_LENGTH);
	int c;
	
	while(true)
	{
		c = fgetc(parseFile);
		
		if(c == term)
		{
			break;
		}
		
		if(LT_Assert(c == EOF || c == '\n', "LT_ReadString: Unterminated string literal"))
		{
			char *emptyString = LT_Alloc(1);
			emptyString[0] = '\0';
			
			return LT_SetGarbage(emptyString);
		}
		
		if(c == '\\' && info.escapeChars)
		{
			c = fgetc(parseFile);
			
			if(LT_Assert(c == EOF || c == '\n', "LT_ReadString: Unterminated string literal"))
			{
				str[i] = '\0';
				return str;
			}
			
			if(i > (TOKEN_STR_BLOCK_LENGTH * strBlocks))
			{
				str = LT_ReAlloc(str, TOKEN_STR_BLOCK_LENGTH * ++strBlocks);
			}
			
			str = LT_Escaper(str, i++, c);
		}
		else
		{
			if(i > (TOKEN_STR_BLOCK_LENGTH * strBlocks))
			{
				str = LT_ReAlloc(str, TOKEN_STR_BLOCK_LENGTH * ++strBlocks);
			}
			
			str[i++] = c;
			
			if(info.stripInvalid)
			{
				str[i++] = (isspace(c) || isprint(c)) ? c : ' ';
			}
		}
	}
	
	str[i++] = '\0';
	
	if(info.doConvert)
	{
		LT_DoConvert(&str);
	}
	
	return LT_SetGarbage(LT_ReAlloc(str, i));
}

char *LT_Escaper(char *str, size_t pos, char escape)
{
	switch(escape)
	{
		case '\\': case '\'': case '"':  str[pos] = escape; break;
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
				int c = fgetc(parseFile);
				
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
						ungetc(c, parseFile);
						str[pos] = i;
						break;
				}
			}
			
			break;
		
		case '0': case '1': case '2': case '3':
		case '4': case '5': case '6': case '7':
			{
				int c = escape;
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
							ungetc(c, parseFile);
							str[pos] = i;
							return str;
					}
					
					c = fgetc(parseFile);
				}
				
				str[pos] = i;
				break;
			}
			
			break;
		default:
			LT_Assert(true, "LT_Escaper: Unknown escape character");
			break;
	}
	
	return str;
}

LT_Token LT_GetToken()
{
	LT_Token tk = { 0 };
	int c = fgetc(parseFile);
	
	if(c == EOF)
	{
		tk.token = LT_TkNames[TOK_EOF];
		tk.pos = ftell(parseFile);
		return tk;
	}
	
	while(isspace(c) && c != '\n')
	{
		c = fgetc(parseFile);
		
		if(c == EOF) // [marrub] This could have caused issues if there was whitespace before EOF.
		{
			tk.token = LT_TkNames[TOK_EOF];
			tk.pos = ftell(parseFile);
			return tk;
		}
	}
	
	tk.pos = ftell(parseFile);
	
	switch(c)
	{
	case '$':  tk.token = LT_TkNames[TOK_Sigil];  return tk;
	case '#':  tk.token = LT_TkNames[TOK_Hash];   return tk;
	case '.':  tk.token = LT_TkNames[TOK_Period]; return tk;
	case ':':  tk.token = LT_TkNames[TOK_Colon];  return tk;
	case ',':  tk.token = LT_TkNames[TOK_Comma];  return tk;
	case '%':  tk.token = LT_TkNames[TOK_Mod];    return tk;
	case '?':  tk.token = LT_TkNames[TOK_Query];  return tk;
	case '{':  tk.token = LT_TkNames[TOK_BraceO]; return tk;
	case '}':  tk.token = LT_TkNames[TOK_BraceC]; return tk;
	case '[':  tk.token = LT_TkNames[TOK_BrackO]; return tk;
	case ']':  tk.token = LT_TkNames[TOK_BrackC]; return tk;
	case '(':  tk.token = LT_TkNames[TOK_ParenO]; return tk;
	case ')':  tk.token = LT_TkNames[TOK_ParenC]; return tk;
	case '\n': tk.token = LT_TkNames[TOK_LnEnd];  return tk;
	
	// [marrub] Sorry, I wouldn't normally do a quick and dirty hack like this,
	//          but sometimes I really do care about my sanity. And wrists.
#define DoubleTokDef(ch, t1, t2) \
	case ch: \
		c = fgetc(parseFile); \
		\
		if(c == ch) \
		{ \
			tk.token = LT_TkNames[t2]; \
		} \
		else \
		{ \
			tk.token = LT_TkNames[t1]; \
			ungetc(c, parseFile); \
		} \
		\
		return tk;
	
	DoubleTokDef('&', TOK_And, TOK_And2);
	DoubleTokDef('=', TOK_Equal, TOK_CmpEQ);
	DoubleTokDef('^', TOK_OrX, TOK_OrX2);
	DoubleTokDef('|', TOK_OrI, TOK_OrI2);
	
#undef DoubleTokDef
	
	// [marrub] Special god damn snowflakes
	case '>':
		c = fgetc(parseFile);
		
		if(c == '=')
		{
			tk.token = LT_TkNames[TOK_CmpGE];
		}
		else if(c == '>')
		{
			tk.token = LT_TkNames[TOK_ShR];
		}
		else
		{
			tk.token = LT_TkNames[TOK_CmpGT];
			ungetc(c, parseFile);
		}
		
		return tk;
	case '<':
		c = fgetc(parseFile);
		
		if(c == '=')
		{
			tk.token = LT_TkNames[TOK_CmpLE];
		}
		else if(c == '<')
		{
			tk.token = LT_TkNames[TOK_ShL];
		}
		else if(c == '>')
		{
			tk.token = LT_TkNames[TOK_CmpNE];
		}
		else
		{
			tk.token = LT_TkNames[TOK_CmpLT];
			ungetc(c, parseFile);
		}
		
		return tk;
	case '!':
		c = fgetc(parseFile);
		
		if(c == '=')
		{
			tk.token = LT_TkNames[TOK_CmpNE];
		}
		else
		{
			tk.token = LT_TkNames[TOK_Not];
			ungetc(c, parseFile);
		}
		
		return tk;
	case '~':
		c = fgetc(parseFile);
		
		if(c == '=')
		{
			tk.token = LT_TkNames[TOK_CmpNE];
		}
		else
		{
			ungetc(c, parseFile);
			tk.token = LT_TkNames[TOK_ChrSeq];
			tk.string = LT_Alloc(2);
			tk.string[0] = c;
			tk.string[1] = '\0';
			
			LT_SetGarbage(tk.string);
		}
		
		return tk;
	// [zombie] extra tokens
	case '/':
		c = fgetc(parseFile);
		
		if(c == '/')
		{
			tk.token = LT_TkNames[TOK_Comment];
		}
		else if(c == '*')
		{
			tk.token = LT_TkNames[TOK_BlkCmtO];
		}
		else if(c == '+')
		{
			tk.token = LT_TkNames[TOK_NstCmtO];
		}
		else
		{
			tk.token = LT_TkNames[TOK_Div];
			ungetc(c, parseFile);
		}
		
		return tk;
	case '*':
		c = fgetc(parseFile);
		
		if(c == '/')
		{
			tk.token = LT_TkNames[TOK_BlkCmtC];
		}
		else if(c == '*')
		{
			tk.token = LT_TkNames[TOK_Exp];
		}
		else
		{
			tk.token = LT_TkNames[TOK_Mul];
			ungetc(c, parseFile);
		}
		
		return tk;
	case '-':
		c = fgetc(parseFile);
		
		if(c == '-')
		{
			tk.token = LT_TkNames[TOK_Sub2];
		}
		else if (c == '>')
		{
			tk.token = LT_TkNames[TOK_Arrow];
		}
		else
		{
			tk.token = LT_TkNames[TOK_Sub];
			ungetc(c, parseFile);
		}
		
		return tk;
	case '+':
		c = fgetc(parseFile);
		
		if (c == '/')
		{
			tk.token = LT_TkNames[TOK_NstCmtC];
		}
		else if (c == '+')
		{
			tk.token = LT_TkNames[TOK_Add2];
		}
		else
		{
			tk.token = LT_TkNames[TOK_Add];
			ungetc(c, parseFile);
		}
		
		return tk;
	}
	
	
	if(stringChars[0] != '\0')
	{
		for(size_t i = 0; i < 6;)
		{
			char cc = stringChars[i++];
			
			if(cc == '\0')
			{
				break;
			}
			else if(c == cc)
			{
				tk.token = LT_TkNames[TOK_String];
				tk.string = LT_ReadString(c);
				return tk;
			}
		}
	}
	
	if(charChars[0] != '\0')
	{
		for(size_t i = 0; i < 6;)
		{
			char cc = charChars[i++];
			
			if(cc == '\0')
			{
				break;
			}
			else if(c == cc)
			{
				tk.token = LT_TkNames[TOK_Charac];
				tk.string = LT_ReadString(c);
				return tk;
			}
		}
	}
	
	if(isdigit(c))
	{
		ungetc(c, parseFile);
		
		tk.token = LT_TkNames[TOK_Number];
		tk.string = LT_ReadNumber();
		return tk;
	}
	
	if(isalpha(c) || c == '_')
	{
		size_t i = 0, strBlocks = 1;
		char *str = LT_Alloc(TOKEN_STR_BLOCK_LENGTH);
		
		while(c != EOF && (isalnum(c) || c == '_'))
		{
			if(i > (TOKEN_STR_BLOCK_LENGTH * strBlocks))
			{
				str = LT_ReAlloc(str, TOKEN_STR_BLOCK_LENGTH * ++strBlocks);
			}
			
			str[i++] = c;
			
			c = fgetc(parseFile);
		}
		
		str[i++] = '\0'; // [marrub] Completely forgot this line earlier. Really screwed up everything.
		
		if(info.doConvert)
		{
			LT_DoConvert(&str);
		}
		
		ungetc(c, parseFile);
		
		tk.token = LT_TkNames[TOK_Identi];
		tk.string = LT_SetGarbage(LT_ReAlloc(str, i));
		return tk;
	}
	
	tk.token = LT_TkNames[TOK_ChrSeq];
	tk.string = LT_Alloc(2);
	tk.string[0] = c;
	tk.string[1] = '\0';
	
	LT_SetGarbage(tk.string);
	
	return tk;
}

