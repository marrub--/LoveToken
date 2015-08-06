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
#include <stdlib.h>
#include <stdarg.h>

#ifdef __GDCC__
	#include <ACS_Zandronum.h>
#else
	#ifndef LT_NO_ICONV
		#include <iconv.h>
	#endif
#endif

#ifdef __GDCC__

// TODO: replace these with GDCC's new file function tables or whatever they're called

#define fopen LT_FOpen
#define ftell LT_FTell
#define fgetc LT_FGetC
#define ungetc LT_UnGetC
#define fseek LT_FSeek
#define fclose LT_FClose
#define FILE LT_File

typedef struct
{
	__str langId;
	__str data;
	int pos;
} LT_File;
#endif

/*
 * Variables
 */

static LT_GarbageList *gbHead, *gbRover;
static FILE *parseFile;
static LT_Config cfg;

#ifndef LT_NO_ICONV
static iconv_t icDesc;
#endif

static LT_BOOL assertError = LT_FALSE;
static char *assertString;
static char *stringChars = (char *)"\"", *charChars = (char *)"'";

static const char *errors[] = {
	"LT_Error: Syntax error",
	"LT_Error: Unknown operation",
	"LT_Error: Out of memory"
};

const char *LT_TkNames[] = {
	// [marrub] So, this was an interesting bug. This was completely misordered from the enum.
	//          As can be guessed, this caused many issues. Seriously, all of them.
	"TOK_Colon",   "TOK_Comma", "TOK_Div",    "TOK_Mod",    "TOK_Mul",    "TOK_Query",
	"TOK_BraceO",  "TOK_BraceC","TOK_BrackO", "TOK_BrackC", "TOK_ParenO", "TOK_ParenC",
	"TOK_LnEnd",   "TOK_Add2",  "TOK_Add",    "TOK_And2",   "TOK_And",    "TOK_CmpGE",
	"TOK_ShR",     "TOK_CmpGT", "TOK_CmpLE",  "TOK_ShL",    "TOK_CmpNE",  "TOK_CmpLT",
	"TOK_CmpEQ",   "TOK_Equal", "TOK_Not",    "TOK_OrI2",   "TOK_OrI",    "TOK_OrX2",
	"TOK_OrX",     "TOK_Sub2",  "TOK_Sub",    "TOK_String", "TOK_Charac", "TOK_Number",
	"TOK_Identi",  "TOK_EOF",   "TOK_ChrSeq", "TOK_Comment","TOK_Period", "TOK_Arrow",
	"TOK_Sigil",   "TOK_Hash",  "TOK_BlkCmtO","TOK_BlkCmtC","TOK_Exp",    "TOK_NstCmtO",
	"TOK_NstCmtC", "TOK_Semicl"
};

/*
 * Functions
 */

#ifndef LT_NO_ICONV
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
#endif

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
#ifndef __GDCC__
	gbRover->next = LT_Alloc(sizeof(LT_GarbageList));
	gbRover = gbRover->next;
	gbRover->ptr = p;
	gbRover->next = NULL;
	
	return gbRover->ptr;
#else
	return p;
#endif
}

#ifdef __GDCC__
#define StrParam(...) \
	( \
		ACS_BeginStrParam(), \
		__nprintf(__VA_ARGS__), \
		ACS_EndStrParam() \
	)
#define StrParamL(...) (StrParam("%LS", StrParam(__VA_ARGS__)))

LT_File *LT_FOpen(__str languageId, const char *mode)
{
	LT_File *file = LT_Alloc(sizeof(LT_File));
	
	file->langId = languageId;
	file->data = StrParamL("%S", languageId);
	file->pos = 0;
	
	return file;
}

int LT_FTell(LT_File *file)
{
	return file->pos;
}

int LT_FGetC(LT_File *file)
{
	int c = ACS_GetChar(file->data, file->pos++);
	return c < 1 ? EOF : c;
}

int LT_UnGetC(int ch, LT_File *file)
{
	int c = ACS_GetChar(file->data, file->pos--);
	return c < 1 ? EOF : c;
}

int LT_FSeek(LT_File *file, long int offset, int whence)
{
	switch(whence)
	{
	case SEEK_SET:
		file->pos = offset;
		return 0;
	case SEEK_CUR:
		file->pos += offset;
		return 0;
	case SEEK_END:
		file->pos = ACS_StrLen(file->data) + offset;
		return 0;
	}
	
	return 1;
}

int LT_FClose(LT_File *file)
{
	free(file);
	return 0;
}
#endif

