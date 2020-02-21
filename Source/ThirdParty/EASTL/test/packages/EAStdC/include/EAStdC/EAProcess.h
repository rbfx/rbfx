///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// This module defines functions for process spawning and query.
// Normally this functionality is present only on platforms that support
// multiple processes, such as desktop and server platforms.
/////////////////////////////////////////////////////////////////////////////


#ifndef EASTDC_EAPROCESS_H
#define EASTDC_EAPROCESS_H


#include <EABase/eabase.h>
#include <EAStdC/internal/Config.h>
#include <EAStdC/EAString.h>
EA_DISABLE_ALL_VC_WARNINGS()
#include <stddef.h>
#include <stdlib.h>
EA_RESTORE_ALL_VC_WARNINGS()


namespace EA
{
   namespace StdC
   {
		#if defined(EA_PLATFORM_WINDOWS) && EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)
			const int kMaxPathLength      = _MAX_PATH;
			const int kMaxDirectoryLength = _MAX_PATH;
		#elif defined(EA_PLATFORM_XBOXONE) || defined(CS_UNDEFINED_STRING)
			const int kMaxPathLength      = 260;
			const int kMaxDirectoryLength = 260;
		#elif defined(EA_PLATFORM_SONY)
			const int kMaxPathLength      = 1024;
			const int kMaxDirectoryLength = 1024;
		#elif defined(EA_PLATFORM_OSX)
			const int kMaxPathLength      = 1024;
			const int kMaxDirectoryLength = 1024;
		#elif defined(EA_PLATFORM_LINUX)
			const int kMaxPathLength      = 1024;
			const int kMaxDirectoryLength = 1024;
		#else
			const int kMaxPathLength      = 512;
			const int kMaxDirectoryLength = 512;
		#endif


		/// PathFlags
		///
		enum PathFlags
		{
			kPathFlagNone       = 0x00,
			kPathFlagBundlePath = 0x01   // Apple-specific: Return the path to the bundle instead of its inner contents. https://developer.apple.com/library/ios/documentation/CoreFoundation/Conceptual/CFBundles/AboutBundles/AboutBundles.html
		};


		/// GetCurrentProcessPath
		///
		/// Returns the file path to the current process.
		/// The output parameter pPath must be big enough to hold the largest
		/// possible path (i.e. IO::kMaxPathLength) for the given platform. 
		/// The return value is the strlen of the path in the argument pPath.
		/// The return value is 0 upon failure, but there should never be failure
		/// as long as the function is implemented on the given platform.
		/// The pathCapacity must be at least 1.
		///
		/// Example usage:
		///     char16_t path[IO::kMaxPathLength];
		///
		///     GetCurrentProcessPath(path, IO::kMaxPathLength);
		///     printf("Path: %ls\n", path);
		///
		EASTDC_API size_t GetCurrentProcessPath(char*  pPath, int pathCapacity = kMaxPathLength, int pathFlags = kPathFlagNone);
		EASTDC_API size_t GetCurrentProcessPath(char16_t* pPath, int pathCapacity = kMaxPathLength, int pathFlags = kPathFlagNone);
		EASTDC_API size_t GetCurrentProcessPath(char32_t* pPath, int pathCapacity = kMaxPathLength, int pathFlags = kPathFlagNone);
		#if defined(EA_WCHAR_UNIQUE) && EA_WCHAR_UNIQUE
			size_t GetCurrentProcessPath(wchar_t* pPath, int pathCapacity = kMaxPathLength, int pathFlags = kPathFlagNone);
		#endif


		/// SetCurrentProcessPath
		///
		/// Specifies the process path, for platforms in which it's not possible for this library
		/// to know it without being told. Subsequent calls to GetCurrentProcessPath with flags other
		/// than kPathFlagNone will ignore such flags and simply return this path.
		///
		EASTDC_API void SetCurrentProcessPath(const char* pPath);


