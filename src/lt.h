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

#ifndef LOVETOKEN_LT_H
#define LOVETOKEN_LT_H

/*
 * Includes
 */

#include <stdlib.h>

/*
 * Definitions
 */

// [marrub] This can be changed if you have either a lot of very
//          long strings, or a lot of very small strings, for optimization.
#define TOKEN_STR_BLOCK_LENGTH 4096

// [marrub] When using in FFI, remove this from the declarations.
//          Also make sure to redefine this if your platform is not supported.
//          (OSX shouldn't need this at all)
#ifndef LT_NO_EXPORT
	#if defined(_MSC_VER)
		#define LT_DLLEXPORT __declspec(dllexport)
		#define LT_EXPORT
	#elif defined(_GCC)
		#define LT_EXPORT __attribute__((visibility("default")))
		#define LT_DLLEXPORT
	#else
		#define LT_EXPORT
		#define LT_DLLEXPORT
	#endif
#else
	#define LT_EXPORT
	#define LT_DLLEXPORT
#endif

#ifdef __GDCC__
	#define LT_NO_ICONV
#endif

#define LT_TRUE 1
#define LT_FALSE 0

enum
{
	TOK_Colon,  TOK_Comma,  TOK_Div,    TOK_Mod,     TOK_Mul,
	TOK_Query,  TOK_BraceO, TOK_BraceC, TOK_BrackO,  TOK_BrackC,
	TOK_ParenO, TOK_ParenC, TOK_LnEnd,  TOK_Add2,    TOK_Add,
	TOK_And2,   TOK_And,    TOK_CmpGE,  TOK_ShR,     TOK_CmpGT,
	TOK_CmpLE,  TOK_ShL,    TOK_CmpNE,  TOK_CmpLT,   TOK_CmpEQ,
	TOK_Equal,  TOK_Not,    TOK_OrI2,   TOK_OrI,     TOK_OrX2,
	TOK_OrX,    TOK_Sub2,   TOK_Sub,    TOK_String,  TOK_Charac,
	TOK_Number, TOK_Identi, TOK_EOF,    TOK_ChrSeq,  TOK_Comment,
	TOK_Period, TOK_Arrow,  TOK_Sigil,  TOK_Hash,    TOK_BlkCmtO,
	TOK_BlkCmtC,TOK_Exp,    TOK_NstCmtO,TOK_NstCmtC, TOK_Semicl
};

enum
{
	LTERR_SYNTAX,
	LTERR_UNKNOWN_OPERATION,
	LTERR_NOMEMORY
};

/*
 * Types
 */

typedef int LT_BOOL;

typedef struct
{
	LT_BOOL escapeChars;
	LT_BOOL stripInvalid;
#ifndef LT_NO_ICONV
	LT_BOOL doConvert;
	const char *fromCode;
	const char *toCode;
#endif
	const char *stringChars;
	const char *charChars;
} LT_Config;

typedef struct
{
	const char *token;
	char *string;
	unsigned strlen;
	int pos;
} LT_Token;

typedef struct
{
	LT_BOOL failure;
	const char *str;
} LT_AssertInfo;

typedef struct LT_GarbageList_s
{
	struct LT_GarbageList_s *next;
	void *ptr;
} LT_GarbageList; // [marrub] Don't include this into FFI declarations.

/*
 * Functions
 */

#ifdef __cplusplus
extern "C"
{
#endif

LT_DLLEXPORT void LT_EXPORT LT_Init(LT_Config initCfg);
LT_DLLEXPORT void LT_EXPORT LT_SetConfig(LT_Config newCfg);
LT_DLLEXPORT void LT_EXPORT LT_Quit(void);

LT_DLLEXPORT LT_BOOL LT_EXPORT LT_Assert(LT_BOOL assertion, const char *fmt, ...);
LT_DLLEXPORT void LT_EXPORT LT_Error(int type); // [marrub] C use ONLY
LT_DLLEXPORT LT_AssertInfo LT_EXPORT LT_CheckAssert(void);

#ifndef __GDCC__
LT_DLLEXPORT LT_BOOL LT_EXPORT LT_OpenFile(const char *filePath);
#else
LT_DLLEXPORT LT_BOOL LT_EXPORT LT_OpenFile(__str filePath);
#endif
LT_DLLEXPORT void LT_EXPORT LT_SetPos(int newPos);
LT_DLLEXPORT void LT_EXPORT LT_CloseFile(void);

LT_DLLEXPORT char *LT_EXPORT LT_ReadNumber(void);
LT_DLLEXPORT void LT_EXPORT LT_ReadString(LT_Token *tk, char term);
LT_DLLEXPORT char *LT_EXPORT LT_Escaper(char *str, size_t pos, char escape);
LT_DLLEXPORT LT_Token LT_EXPORT LT_GetToken(void);
LT_DLLEXPORT char *LT_EXPORT LT_ReadLiteral(void);
LT_DLLEXPORT void LT_EXPORT LT_SkipWhite(void);
LT_DLLEXPORT void LT_EXPORT LT_SkipWhite2(void);

#ifdef __cplusplus
}
#endif

/*
 * Variables
 * Don't include these into FFI declarations.
 */

#ifdef __cplusplus
extern "C" const char *LT_EXPORT LT_TkNames[];
#else
extern const char *LT_EXPORT LT_TkNames[];
#endif

#endif

