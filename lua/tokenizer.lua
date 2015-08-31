--[[

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

--]]

local ffi = require("ffi")
local tokenizer = {}

local loveToken = ffi.load("LoveToken")
ffi.cdef([[
typedef int LT_BOOL;

typedef struct
{
	LT_BOOL escapeChars;
	LT_BOOL stripInvalid;
	LT_BOOL doConvert;
	const char *fromCode;
	const char *toCode;
	const char *stringChars;
	const char *charChars;
} LT_Config;

typedef struct
{
	const char *token;
	char *string;
	int strlen;
	int pos;
} LT_Token;

typedef struct
{
	LT_BOOL failure;
	const char *str;
} LT_AssertInfo;

void LT_Init(LT_Config initCfg);
void LT_SetConfig(LT_Config newCfg);
void LT_Quit(void);

LT_BOOL LT_Assert(LT_BOOL assertion, const char *fmt, ...);
LT_AssertInfo LT_CheckAssert(void);

LT_BOOL LT_OpenFile(const char *filePath);
LT_BOOL LT_OpenString(const char *str);
void LT_SetPos(int newPos);
void LT_CloseFile(void);
void LT_CloseString(void);

char *LT_ReadNumber(void);
void LT_ReadString(LT_Token *tk, char term);
char *LT_Escaper(char *str, size_t pos, char escape);
LT_Token LT_GetToken(void);
char *LT_ReadLiteral(void);
void LT_SkipWhite(void);
void LT_SkipWhite2(void);
]])

local pReturn

function tokenizer:init(initInfo, filePath)
	loveToken.LT_Init(initInfo)
	if (filePath ~= nil) then
		loveToken.LT_OpenFile(filePath)
	end
end

function tokenizer:assert(assertion, str)
	return loveToken.LT_Assert(assertion, str)
end

function tokenizer:checkError()
	ltAssertion = loveToken.LT_CheckAssert()
	
	if (ltAssertion.str == nil) then
		assert(not ltAssertion.failure, "unknown assertion")
	else
		assert(not ltAssertion.failure, ffi.string(ltAssertion.str))
	end
end

function tokenizer:openFile(filePath)
	pReturn = loveToken.LT_OpenFile(filePath)
	tokenizer:checkError()
	return pReturn
end

function tokenizer:openString(str)
	pReturn = loveToken.LT_OpenString(str)
	tokenizer:checkError()
	return pReturn
end

function tokenizer:closeFile()
	loveToken.LT_CloseFile()
end

function tokenizer:closeString()
	loveToken.LT_CloseString()
end

function tokenizer:quit()
	loveToken.LT_CloseFile()
	loveToken.LT_Quit()
end

function tokenizer:readNumber()
	pReturn = loveToken.LT_ReadNumber()
	tokenizer:checkError()
	return ffi.string(pReturn)
end

function tokenizer:readString(tk, term)
	pReturn = loveToken.LT_ReadString(tk, term)
	tokenizer:checkError()
	return ffi.string(pReturn)
end

function tokenizer:escaper(str, pos, escape)
	pReturn = loveToken.LT_Escaper(str, pos, escape)
	tokenizer:checkError()
	return ffi.string(pReturn)
end

function tokenizer:getToken()
	pReturn = loveToken.LT_GetToken()
	tokenizer:checkError()
	local lt = {}
	lt.token = ffi.string(pReturn.token)
	lt.string = pReturn.string
	lt.strlen = pReturn.strlen
	lt.pos = pReturn.pos
	if (pReturn.string ~= nil) then
		lt.string = ffi.string(pReturn.string)
	end
	return lt
end

function tokenizer:readLiteral()
	return ffi.string(loveToken.LT_ReadLiteral())
end

function tokenizer:setPos(newPos)
	loveToken.LT_SetPos(newPos)
	tokenizer:checkError()
end

function tokenizer:skipWhite()
	loveToken.LT_SkipWhite()
end

function tokenizer:skipWhite2()
	loveToken.LT_SkipWhite2()
end

return tokenizer
