///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#if defined(EA_PRAGMA_ONCE_SUPPORTED)
	#pragma once // Some compilers (e.g. VC++) benefit significantly from using this. We've measured 3-4% build speed improvements in apps as a result.
#endif


#ifndef EATHREAD_CALLSTACK_APPLE_H
#define EATHREAD_CALLSTACK_APPLE_H


#include <eathread/eathread_callstack.h>

namespace EA
{
namespace Thread
{

	/// ModuleInfoApple
	///
	/// This struct is based on the EACallstack ModuleInfo struct, but that can't be used here because
	/// this package is a lower level package than EACallstack.
	///
	struct ModuleInfoApple
	{
		char8_t  mPath[256];        /// File name or file path
		char8_t  mName[256];        /// Module name. Usually the same as the file name without the extension.
		uint64_t mBaseAddress;      /// Base address in memory.
		uint64_t mSize;             /// Module size in memory.
		char     mType[32];         /// The type field (e.g. __TEXT) from the vmmap output.
		char     mPermissions[16];  /// The permissions "r--/rwx" kind of string from the vmmap output.
	};


#if EATHREAD_APPLE_GETMODULEINFO_ENABLED
	/// GetModuleInfoApple
	///
	/// This function exists for the purpose of being a central module/VM map info collecting function,
	/// used by a couple functions within this package.
	/// Writes as many entries as possible to the user-supplied array, up to the capacity of the array.
	/// Returns the required number of entries, which may be more than the user-supplied capacity in the
	/// case that the user didn't supply enough.
	///
	size_t GetModuleInfoApple(ModuleInfoApple* pModuleInfoAppleArray, size_t moduleInfoAppleArrayCapacity, 
								const char* pTypeFilter = NULL, bool bEnableCache = true);
#endif


} // namespace Callstack

} // namespace EA

#endif // Header include guard














