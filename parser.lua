--[[

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

--]]

local ffi = require("ffi")
local parser = {}

local loveToken = ffi.load("LoveToken")
ffi.cdef([[
typedef struct
{
	bool escapeChars;
} LT_InitInfo;

typedef struct
{
	char *token;
	char *string;
	int pos;
} LT_Token;

extern bool LT_AssertError;

void LT_Init(LT_InitInfo initInfo);
void LT_Quit();
bool LT_Assert(bool assertion, const char *str);

bool LT_OpenFile(const char *filePath);
void LT_CloseFile();

char *LT_ReadNumber();
char *LT_ReadString(char term);
char *LT_Escaper(char *str, size_t pos, char escape);
LT_Token LT_GetToken();
]])

function parser:init(initInfo, filePath)
	loveToken.LT_Init(initInfo)
	loveToken.LT_OpenFile(filePath)
end

function parser:assert(assertion, str)
	return loveToken.LT_Assert(assertion, str)
end

function parser:openFile(filePath)
	return loveToken.LT_OpenFile(filePath)
end

function parser:closeFile()
	loveToken.LT_CloseFile()
end

function parser:quit()
	loveToken.LT_CloseFile()
	loveToken.LT_Quit()
end

function parser:readNumber()
	return ffi.string(loveToken.LT_ReadNumber())
end

function parser:readString(term)
	return ffi.string(loveToken.LT_ReadString(term))
end

function parser:escaper(str, pos, escape)
	return ffi.string(loveToken.LT_Escaper(str, pos, escape))
end

function parser:getToken()
	return loveToken.LT_GetToken()
end

return parser
