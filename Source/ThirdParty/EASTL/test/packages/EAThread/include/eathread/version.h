///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////


#ifndef EATHREAD_VERSION_H
#define EATHREAD_VERSION_H


#include <eathread/internal/config.h>

#if defined(EA_PRAGMA_ONCE_SUPPORTED)
	#pragma once // Some compilers (e.g. VC++) benefit significantly from using this. We've measured 3-4% build speed improvements in apps as a result.
#endif



namespace EA
{
	namespace Thread
	{
		/// Version contains the version of the library when it was built.
		/// This can be used to verify the correct version has been linked
		/// into the executable or loaded by the O/S (in the case of a DLL).
		struct Version
		{
			int mMajor;
			int mMinor;
			int mPatch;
		};

		/// Get the library version information.
		EATHREADLIB_API const Version *GetVersion();

		/// Check that the linked/loaded library is the same as the headers 
		/// are expecting.
		///
		/// If the version numbers passed to CheckVersion match those
		/// built into the library when it was compiled, true is returned. 
		/// If not, false is returned.
		EATHREADLIB_API bool CheckVersion(int majorVersion, int minorVersion, int patchVersion);

	}

}

#endif
