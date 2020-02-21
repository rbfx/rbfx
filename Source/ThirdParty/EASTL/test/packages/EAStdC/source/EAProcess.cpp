///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// This module defines functions for process spawning and query.
/////////////////////////////////////////////////////////////////////////////


#include <EAStdC/internal/Config.h>
#include <EAStdC/EAProcess.h>
#include <EAStdC/EAString.h>
#include <string.h>
#include <EAAssert/eaassert.h>

#if   defined(EA_PLATFORM_MICROSOFT)
	#pragma warning(push, 0)
	#include <Windows.h>
	#include <stdlib.h>
	#include <process.h>

	#if EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)
		#include <ShellAPI.h>
		#pragma warning(pop)

		#ifdef _MSC_VER
			#pragma comment(lib, "shell32.lib") // Required for shellapi calls.
		#endif
	#endif

#elif defined(EA_PLATFORM_UNIX)
	#include <sys/types.h>
	#if EASTDC_SYS_WAIT_H_AVAILABLE
		#include <sys/wait.h>
	#endif
	#include <unistd.h>
	#include <errno.h>
	#include <stdio.h>

#elif defined(EA_PLATFORM_SONY) && EA_SCEDBG_ENABLED
	#include <libdbg.h>
#endif


#if defined(EA_PLATFORM_APPLE)
	#include <mach-o/dyld.h>    // _NSGetExecutablePath
	#include <sys/syslimits.h>  // PATH_MAX
	#include <libgen.h>         // dirname

namespace EA
{
	namespace StdC
	{
		//Used to determine if a given path is a bundle extension
			const char* kBundleExtensions[] = {
			".app",
			".bundle",
			".plugin"
		};
	}
}
#endif


namespace EA
{

namespace StdC
{


// EASTDC_SETCURRENTPROCESSPATH_REQUIRED
//
// Defined as 0 or 1.
//
#ifndef EASTDC_SETCURRENTPROCESSPATH_REQUIRED
	#if defined(EA_PLATFORM_SONY) && defined(EA_PLATFORM_CONSOLE)
		#define EASTDC_SETCURRENTPROCESSPATH_REQUIRED 1
	#else
		#define EASTDC_SETCURRENTPROCESSPATH_REQUIRED 0
	#endif
#endif

#if EASTDC_SETCURRENTPROCESSPATH_REQUIRED
	static char gCurrentProcessPath[kMaxPathLength] = { 0 };
#endif


EASTDC_API void SetCurrentProcessPath(const char* pPath)
{
	#if EASTDC_SETCURRENTPROCESSPATH_REQUIRED
		Strlcpy(gCurrentProcessPath, pPath, EAArrayCount(gCurrentProcessPath));
	#else
		EA_UNUSED(pPath);
	#endif
}



#if defined(EA_PLATFORM_MICROSOFT) && EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP | EA_WINAPI_PARTITION_TV_APP | EA_WINAPI_PARTITION_TV_TITLE | EA_WINAPI_PARTITION_GAMES)

	// According to Microsoft documentation:
	//   The GetModuleFileName function retrieves the full path and file name
	//   for the file containing the specified module.
	//   If the function succeeds, the return value is the length of the string 
	//   copied to the buffer, in TCHARs. If the buffer is too small to hold the 
	//   module name, the string is truncated to the user-supplied capacity, and 
	//   the function returns that capacity.

	EASTDC_API size_t GetCurrentProcessPath(char16_t* pPath, int pathCapacity, int /*pathFlags*/)
	{
		EA_ASSERT(pathCapacity > 0); EA_UNUSED(pathCapacity);

		const DWORD dwResult = GetModuleFileNameW(NULL, reinterpret_cast<LPWSTR>(pPath), (DWORD)pathCapacity);

		if((dwResult != 0) && (dwResult < (DWORD)pathCapacity)) // If there wasn't an error and there was enough capacity...
			return (size_t)dwResult;

		pPath[0] = 0;
		return 0;
	}

	EASTDC_API size_t GetCurrentProcessPath(char* pPath, int pathCapacity, int pathFlags)
	{
		EA_ASSERT(pathCapacity > 0); EA_UNUSED(pathCapacity);

		// We cannot use GetModuleFileNameA here, because the text encoding of 
		// GetModuleFileNameA is arbitrary and in any case is usually not UTF8. 
		char16_t path16[kMaxPathLength];
		GetCurrentProcessPath(path16, EAArrayCount(path16), pathFlags);

		const int intendedStrlen = Strlcpy(pPath, path16, (size_t)pathCapacity);

		if((intendedStrlen >= 0) && (intendedStrlen < pathCapacity))
			return (size_t)intendedStrlen;

		pPath[0] = 0;
		return 0;
	}

