///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////


#include <EAStdC/EAStdC.h>
#include <EAStdC/internal/SprintfCore.h>


namespace EA
{
	namespace StdC
	{
		EASTDC_API void Init()
		{
			SprintfLocal::EASprintfInit();
		}

		EASTDC_API void Shutdown()
		{
			SprintfLocal::EASprintfShutdown();
		}


		// Disabled by default, for compatibility with C99 behavior.
		bool gAssertionsEnabled = false;

		EASTDC_API void SetAssertionsEnabled(bool enabled)
		{
			gAssertionsEnabled = enabled;
		}

		EASTDC_API bool GetAssertionsEnabled()
		{
			return gAssertionsEnabled;
		}
	}
}