		/// GetCurrentProcessDirectory
		///
		/// Returns the directory path to the current process.
		/// The output parameter pPath must be big enough to hold the largest
		/// possible directory path (i.e. IO::kMaxDirectoryLength) for the given platform. 
		/// The return value is the strlen of the path in the argument pPath.
		/// The return value is 0 upon failure, but there should never be failure
		/// as long as the function is implemented on the given platform.
		/// The pathCapacity must be at least 1.
		///
		/// The return value will have a trailing directory separator, as with 
		/// other directory paths in this system.
		///
		/// Example usage:
		///     char16_t dir[IO::kMaxDirectoryLength];
		///
		///     GetCurrentProcessDirectory(dir, IO::kMaxDirectoryLength);
		///     printf("Directory: %ls\n", dir);
		///
		EASTDC_API size_t GetCurrentProcessDirectory(char*  pDirectory, int pathCapacity = kMaxDirectoryLength, int pathFlags = kPathFlagNone);
		EASTDC_API size_t GetCurrentProcessDirectory(char16_t* pDirectory, int pathCapacity = kMaxDirectoryLength, int pathFlags = kPathFlagNone);
		EASTDC_API size_t GetCurrentProcessDirectory(char32_t* pDirectory, int pathCapacity = kMaxDirectoryLength, int pathFlags = kPathFlagNone);
		#if defined(EA_WCHAR_UNIQUE) && EA_WCHAR_UNIQUE
			size_t GetCurrentProcessDirectory(wchar_t* pDirectory, int pathCapacity = kMaxDirectoryLength, int pathFlags = kPathFlagNone);
		#endif


		/// GetEnvironmentVar
		///
		/// Environment variables are per-process global variables. Their advantage
		/// is that they can be read or written at any time during the process execution
		/// and are available from any part of the executing process, including from
		/// dynamic libraries (e.g. DLLs on the Windows platform). 
		///
		/// The input pName is a 0-terminated string.
		/// The output sValue will hold the resulting environment variable string.
		/// The return value is the strlen of the required string. Thus a return value
		/// that is less than valueCapacity was able to write the entire result string.
		/// A return value of zero means that the variable exists but it is empty.
		/// A return value of size_t(-1) means the variable doesn't exist.
		///
		/// This function is named GetEnvironmentVar instead of GetEnvironmentVariable
		/// because the windows.h file #defines the latter to something else.
		///
		/// As of this writing, another version of GetEnvironmentVar exists in the 
		/// EAEnvironmentVariable module and which uses char buffers instead of string
		/// objects. You may prefer the char buffers if you are avoiding memory allocations.
		///
		/// Example usage:
		///     wchar_t pValue[64];
		///
		///     if(GetEnvironmentVar(L"UserName", pValue, 64) < 64)
		///         printf("Path: %ls\n", pValue);
		///
		EASTDC_API size_t GetEnvironmentVar(const char*  pName, char*  pValue, size_t valueCapacity);
		EASTDC_API size_t GetEnvironmentVar(const char16_t* pName, char16_t* pValue, size_t valueCapacity);
		EASTDC_API size_t GetEnvironmentVar(const char32_t* pName, char32_t* pValue, size_t valueCapacity);
		#if defined(EA_WCHAR_UNIQUE) && EA_WCHAR_UNIQUE
			size_t GetEnvironmentVar(const wchar_t* pName, wchar_t* pValue, size_t valueCapacity);
		#endif


		/// SetEnvironmentVar
		///
		/// Sets the given environment variable. Returns true if it could be set. 
		/// A return value of false means the variable didn't previously exist.
		/// To remove an environment variable, set pValue to NULL. Removing is different
		/// from seting to an empty string, as the latter will result in a successful
		/// return from GetEnvironmentVar, whereas the former will result in an error return.
		/// This function is named SetEnvironmentVar instead of SetEnvironmentVariable
		/// because the windows.h file #defines the latter to something else.
		///
		/// Example usage:
		///     if(SetEnvironmentVar("User Name", "Lance Armstrong"))
		///         printf("Success setting user name.\n");
		///
		EASTDC_API bool SetEnvironmentVar(const char*  pName, const char*  pValue);
		EASTDC_API bool SetEnvironmentVar(const char16_t* pName, const char16_t* pValue);
		EASTDC_API bool SetEnvironmentVar(const char32_t* pName, const char32_t* pValue);
		#if defined(EA_WCHAR_UNIQUE) && EA_WCHAR_UNIQUE
			bool SetEnvironmentVar(const wchar_t* pName, const wchar_t* pValue);
		#endif