	EASTDC_API size_t GetCurrentProcessDirectory(char16_t* pDirectory, int pathCapacity, int /*pathFlags*/)
	{
		EA_ASSERT(pathCapacity > 0); EA_UNUSED(pathCapacity);

		const DWORD dwResult = GetModuleFileNameW(NULL, reinterpret_cast<LPWSTR>(pDirectory), (DWORD)pathCapacity);

		if((dwResult != 0) && (dwResult < (DWORD)pathCapacity)) // If there wasn't an error and there was enough capacity...
		{
			DWORD dw;

			for(dw = dwResult; dw > 0; --dw)
			{
				if((pDirectory[dw - 1] != '/') && (pDirectory[dw - 1] != '\\'))
					pDirectory[dw - 1] = 0;
				else
					break;
			}

			return dw;
		}

		pDirectory[0] = 0;
		return 0;
	}

	EASTDC_API size_t GetCurrentProcessDirectory(char* pDirectory, int pathCapacity, int pathFlags)
	{
		EA_ASSERT(pathCapacity > 0); EA_UNUSED(pathCapacity);

		char16_t path16[kMaxDirectoryLength];
		GetCurrentProcessDirectory(path16, EAArrayCount(path16), pathFlags);

		const int intendedStrlen = Strlcpy(pDirectory, path16, (size_t)pathCapacity);

		if((intendedStrlen >= 0) && (intendedStrlen < pathCapacity))
			return (size_t)intendedStrlen;

		pDirectory[0] = 0;
		return 0;
	}

#elif defined(EA_PLATFORM_SONY) && EA_SCEDBG_ENABLED // Debug time only, as the following code would not be allowed for use in retail kits.

	// sceDbgGetExecutablePath
	// Gets the application file path. The path can only be obtained for applications run 
	// from the host PC (run by Visual Studio, Neighborhood or the -run command).

	EASTDC_API size_t GetCurrentProcessPath(char* pPath, int pathCapacity, int /*pathFlags*/)
	{
		EA_ASSERT(pathCapacity > 0);

		size_t result;

		// If the user has set the process path, use the setting instead of querying.
		if (gCurrentProcessPath[0])
		{
			result = Strlcpy(pPath, gCurrentProcessPath, pathCapacity);
		}
		else
		{
			result = (size_t)sceDbgGetExecutablePath(pPath, (size_t)(unsigned)pathCapacity);
		}

		if(result < (size_t)pathCapacity) // sceDbgGetExecutablePath returns the requires strlen.
		{
			return result;
		}
		else
		{
			pPath[0] = 0;
			return 0; // sceDbgGetExecutablePath always 0-terminates, even on too small capacity.
		}
	}

	EASTDC_API size_t GetCurrentProcessPath(char16_t* pPath, int pathCapacity, int pathFlags)
	{
		EA_ASSERT(pathCapacity > 0);

		char path8[kMaxPathLength];
		GetCurrentProcessPath(path8, EAArrayCount(path8), pathFlags);

		const int intendedStrlen = Strlcpy(pPath, path8, pathCapacity);

		if((intendedStrlen >= 0) && (intendedStrlen < pathCapacity)) // If successful (UTF8 to UTF16 conversions can theoretically fail, if the encoded chars are bad...
			return (size_t)intendedStrlen;

		pPath[0] = 0;
		return 0;
	}

	EASTDC_API size_t GetCurrentProcessDirectory(char* pDirectory, int pathCapacity, int /*pathFlags*/)
	{
		int32_t result;
		if (gCurrentProcessPath[0])
		{
			result = (int32_t)Strlcpy(pDirectory, gCurrentProcessPath, pathCapacity);
		}
		else
		{
			result = sceDbgGetExecutablePath(pDirectory, (size_t)(unsigned)pathCapacity);
		}

		if(result < pathCapacity) // sceDbgGetExecutablePath returns the requires strlen.
		{
			while (result > 0)
			{
				if (pDirectory[result - 1] == '/' || pDirectory[result - 1] == '\\')
					break;
					
				pDirectory[--result] = '\0';
			}

			return (size_t)(uint32_t)(result);
		}
		else
		{
			pDirectory[0] = 0;
			return 0; // sceDbgGetExecutablePath always 0-terminates, even on too small capacity.
		}
	}

	EASTDC_API size_t GetCurrentProcessDirectory(char16_t* pDirectory, int pathCapacity, int pathFlags)
	{
		EA_ASSERT(pathCapacity > 0);

		char path8[kMaxDirectoryLength];    // We don't have access to EAIO here.
		GetCurrentProcessDirectory(path8, EAArrayCount(path8), pathFlags);

		const int intendedStrlen = Strlcpy(pDirectory, path8, pathCapacity);

		if((intendedStrlen >= 0) && (intendedStrlen < pathCapacity)) // If successful (UTF8 to UTF16 conversions can theoretically fail, if the encoded chars are bad...
			return (size_t)intendedStrlen;

		pDirectory[0] = 0;
		return 0;
	}

#elif defined(EA_PLATFORM_APPLE)

