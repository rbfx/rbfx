///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////


#include <EAStdC/internal/Config.h>


#ifndef EASTDC_EASTDC_H
#define EASTDC_EASTDC_H


namespace EA
{
	namespace StdC
	{
		/// Does any startup initialization that EAStdC might need. Usually this
		/// means allocating structures that can't otherwise be automatically 
		/// auto-initialized (e.g. due to thread safety problems).
		/// Memory may be allocated via EASTDC_NEW within this function. 
		EASTDC_API void Init();

		/// Undoes any initialization that Init did.
		/// Memory may be freed via EASTDC_DELETE within this function.
		EASTDC_API void Shutdown();



		/// SetAssertionsEnabled
		/// If enabled then debug builds execute EA_ASSERT and EA_FAIL statements
		/// for serious failures that are otherwise silent. For example, the C strtoul
		/// function silently fails by default according the the C99 language standard.
		///
		/// The assertions this applies to are assertions that check user parameters 
		/// and thus detect user bugs, in particular those that could otherwise go 
		/// silently undected. This doesn't apply to assertions that are internal
		/// sanity checks. 
		///
		/// By default assertions are disabled (for compatibility with C99 behavior), 
		/// but they also require EA_ASSERT to be enabled for the given build.
		EASTDC_API void SetAssertionsEnabled(bool enabled);

		/// GetAssertionsEnabled
		/// Returns whether assertions are enabled, as described in SetAssertionsEnabled.
		EASTDC_API bool GetAssertionsEnabled();
	}
}

#endif // Header include guard