		/// Spawn
		///
		/// This function spawns a process whose path is 'pPath'. The arguments for the
		/// process are specified as an array of pointers to strings.
		///
		/// It is customary for the application that spawns another application to pass
		/// the spawned application path as the first argument. However, we, like the
		/// standard C spawn function, do not enforce this. It is up to the caller to
		/// follow this convention.
		///
		/// The return value is the exit status of the process, or -1 on error.
		///
		/// Given that this is a low-level C runtime replacement function, this function
		/// does not do string encoding conversion of the input path. The caller must
		/// do such a conversion manually with a call to:
		///     EA::TextUtil::ConvertStringEncoding(sSomePathSource, sSomePathDestination, kCodePageSystem);
		///
		/// If 'wait' is true, the function does not return until the spawned process completes.
		///
		/// This function works only on systems that support multiple concurrent
		/// processes. Usually that means desktop and server operating systems.
		///
		/// Example usage:
		///   const char16_t* ptrArray[4] = { EA_CHAR16("/System/Utilities/PingSomeAddresses.exe"), EA_CHAR16("www.bozo.com"), EA_CHAR16("www.nifty.com"), NULL };
		///   int nReturnValue = Spawn("/System/Utilities/PingSomeAddresses.exe", ptrArray, true);
		///
		EASTDC_API int Spawn(const char*  pPath, const char*  const* pArgumentArray, bool wait = false);
		EASTDC_API int Spawn(const char16_t* pPath, const char16_t* const* pArgumentArray, bool wait = false);
		EASTDC_API int Spawn(const char32_t* pPath, const char32_t* const* pArgumentArray, bool wait = false);
		#if defined(EA_WCHAR_UNIQUE) && EA_WCHAR_UNIQUE
			int Spawn(const wchar_t* pPath, const wchar_t* const* pArgumentArray, bool wait = false);
		#endif


		/// ExecuteShellCommand
		///
		/// Similar to the C runtime "system()" function present on some platforms.
		/// Multiple commands can be executed by separating them with newline characters.
		///
		/// This function works only on systems that support system-level command execution. 
		/// Usually that means desktop and server operating systems.
		///
		/// Example usage:
		///    ExecuteShellCommand("su root");
		///    ExecuteShellCommand("rm /* -r");
		///
		///    ExecuteShellCommand("su root\nrm /* -r");
		///
		EASTDC_API int ExecuteShellCommand(const char*  pCommand);
		EASTDC_API int ExecuteShellCommand(const char16_t* pCommand);
		EASTDC_API int ExecuteShellCommand(const char32_t* pCommand);
		#if defined(EA_WCHAR_UNIQUE) && EA_WCHAR_UNIQUE
			int ExecuteShellCommand(const wchar_t* pCommand);
		#endif


		/// SearchEnvironmentPath
		///
		/// Searches the system application path set for the named application.
		/// 
		/// Returns true and the full path to the file if found. Desktop operating
		/// systems will often have a PATH variable or similar setting which lists 
		/// a set of directories in which to look for executable programs when the 
		/// programs are executed by file name (and not directory) alone. 
		/// This function searches the system path set for the given file name.
		/// The returned path will have surrounding quotes removed if they were present.
		///
		/// The pFileName and pPath parameters may not be NULL.
		///  
		/// If the input pEnvironmentVar is non-NULL, the environment variable
		/// identified by pEnvironmentVar is assumed to refer to a set of 
		/// paths to search and it is used instead of the default system path set.
		/// The environment variable should be of the form "<directory>;<directory>;..." 
		/// where individual paths may be quoted. Note that pEnvironmentVar specifies
		/// the environment variable name (e.g. "PATH") and not its value.
		///
		/// Example usage:
		///     char16_t fullPath[IO::kMaxPathLength];
		///
		///     if(SearchEnvironmentPath("perforce.exe", fullPath, "PATH"))
		///         printf("Full path to Perforce is "%ls\n", fullPath);
		///
		EASTDC_API bool SearchEnvironmentPath(const char*  pFileName, char*  pPath, const char*  pEnvironmentVar = NULL);
		EASTDC_API bool SearchEnvironmentPath(const char16_t* pFileName, char16_t* pPath, const char16_t* pEnvironmentVar = NULL);
		EASTDC_API bool SearchEnvironmentPath(const char32_t* pFileName, char32_t* pPath, const char32_t* pEnvironmentVar = NULL);
		#if defined(EA_WCHAR_UNIQUE) && EA_WCHAR_UNIQUE
			bool SearchEnvironmentPath(const wchar_t* pFileName, wchar_t* pPath, const wchar_t* pEnvironmentVar = NULL);
		#endif