	EASTDC_API size_t GetCurrentProcessPath(char16_t* pPath, int pathCapacity, int pathFlags)
	{
		EA_ASSERT(pathCapacity > 0); EA_UNUSED(pathCapacity);

		char path8[kMaxPathLength];
		GetCurrentProcessPath(path8, EAArrayCount(path8), pathFlags);

		const int intendedStrlen = Strlcpy(pPath, path8, pathCapacity);

		if((intendedStrlen >= 0) && (intendedStrlen < pathCapacity)) // If successful (UTF8 to UTF16 conversions can theoretically fail, if the encoded chars are bad...
			return (size_t)intendedStrlen;

		pPath[0] = 0;
		return 0;
	}
	
	static bool IsBundleFolder(char* pPath, int pathCapacity)
	{
		for(size_t i = 0; i < EAArrayCount(kBundleExtensions); i++)
		{
			if(Striend(pPath, kBundleExtensions[i]))
			{
				return true;
			}
		}
		return false;
	}
	
	// To consider: add a flag so user can specify if they want the path to the actual executable even if it is in a .extension
	// EG: /path/to/MyApp.extension or /path/to/MyApp.extension/MyExecutable
	// Currently /path/to/.extension is returned if it exists, otherwise it returns the executable path
	EASTDC_API size_t GetCurrentProcessPath(char* pPath, int pathCapacity, int pathFlags)
	{
		EA_ASSERT(pathCapacity > 0);

		// https://developer.apple.com/library/mac/#documentation/Darwin/Reference/ManPages/man3/dyld.3.html
		// http://lists.apple.com/archives/darwin-dev/2008/Dec/msg00037.html
		
		uint32_t capacityU32 = (uint32_t)pathCapacity;
		int result = _NSGetExecutablePath(pPath, &capacityU32); // Returns -1 and sets capacityU32 if the capacity is not enough.
	
		if(result == 0)
		{
			EA_ASSERT(pathCapacity >= kMaxPathLength);
			char absolutePath[PATH_MAX];

			if(realpath(pPath, absolutePath) != NULL) // Obtain canonicalized absolute pathname.
			{
				if(pathFlags & kPathFlagBundlePath)
				{
					// We recursively call dirname() until we find .extension
					char appPath[kMaxPathLength];
					EA::StdC::Strlcpy(appPath, absolutePath, kMaxPathLength);
					bool found = IsBundleFolder(appPath, kMaxPathLength);

					while(!found &&
							EA::StdC::Strncmp(appPath, ".", kMaxPathLength) != 0 &&
							EA::StdC::Strncmp(appPath, "/", kMaxPathLength) != 0)
					{
						EA::StdC::Strlcpy(appPath,dirname(appPath), kMaxPathLength);
						found = IsBundleFolder(appPath, kMaxPathLength);
					}
				
					if(found)
						EA::StdC::Strlcpy(pPath, appPath, pathCapacity);
					else // If not found, we use the original executable absolute path.
						EA::StdC::Strlcpy(pPath, absolutePath, pathCapacity);
				}
				else
					EA::StdC::Strlcpy(pPath, absolutePath, pathCapacity);

				return Strlen(pPath);
			}
		}
		
		pPath[0] = 0;
		return 0;
	}

	EASTDC_API size_t GetCurrentProcessDirectory(char16_t* pDirectory, int pathCapacity, int pathFlags)
	{
		EA_ASSERT(pathCapacity > 0); EA_UNUSED(pathCapacity);

		char path8[kMaxDirectoryLength];    // We don't have access to EAIO here.
		GetCurrentProcessDirectory(path8, EAArrayCount(path8), pathFlags);

		const int intendedStrlen = Strlcpy(pDirectory, path8, pathCapacity);

		if((intendedStrlen >= 0) && (intendedStrlen < pathCapacity)) // If successful (UTF8 to UTF16 conversions can theoretically fail, if the encoded chars are bad...
			return (size_t)intendedStrlen;

		pDirectory[0] = 0;
		return 0;
	}

