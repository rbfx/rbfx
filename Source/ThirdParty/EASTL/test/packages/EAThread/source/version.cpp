///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "eathread/version.h"

namespace EA
{
	namespace Thread
	{
		const Version gVersion =
		{
			EATHREAD_VERSION_MAJOR,
			EATHREAD_VERSION_MINOR,
			EATHREAD_VERSION_PATCH
		};

		const Version* GetVersion()
		{
			return &gVersion;
		}

		bool CheckVersion(int majorVersion, int minorVersion, int patchVersion)
		{
			return (majorVersion == EATHREAD_VERSION_MAJOR) &&
				   (minorVersion == EATHREAD_VERSION_MINOR) &&
				   (patchVersion == EATHREAD_VERSION_PATCH);
		}
	}
}


