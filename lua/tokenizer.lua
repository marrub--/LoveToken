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
typedef struct
{
	bool escapeChars;
	bool stripInvalid;
	bool doConvert;
	const char *fromCode;
	const char *toCode;
	const char *stringChars;
	const char *charChars; // [marrub] heh
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

void LT_Init(LT_InitInfo initInfo);
void LT_Quit();
bool LT_Assert(bool assertion, const char *str);
LT_AssertInfo LT_CheckAssert();

bool LT_OpenFile(const char *filePath);
void LT_CloseFile();

char *LT_ReadNumber();
char *LT_ReadString(char term);
char *LT_Escaper(char *str, size_t pos, char escape);
LT_Token LT_GetToken();
]])

local pReturn

function tokenizer:init(initInfo, filePath)
	loveToken.LT_Init(initInfo)
	loveToken.LT_OpenFile(filePath)
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

function tokenizer:closeFile()
	loveToken.LT_CloseFile()
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

function tokenizer:readString(term)
	pReturn = loveToken.LT_ReadString(term)
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
	lt.pos = pReturn.pos
	if (pReturn.string ~= nil) then
		lt.string = ffi.string(pReturn.string)
	end
	return lt
end

return tokenizer
