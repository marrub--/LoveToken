// This file is placed under public domain.
#include "lt.h"
#include <ACS_Zandronum.h>
#include <stdio.h>

#define Print(...) \
	( \
		ACS_BeginPrint(), \
		__nprintf(__VA_ARGS__), \
		ACS_EndPrint() \
	)
#define Log(...) \
	( \
		ACS_BeginLog(), \
		__nprintf(__VA_ARGS__), \
		ACS_EndLog() \
	)

[[extern("ACS"), call("ScriptS"), script("Enter")]]
void ExampleRunScript()
{
	LT_Config initCfg = { 0 };
	LT_Init(initCfg);
	
	LT_OpenFile(s"EXAMPLESCRIPT");
	
	LT_Token tk = LT_GetToken();
	
	ACS_Delay(15);
	
	Print("Printing out the parsed tokens to the console in 3...");
	ACS_Delay(35);
	Print("2...");
	ACS_Delay(35);
	Print("1...");
	ACS_Delay(35);
	Print("Showtime!");
	ACS_Delay(35);
	
	while (tk.token != LT_TkNames[TOK_EOF])
	{
		if (tk.string != NULL)
		{
			Log("%s - %s", tk.token, tk.string);
		}
		else
		{
			Log(tk.token);
		}
		
		tk = LT_GetToken();
		
		ACS_Delay(1);
	}
	
	LT_Quit();
	LT_CloseFile();
}