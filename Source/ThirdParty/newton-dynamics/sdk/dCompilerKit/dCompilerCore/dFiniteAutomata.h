/* Copyright (c) <2003-2016> <Newton Game Dynamics>
* 
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
* 
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely
*/

#ifndef __dCompilerKit_h__
#define __dCompilerKit_h__

#ifndef _WIN32_WINNT            // Specifies that the minimum required platform is Windows Vista.
#define _WIN32_WINNT 0x0600     // Change this to the appropriate value to target other versions of Windows.
#endif


#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <tchar.h>
#include <crtdbg.h>

#include <dTree.h>
#include <dList.h>
#include <dCRC.h>



#ifndef DTRACE
	#ifdef _DEBUG
		inline void dExpandTraceMessage (const char *fmt, ...)
		{
			va_list v_args;
			char* const text = (char*) malloc (strlen (fmt) + 2048);

			text[0] = 0;
			va_start (v_args, fmt);     
			vsprintf(text, fmt, v_args);
			va_end (v_args);            

			OutputDebugStringA (text);

			free (text);
		}

		#define DTRACE(x)										\
		{														\
			dExpandTraceMessage x;								\
		}
	#else
		#define DTRACE(x)
	#endif
#endif


class dAutomataState;

class dFiniteAutomata
{
	public:
	dFiniteAutomata();
	virtual ~dFiniteAutomata();

	virtual dAutomataState* CreateState (int id) const;
	static int GetScapeChar (int symbol);
};

#endif


