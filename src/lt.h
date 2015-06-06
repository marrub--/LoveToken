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
#include <stdbool.h>

/*
 * Definitions
 */

// [marrub] This can be changed if you have either a lot of very
//          long strings, or a lot of very small strings, for optimization.
#define TOKEN_STR_BLOCK_LENGTH 512

// [marrub] When using in FFI, remove this from the declarations.
//          Also make sure to redefine this for cross-platform.
#define LT_EXPORT __declspec(dllexport)

enum
{
	TOK_Colon,  TOK_Comma,  TOK_Div,    TOK_Mod,    TOK_Mul,
	TOK_Query,  TOK_BraceO, TOK_BraceC, TOK_BrackO, TOK_BrackC,
	TOK_ParenO, TOK_ParenC, TOK_LnEnd,  TOK_Add2,   TOK_Add,
	TOK_And2,   TOK_And,    TOK_CmpGE,  TOK_ShR,    TOK_CmpGT,
	TOK_CmpLE,  TOK_ShL,    TOK_CmpNE,  TOK_CmpLT,  TOK_CmpEQ,
	TOK_Equal,  TOK_Not,    TOK_OrI2,   TOK_OrI,    TOK_OrX2,
	TOK_OrX,    TOK_Sub2,   TOK_Sub,    TOK_String, TOK_Charac,
	TOK_Number, TOK_Identi, TOK_EOF,    TOK_ChrSeq
};

enum
{
	LTERR_SYNTAX,
	LTERR_UNKNOWN_OPERATION
};

/*
 * Types
 */

typedef struct
{
	bool escapeChars;
	bool stripInvalid;
	bool doConvert;
	const char *fromCode;
	const char *toCode;
} LT_InitInfo;

typedef struct
{
	char *token;
	char *string;
	int pos;
} LT_Token;

typedef struct
{
	bool failure;
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

void LT_EXPORT LT_Init(LT_InitInfo initInfo);
void LT_EXPORT LT_Quit();
bool LT_EXPORT LT_Assert(bool assertion, const char *str);
LT_AssertInfo LT_EXPORT LT_CheckAssert();
void LT_EXPORT LT_Error(int type); // [marrub] C use ONLY

bool LT_EXPORT LT_OpenFile(const char *filePath);
void LT_EXPORT LT_CloseFile();

char *LT_EXPORT LT_ReadNumber();
char *LT_EXPORT LT_ReadString(char term);
char *LT_EXPORT LT_Escaper(char *str, size_t pos, char escape);
LT_Token LT_EXPORT LT_GetToken();

/*
 * Variables
 * Don't include these into FFI declarations.
 */

extern char *LT_EXPORT LT_TkNames[];

#endif