void LT_Init(LT_Config initCfg)
{
#ifndef __GDCC__
	// [marrub] we don't need a garbage collector in GDCC
	gbHead = LT_Alloc(sizeof(LT_GarbageList));
	gbHead->next = NULL;
	gbHead->ptr = NULL;
	
	gbRover = gbHead;
#endif
	
	cfg = initCfg;
	
#ifndef LT_NO_ICONV
	if(cfg.doConvert && cfg.fromCode != NULL && cfg.toCode != NULL)
	{
		icDesc = iconv_open(cfg.toCode, cfg.fromCode);
		
		if(icDesc == (iconv_t) -1)
		{
			LT_Assert(LT_TRUE, "LT_Init: Failure opening iconv");
		}
	}
	else
	{
		cfg.doConvert = LT_FALSE;
	}
	
	if(cfg.stripInvalid && cfg.doConvert)
	{
		cfg.stripInvalid = LT_FALSE;
	}
#endif
	
	if(cfg.stringChars != NULL)
	{
		unsigned i;
		
		stringChars = LT_Alloc(6);
		
		for(i = 0; i < 6; i++)
		{
			int c = cfg.stringChars[i];
			
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
	
	if(cfg.charChars != NULL)
	{
		unsigned i;
		
		charChars = LT_Alloc(6);
		
		for(i = 0; i < 6; i++)
		{
			int c = cfg.charChars[i];
			
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

void LT_SetConfig(LT_Config newCfg)
{
	cfg = newCfg;
	
#ifndef LT_NO_ICONV
	if(cfg.doConvert && cfg.fromCode != NULL && cfg.toCode != NULL)
	{
		if(icDesc != NULL)
		{
			iconv_close(icDesc);
		}
		
		icDesc = iconv_open(cfg.toCode, cfg.fromCode);
		
		if(icDesc == (iconv_t) -1)
		{
			LT_Assert(LT_TRUE, "LT_Init: Failure opening iconv");
		}
	}
	else
	{
		if(icDesc != NULL)
		{
			iconv_close(icDesc);
			icDesc = NULL;
		}
		
		cfg.doConvert = LT_FALSE;
	}
	
	if(cfg.stripInvalid && cfg.doConvert)
	{
		cfg.stripInvalid = LT_FALSE;
	}
#endif
	
	if(cfg.stringChars != NULL)
	{
		unsigned i;
		
		stringChars = LT_Alloc(6);
		
		for(i = 0; i < 6; i++)
		{
			int c = cfg.stringChars[i];
			
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
	
	if(cfg.charChars != NULL)
	{
		unsigned i;
		
		charChars = LT_Alloc(6);
		
		for(i = 0; i < 6; i++)
		{
			int c = cfg.charChars[i];
			
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
#ifndef LT_NO_ICONV
	if(cfg.doConvert)
	{
		iconv_close(icDesc);
	}
#endif
	
#ifndef __GDCC__
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
#endif
}

LT_BOOL LT_Assert(LT_BOOL assertion, const char *fmt, ...)
{
	if(assertion)
	{
		char *asBuffer = LT_Alloc(512);
		int place = (int)ftell(parseFile);
		
		va_list va;
		assertError = LT_TRUE;
		assertString = malloc(512);
		
		va_start(va, fmt);
		vsprintf(asBuffer, fmt, va);
		va_end(va);
		
		sprintf(assertString, "(offset %d) %s", place, asBuffer);
		
		LT_SetGarbage(assertString);
		
		free(ftString);
		free(asBuffer);
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

#ifndef __GDCC__
LT_BOOL LT_OpenFile(const char *filePath)
#else
LT_BOOL LT_OpenFile(__str filePath)
#endif
{
	parseFile = fopen(filePath, "r");
	
	if(parseFile == NULL)
	{
		LT_Assert(LT_TRUE, "LT_OpenFile: %s", strerror(errno));
		return LT_FALSE;
	}
	
	return LT_TRUE;
}

void LT_SetPos(int newPos)
{
#ifndef __GDCC__
	if(fseek(parseFile, newPos, SEEK_SET) != 0)
	{
		LT_Assert(ferror(parseFile), "LT_SetPos: %s", strerror(errno));
	}
#else
	fseek(parseFile, newPos, SEEK_SET);
#endif
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
		
		if(cfg.stripInvalid)
		{
			str[i++] = (isspace(c) || isprint(c)) ? c : ' ';
		}
	}
	
	str[i++] = '\0';
	
#ifndef LT_NO_ICONV
	if(cfg.doConvert)
	{
		LT_DoConvert(&str);
	}
#endif
	
	return LT_SetGarbage(LT_ReAlloc(str, i));
}

char *LT_ReadString(char term)
{
	size_t i = 0, strBlocks = 1;
	char *str = LT_Alloc(TOKEN_STR_BLOCK_LENGTH);
	int c;
	
	while(LT_TRUE)
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
		
		if(c == '\\' && cfg.escapeChars)
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
			
			if(cfg.stripInvalid)
			{
				str[i++] = (isspace(c) || isprint(c)) ? c : ' ';
			}
		}
	}
	
	str[i++] = '\0';
	
#ifndef LT_NO_ICONV
	if(cfg.doConvert)
	{
		LT_DoConvert(&str);
	}
#endif
	
	return LT_SetGarbage(LT_ReAlloc(str, i));
}

char *LT_Escaper(char *str, size_t pos, char escape)
{
	unsigned i;
	
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
			for(i = 0;;)
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
				unsigned n;
				
				i = 0;
				
				for(n = 2; n != 0; n--)
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
			LT_Assert(LT_TRUE, "LT_Escaper: Unknown escape character '%c'", escape);
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
	case ';':  tk.token = LT_TkNames[TOK_Semicl]; return tk;
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
		unsigned i;
		
		for(i = 0; i < 6;)
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
		unsigned i;
		
		for(i = 0; i < 6;)
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
		
#ifndef LT_NO_ICONV
		if(cfg.doConvert)
		{
			LT_DoConvert(&str);
		}
#endif
		
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

void LT_SkipWhite()
{
	char c = fgetc(parseFile);
	
	while(isspace(c) && c != EOF)
	{
		c = fgetc(parseFile);
	}
	
	ungetc(c, parseFile);
}