		/// OpenFile
		///
		/// Opens a file via the default system application 
		/// For example, a .doc file would be opened by your word processor.
		/// The input pPath must point to a valid path string.
		///
		/// This function works only on systems that support multiple concurrent
		/// processes. Usually that means desktop and server operating systems.
		///
		/// Example usage:
		///    OpenFile("/system/settings/somefile.txt");
		///    OpenFile("/system/settings/somefile.html");
		///    OpenFile("http://www.bozo.com/somefile.html");
		///
		EASTDC_API bool OpenFile(const char*  pPath);
		EASTDC_API bool OpenFile(const char16_t* pPath);
		EASTDC_API bool OpenFile(const char32_t* pPath);
		#if defined(EA_WCHAR_UNIQUE) && EA_WCHAR_UNIQUE
			bool OpenFile(const wchar_t* pPath);
		#endif


   } // namespace StdC

} // namespace EA






///////////////////////////////////////////////////////////////////////////////
// Inlines
///////////////////////////////////////////////////////////////////////////////

namespace EA
{
	namespace StdC
	{

		#if defined(EA_WCHAR_UNIQUE) && EA_WCHAR_UNIQUE

			#if !defined(EASTDC_UNICODE_CONST_CHAR_PTR_CONST_CHAR_PTR_CAST)
				#if (EA_WCHAR_SIZE == 2)
					#define EASTDC_UNICODE_CONST_CHAR_PTR_CONST_CHAR_PTR_CAST(x) reinterpret_cast<const char16_t* const*>(reinterpret_cast<const void *>(x))
				#else
					#define EASTDC_UNICODE_CONST_CHAR_PTR_CONST_CHAR_PTR_CAST(x) reinterpret_cast<const char32_t* const*>(x)
				#endif
			#endif
			
			
			inline size_t GetCurrentProcessPath(wchar_t* pPath, int pathCapacity, int pathFlags)
			{
				return GetCurrentProcessPath(EASTDC_UNICODE_CHAR_PTR_CAST(pPath), pathCapacity, pathFlags);
			}
			
			inline size_t GetCurrentProcessDirectory(wchar_t* pDirectory, int pathCapacity, int pathFlags)
			{
				return GetCurrentProcessDirectory(EASTDC_UNICODE_CHAR_PTR_CAST(pDirectory), pathCapacity, pathFlags);
			}

			inline size_t GetEnvironmentVar(const wchar_t* pName, wchar_t* pValue, size_t valueCapacity)
			{
				return GetEnvironmentVar(EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pName), 
										 EASTDC_UNICODE_CHAR_PTR_CAST(pValue), 
										 valueCapacity);
			}

			inline bool SetEnvironmentVar(const wchar_t* pName, const wchar_t* pValue)
			{
				return SetEnvironmentVar(EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pName), 
										 EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pValue));
			}
			
			inline int Spawn(const wchar_t* pPath, const wchar_t* const* pArgumentArray, bool wait)
			{
				return Spawn(EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pPath), 
							 EASTDC_UNICODE_CONST_CHAR_PTR_CONST_CHAR_PTR_CAST(pArgumentArray), 
							 wait);
			}
			
			inline int ExecuteShellCommand(const wchar_t* pCommand)
			{
				return ExecuteShellCommand(EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pCommand));
			}
			
			inline bool SearchEnvironmentPath(const wchar_t* pFileName, wchar_t* pPath, const wchar_t* pEnvironmentVar)
			{
				return SearchEnvironmentPath(EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pFileName), 
											 EASTDC_UNICODE_CHAR_PTR_CAST(pPath), 
											 EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pEnvironmentVar));
			}
			
			inline bool OpenFile(const wchar_t* pPath)
			{
				return OpenFile(EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pPath)); // EASTDC_UNICODE_CONST_CHAR_PTR_CAST is defined in EAString.h
			}
			

		#endif


	} // namespace StdC

} // namespace EA


#endif // Header include guard