	EASTDC_API size_t GetCurrentProcessDirectory(char* pDirectory, int pathCapacity, int pathFlags)
	{
		EA_ASSERT(pathCapacity > 0);

		char   pAppPath[kMaxPathLength];
		size_t n = GetCurrentProcessPath(pAppPath, kMaxPathLength, pathFlags);
		
		if(n > 0)
		{
			// argv[0]       pDirectory
			// --------------------------------------------------
			// ""      ->    ""          (Should never happen)
			// "/"     ->    "/"         (Should never happen)
			// "a"     ->    "a"         (Should never happen)
			// "/a/b"  ->    /a/"

			EA_COMPILETIME_ASSERT(kMaxDirectoryLength >= kMaxPathLength); // We assert this because argv[0] could concievably be as long as kMaxPathLength.
			const size_t intendedStrlen = Strlcpy(pDirectory, pAppPath, pathCapacity);

			if(intendedStrlen < (size_t)pathCapacity) // If succeeded...
			{
				for(char* p = pDirectory + intendedStrlen; p > pDirectory; --p)
				{
					if(p[-1] == '/')
					{
						p[0] = 0; // e.g. /aaa/bbb/ccc => /aaa/bbb/
						return (size_t)(p - pDirectory);
					}
				}

				// Alternative implementation which we should validate, as it's simpler:
				//char* p = strrchr(pDirectory, '/');
				//if(p) // This should usually (always?) be valid.
				//    *p = 0;   // e.g. /aaa/bbb/ccc => /aaa/bbb/

				return Strlen(pDirectory);
			}
		}
		
		pDirectory[0] = 0;
		return 0;
	}

#elif defined(EA_PLATFORM_LINUX)

	EASTDC_API size_t GetCurrentProcessPath(char* pPath, int pathCapacity, int /*pathFlags*/)
	{
		EA_ASSERT(pathCapacity > 0); EA_UNUSED(pathCapacity);

		ssize_t resultLen = readlink("/proc/self/exe", pPath, pathCapacity);
		if( resultLen != -1 )
		{
			ssize_t terminatorLocation = resultLen < (pathCapacity-1) ? resultLen : (pathCapacity-1);
			pPath[terminatorLocation] = '\0';
			return terminatorLocation;
		}
		else
		{
			pPath[0] = 0;
			return 0;
		}
	}

	EASTDC_API size_t GetCurrentProcessPath(char16_t* pPath, int pathCapacity, int pathFlags)
	{
		EA_ASSERT(pathCapacity > 0); EA_UNUSED(pathCapacity);

		char path8[kMaxPathLength];
		GetCurrentProcessPath(path8, EAArrayCount(path8), pathFlags);

		const int intendedStrlen = Strlcpy(pPath, path8, pathCapacity);

		if((intendedStrlen >= 0) && (intendedStrlen < pathCapacity)) // If successful (UTF8 to UTF16 conversions can theoretically fail, if the encoded chars are bad...
			return (size_t)intendedStrlen;

		pPath[0] = 0;
		return 0;
	}

	EASTDC_API size_t GetCurrentProcessDirectory(char* pDirectory, int pathCapacity, int /*pathFlags*/)
	{
		EA_ASSERT(pathCapacity > 0); EA_UNUSED(pathCapacity);

		ssize_t resultLen = readlink("/proc/self/exe", pDirectory, pathCapacity);
		if( resultLen != -1 )
		{
			for(int pos = resultLen; pos > 0; --pos)
			{
				if(pDirectory[pos - 1] != '/')
					pDirectory[pos - 1] = 0;
				else
					break;
			}
			return strlen(pDirectory);
		}
		else
		{
			pDirectory[0] = 0;
			return 0;
		}
	}

	EASTDC_API size_t GetCurrentProcessDirectory(char16_t* pDirectory, int pathCapacity, int pathFlags)
	{
		EA_ASSERT(pathCapacity > 0); EA_UNUSED(pathCapacity);

		char path8[kMaxDirectoryLength];    // We don't have access to EAIO here.
		GetCurrentProcessDirectory(path8, EAArrayCount(path8), pathFlags);

		const int intendedStrlen = Strlcpy(pDirectory, path8, pathCapacity);

		if((intendedStrlen >= 0) && (intendedStrlen < pathCapacity)) // If successful (UTF8 to UTF16 conversions can theoretically fail, if the encoded chars are bad...
			return (size_t)intendedStrlen;

		pDirectory[0] = 0;
		return 0;
	}

