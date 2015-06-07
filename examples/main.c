// This file is placed under public domain.
#include "lt.h"
#include <stdio.h>

int main(int argc, char **argv)
{
	LT_Config initCfg = { 0 }; // we don't need to set any options here
	LT_Token tk;
	
	LT_OpenFile("a.txt");
	
	LT_Init(initCfg);
	
	while(tk.token != LT_TkNames[TOK_EOF])
	{
		LT_AssertInfo check;
		tk = LT_GetToken();
		check = LT_CheckAssert();
		
		if(check.failure)
		{
			printf("%s\n", check.str);
			break;
		}
		
		if(tk.string != NULL)
		{
			printf("%s\n", tk.string);
		}
		else
		{
			printf("%s\n", tk.token);
		}
	}
	
	LT_CloseFile();
	LT_Quit();
	
	return 0;
}