	#if 0
	/*
		// http://blog.linuxgamepublishing.com/2009/10/12/argv-and-argc-and-just-how-to-get-them/
		// http://stackoverflow.com/questions/1585989/how-to-parse-proc-pid-cmdline
		// https://www.google.com/search?q=%2Fproc%2Fself%2Fcmdline

		char** get_argv()
		{
			static char   emptyNonConstString[1][1] = { { 0 } };
			static char** savedArgv = NULL;
		
			if(!savedArgv)
			{
				FILE* pFile = fopen("/proc/self/cmdline", "r");
		
				if(pFile) // This should be true for at least all recent Linux versions.
				{
					const  size_t kBufferSize = 1024; // This should be dynamically allocated if we are to be able to ready any buffer.
					char   buffer[kBufferSize];
					size_t count = fread(buffer, 1, kBufferSize, pFile);
		
					if(ferror(pFile) == 0) // If succeeded...
					{
						buffer[kBufferSize - 1] = 0;
		 
						// To do.
						// buffer has an arbitrary number of 0-terminated strings layed one after another.
						// Allocate an array of char pointers or use a static array of arrays. We can simply copy buffer to a permanent buffer and index its strings.
						// Need to free an allocated array on shutdown.
						// Check to make sure that the strlen wasn't too long.
					}
					fclose(pFile);   
				}
			}
		
			savedArgv = emptyNonConstString;
			return savedArgv;
		}
	*/
	#endif

/*
#elif defined(EA_PLATFORM_BSD) || (defined(EA_PLATFORM_SONY) && (defined(EA_PROCESSOR_X86) || defined(EA_PROCESSOR_X86_64)))
	Need to make this debug-only for proprietary platforms.
	// A way to read the current process path:
	// http://linux.die.net/man/2/readlink

	#if defined(EA_PLATFORM_SONY)
		ssize_t readlink(char *path, char *buf, size_t count)
		{
			int result;
			__asm__ __volatile__( 
				"mov %%rcx, %%r10\n\t"
				"syscall\n\t"
				: "=a"(result) : "a"(58), "D"(path), "S"(buf), "d"(count));
			return result;
		}
	#endif

	char buf[1024];
	char buff[1024];
	sprintf(buff, "/dev/%d/file", getpid());
	size_t s = readlink(buff, buf, sizeof(buf));
*/

#else

	EASTDC_API size_t GetCurrentProcessPath(char16_t* pPath, int pathCapacity, int pathFlags)
	{
		EA_ASSERT(pathCapacity > 0);

		char path8[kMaxPathLength];
		GetCurrentProcessPath(path8, EAArrayCount(path8), pathFlags);

		const int intendedStrlen = Strlcpy(pPath, path8, (size_t)pathCapacity);

		if((intendedStrlen >= 0) && (intendedStrlen < pathCapacity)) // If successful (UTF8 to UTF16 conversions can theoretically fail, if the encoded chars are bad)...
			return (size_t)intendedStrlen;

		pPath[0] = 0;
		return 0;
	}

	EASTDC_API size_t GetCurrentProcessPath(char* pPath, int pathCapacity, int /*pathFlags*/)
	{
		EA_ASSERT(pathCapacity > 0);

		#if EASTDC_SETCURRENTPROCESSPATH_REQUIRED
			const size_t intendedStrlen = Strlcpy(pPath, gCurrentProcessPath, pathCapacity);

			if(intendedStrlen < (size_t)pathCapacity) // If it completely fit...
				return intendedStrlen;
		#else
			EA_UNUSED(pathCapacity);
		#endif

		pPath[0] = 0;
		return 0;
	}

	EASTDC_API size_t GetCurrentProcessDirectory(char16_t* pDirectory, int pathCapacity, int pathFlags)
	{
		char dir8[kMaxDirectoryLength];
		GetCurrentProcessDirectory(dir8, EAArrayCount(dir8), pathFlags);

		const int intendedStrlen = Strlcpy(pDirectory, dir8, (size_t)pathCapacity);

		if((intendedStrlen >= 0) && (intendedStrlen < pathCapacity)) // If successful (UTF8 to UTF16 conversions can theoretically fail, if the encoded chars are bad)...
			return (size_t)intendedStrlen;

		pDirectory[0] = 0;
		return 0;
	}

	EASTDC_API size_t GetCurrentProcessDirectory(char* pDirectory, int pathCapacity, int pathFlags)
	{
		EA_ASSERT(pathCapacity > 0);

		size_t len = GetCurrentProcessPath(pDirectory, pathCapacity, pathFlags);

		if(len > 0)
		{
			for(int pos = (int)len; pos > 0; --pos)
			{
				// We make a broad assumption that both / and \ are directory separators. On a number of unsual platforms
				// we deal with, / is the norm but \ can still be used. e.g. /host/C:\SomeDir\SomeFile.txt
				if((pDirectory[pos - 1] != '/') && (pDirectory[pos - 1] != '\\'))
					pDirectory[pos - 1] = 0;
				else
					break;
			}

			return Strlen(pDirectory);
		}

		pDirectory[0] = 0;
		return 0;
	}

#endif


// The 32 bit versions of GetCurrentProcessPath and GetCurrentProcessDirectory are the same generic
// versions for all platforms, as they just route to using the platform-specific versions.
EASTDC_API size_t GetCurrentProcessPath(char32_t* pPath, int pathCapacity, int pathFlags)
{
	EA_ASSERT(pathCapacity > 0); EA_UNUSED(pathCapacity);

	char path8[kMaxPathLength];
	GetCurrentProcessPath(path8, EAArrayCount(path8), pathFlags);

	const int intendedStrlen = Strlcpy(pPath, path8, (size_t)pathCapacity);

	if((intendedStrlen >= 0) && (intendedStrlen < pathCapacity)) // If successful...
		return (size_t)intendedStrlen;

	pPath[0] = 0;
	return 0;
}

EASTDC_API size_t GetCurrentProcessDirectory(char32_t* pDirectory, int pathCapacity, int pathFlags)
{
	EA_ASSERT(pathCapacity > 0); EA_UNUSED(pathCapacity);

	char path8[kMaxDirectoryLength];    // We don't have access to EAIO here.
	GetCurrentProcessDirectory(path8, EAArrayCount(path8), pathFlags);

	const int intendedStrlen = Strlcpy(pDirectory, path8, (size_t)pathCapacity);

	if((intendedStrlen >= 0) && (intendedStrlen < pathCapacity)) // If successful...
		return (size_t)intendedStrlen;

	pDirectory[0] = 0;
	return 0;
}






EASTDC_API size_t GetEnvironmentVar(const char16_t* pName, char16_t* pValue, size_t valueCapacity)
{
	#if defined(EA_PLATFORM_WINDOWS) && EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)
		DWORD dwLength = GetEnvironmentVariableW(reinterpret_cast<const wchar_t *>(pName), reinterpret_cast<LPWSTR>(pValue), (DWORD)valueCapacity);

		if(dwLength == 0)
		{
			if(GetLastError() == ERROR_ENVVAR_NOT_FOUND)
				return (size_t)-1;
		}
		else if(dwLength > valueCapacity)
			dwLength -= 1; // On insufficient capacity, Windows returns the required capacity.

		return (size_t)dwLength;

	#else
		char name8[260];    
		char value8[260];    

		Strlcpy(name8, pName, 260);
		const size_t len = GetEnvironmentVar(name8, value8, 260);

		if(len < 260)
			return (size_t)Strlcpy(pValue, value8, valueCapacity, len);

		return len; // Note that the len here is for UTF8 chars, but the user is asking for 16 bit chars. So the returned len may be higher than the actual required len.
	#endif
}


EASTDC_API size_t GetEnvironmentVar(const char* pName, char* pValue, size_t valueCapacity)
{
	#if defined(EA_PLATFORM_WINDOWS) && EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)
		DWORD dwLength = GetEnvironmentVariableA(pName, pValue, (DWORD)valueCapacity);

		if(dwLength == 0)
		{
			if(GetLastError() == ERROR_ENVVAR_NOT_FOUND)
				return (size_t)-1;
		}
		else if(dwLength > valueCapacity)
			dwLength -= 1; // On insufficient capacity, Windows returns the required capacity.

		return (size_t)dwLength;

	#elif defined(EA_PLATFORM_UNIX)
		const char* const var = getenv(pName);
		if (var)
			return Strlcpy(pValue, var, valueCapacity);
		return (size_t)-1;
	#else
		// To consider: Implement this manually for the given platform.
		// Environment variables are application globals and so we probably need to use our OSGlobal to implement this.
		EA_UNUSED(pName);
		EA_UNUSED(pValue);
		EA_UNUSED(valueCapacity);
		return (size_t)-1;
	#endif
}


EASTDC_API bool SetEnvironmentVar(const char16_t* pName, const char16_t* pValue)
{
	#if defined(EA_PLATFORM_WINDOWS) && EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)
		const BOOL bResult = SetEnvironmentVariableW(reinterpret_cast<const wchar_t*>(pName), reinterpret_cast<const wchar_t*>(pValue)); // Windows has the same behavior as us: NULL pValue removes the variable.
		return (bResult != 0);
	#else
		char name8[260];
		Strlcpy(name8, pName, 260);

		char value8[260];
		Strlcpy(value8, pValue, 260);

		return SetEnvironmentVar(name8, value8);
	#endif
}


EASTDC_API bool SetEnvironmentVar(const char* pName, const char* pValue)
{
	#if defined(EA_PLATFORM_WINDOWS) && EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)
		const BOOL bResult = SetEnvironmentVariableA(pName, pValue); // Windows has the same behavior as us: NULL pValue removes the variable.
		return (bResult != 0);
	#elif defined(EA_PLATFORM_UNIX)
		// The opinion of the Linux people is that you just shouldn't ever call setenv during application runtime.
		// A better solution for us is to use shared mapped memory (shm_open(), mmap()): http://www.ibm.com/developerworks/aix/library/au-spunix_sharedmemory/index.html
		if(pValue)
			return setenv(pName, pValue, 1) == 0;
		else
			return unsetenv(pName) == 0;
	#else
		// To consider: Implement this manually for the given platform.
		// Environment variables are application globals and so we probably need to use our OSGlobal to implement this.
		// The easiest way for us to implement this is with an stl/eastl map. But we don't currently have access to those
		// from this package. So we are currently stuck using something simpler, like a key=value;key=value;key=value... string.
		EA_UNUSED(pName);
		EA_UNUSED(pValue);
		return false;
	#endif
}




EASTDC_API int Spawn(const char16_t* pPath, const char16_t* const* pArgumentArray, bool wait)
{
	#if defined(EA_PLATFORM_WINDOWS) && EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)
		return int(_wspawnv(wait ? _P_WAIT : _P_DETACH, reinterpret_cast<const wchar_t *>(pPath), reinterpret_cast<const wchar_t* const *>(pArgumentArray)));
	#else
		EA_UNUSED(pPath);
		EA_UNUSED(pArgumentArray);
		EA_UNUSED(wait);

		// TODO: convert and call char version
		EA_FAIL_MESSAGE("Spawn: Not implemented for this platform.");
		return -1;
	#endif
}


EASTDC_API int Spawn(const char* pPath, const char* const* pArgumentArray, bool wait)
{
	#if defined(EA_PLATFORM_WINDOWS) && EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)
		if(wait)
			return int(_spawnv(_P_WAIT, pPath, pArgumentArray));
		else
			return int(_spawnv(_P_DETACH, pPath, pArgumentArray));

	#elif defined(EA_PLATFORM_UNIX) && EASTDC_SYS_WAIT_H_AVAILABLE
		pid_t id = fork();

		if(id == 0) // child
		{
			//int result = 
			execv(pPath, (char* const*)pArgumentArray);
			exit(errno);
		}

		if(wait)
		{
			int status;
			waitpid(id, &status, 0); // waitpid() is safer than wait(), and seems currently be available on all OSs that matter to us.

			if(WIFEXITED(status))
				return WEXITSTATUS(status); // exit value of child

			// the child was killed due to a signal. we could find out
			// which signal if we wanted, but we're not really doing unix signals.
			return -1;
		}
		return 0;

	#else
		EA_UNUSED(pPath);
		EA_UNUSED(pArgumentArray);
		EA_UNUSED(wait);

		EA_FAIL_MESSAGE("Spawn: Not implemented for this platform.");
		return -1;
	#endif
}


EASTDC_API int ExecuteShellCommand(const char16_t* pCommand)
{
	#if defined(EA_PLATFORM_WINDOWS) && EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)
		// Todo: verify that newlines work here and support them if not.
		return _wsystem(reinterpret_cast<const wchar_t*>(pCommand)); // We could do this via the shell api as well.
	#else
		char command8[260];   
		Strlcpy(command8, pCommand, 260);

		return ExecuteShellCommand(command8);
	#endif
}


int ExecuteShellCommand(const char* pCommand)
{
	#if defined(EA_PLATFORM_WINDOWS) && EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)
		// Todo: verify that newlines work here and support them if not.
		return system(pCommand); // We could do this via the shell api as well.
	#elif defined(EA_PLATFORM_UNIX)
		return system(pCommand);
	#else
		EA_UNUSED(pCommand);
		return false;
	#endif
}



#if defined(DISABLED_____EA_PLATFORM_UNIX) // Need to implement this in a way that doesn't use EASTL or an allocator.
	EASTDC_API bool SearchEnvPathWithMode(const char* pathListVar, const char* fileName, int mode, eastl::string8* fullPath)
	{
		if (*fileName == '/' || *fileName == '\\')
		{
			fullPath->assign(fileName);
			return access(fileName, F_OK) == 0;
		}

		const char* pathList = getenv(pathListVar);

		if (pathList)
		{
			const char* pathStart = pathList;
			const char* pathEnd   = pathStart;

			while (true)
			{
				while ((*pathEnd != ':') && (*pathEnd != 0))
					++pathEnd;

				if (pathEnd > pathStart)
				{
					fullPath->assign(pathStart, pathEnd - pathStart);

					if ((*pathEnd != '/') && (*pathEnd != '\\'))
						*fullPath += '/';

					*fullPath += fileName;

					if (access(fullPath->c_str(), F_OK) == 0)
						return true;
				}

				if (*pathEnd == 0)  // end explicitly so we don't access outside pathList.
					break;

				pathEnd++;
				pathStart = pathEnd;
			}          
		}

		return false;
	}

#endif // EA_PLATFORM_UNIX



EASTDC_API bool SearchEnvironmentPath(const char16_t* pFileName, char16_t* pPath, const char16_t* pEnvironmentVar)
{
	#if defined(EA_PLATFORM_WINDOWS) && EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)
		if(!pEnvironmentVar)
			pEnvironmentVar = EA_CHAR16("PATH");
		_wsearchenv(reinterpret_cast<const wchar_t*>(pFileName), reinterpret_cast<const wchar_t*>(pEnvironmentVar), reinterpret_cast<wchar_t*>(pPath));
		return (*pPath != 0);

	#else 
		char path8    [260]; 
		char fileName8[260]; 

		Strlcpy(path8,     pPath,     260);
		Strlcpy(fileName8, pFileName, 260);

		bool success;

		if (pEnvironmentVar)
		{
			char environmentVariable8[260]; 
			Strlcpy(environmentVariable8, pEnvironmentVar, 260);

			success = EA::StdC::SearchEnvironmentPath(fileName8, path8, environmentVariable8);
		}
		else
			success = EA::StdC::SearchEnvironmentPath(fileName8, path8);

		Strlcpy(pPath, path8, 260);
		return success;
	#endif
}


EASTDC_API bool SearchEnvironmentPath(const char* pFileName, char* pPath, const char* pEnvironmentVar)
{
	#if defined(EA_PLATFORM_WINDOWS) && EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)
		if(!pEnvironmentVar)
			pEnvironmentVar = "PATH";
		_searchenv(pFileName, pEnvironmentVar, pPath);
		return (*pPath != 0);

	#elif defined(DISABLED_____EA_PLATFORM_UNIX) // Need to implement this in a way that doesn't use EASTL or an allocator.
		eastl::string8 path8(EASTLAllocatorType(UTFFOUNDATION_ALLOC_PREFIX "EAProcess"));
		bool success;

		if (pEnvironmentVar)
			success = SearchEnvPathWithMode(pEnvironmentVar, pFileName, F_OK, &path8); // Just require existence.
		else
			success = SearchEnvPathWithMode("PATH", pFileName, X_OK, &path8); // Require executability.
			
		if (success)
		{
			Strcpy(pPath, path8.c_str());
			return true;
		}
		return false;

	#else
		EA_UNUSED(pFileName); 
		EA_UNUSED(pPath); 
		EA_UNUSED(pEnvironmentVar);
		return false;
	#endif
}


#if defined(EA_PLATFORM_WINDOWS) && EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)
	namespace // anonymous namespace for this file.
	{
		typedef HINSTANCE (APIENTRY* ShellExecuteWFunctionType)(HWND hwnd, LPCWSTR lpOperation, LPCWSTR lpFile, LPCWSTR lpParameters, LPCWSTR lpDirectory, INT nShowCmd);
		static ShellExecuteWFunctionType ShellExecuteWFunction = NULL;
		static HINSTANCE hShellExecuteWFunctionLibrary = NULL;

		struct ShellExecuteWFunctionEntryPointFinder
		{
			ShellExecuteWFunctionEntryPointFinder()
			{
				hShellExecuteWFunctionLibrary = LoadLibraryW(EA_WCHAR("shell32.dll"));
				if(hShellExecuteWFunctionLibrary)
					ShellExecuteWFunction = (ShellExecuteWFunctionType)(void*)::GetProcAddress(hShellExecuteWFunctionLibrary, "ShellExecuteW");
			}
			~ShellExecuteWFunctionEntryPointFinder()
			{
				if(hShellExecuteWFunctionLibrary)
					::FreeLibrary(hShellExecuteWFunctionLibrary);
			}
		};
	}

	EASTDC_API bool OpenFile(const char16_t* pPath)
	{
		HINSTANCE hInstance = 0;

		ShellExecuteWFunctionEntryPointFinder sShellExecuteWFunctionEntryPointFinder;

		if(ShellExecuteWFunction)
		{
			if(Strstr(pPath, EA_CHAR16("http://")) == pPath) // If the path begins with "http://" and is thus a URL...
			{
				wchar_t pathMod[260 + 4];
				Strcpy(pathMod, EA_WCHAR("url:"));
				Strlcat(pathMod, reinterpret_cast<const wchar_t*>(pPath), 260 + 4); // ShellExecute wants the path to look like this: "url:http://www.bozo.com"
				hInstance = ShellExecuteWFunction(NULL, EA_WCHAR("open"), pathMod, NULL, NULL, SW_SHOWNORMAL);
			}
			else
			{
				hInstance = ShellExecuteWFunction(NULL, EA_WCHAR("open"), reinterpret_cast<const wchar_t*>(pPath), NULL, NULL, SW_SHOWNORMAL);
			}
		}

		return ((uintptr_t)hInstance > 32);
	}

	EASTDC_API bool OpenFile(const char* pPath)
	{
		char16_t path16[260]; 
		Strlcpy(path16, pPath, 260);

		return OpenFile(path16);
	}

#else

	EASTDC_API bool OpenFile(const char16_t* pPath)
	{
		char path8[260];
		Strlcpy(path8, pPath, 260);

		return OpenFile(path8);
	}

	EASTDC_API bool OpenFile(const char* pPath)
	{
		#if defined (EA_PLATFORM_OSX)
			const char* args[] =
			{
				"open",
				pPath,
				0
			};
			return Spawn("/usr/bin/open", args) != -1;
		#else
			EA_UNUSED(pPath);
			return false;
		#endif
	}

#endif // EA_PLATFORM_WINDOWS



} // namespace StdC

} // namespace EA







