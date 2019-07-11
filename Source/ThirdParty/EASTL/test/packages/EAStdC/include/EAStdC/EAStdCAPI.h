#pragma once


#ifndef EASTDC_API // If the build file hasn't already defined this to be dllexport...
	#if EASTDC_DLL
		#if defined(_WIN32)
            #if defined(EASTDC_EXPORTS) || defined(Urho3D_EXPORTS)
			    #define EASTDC_API      __declspec(dllexport)
            #else
			    #define EASTDC_API      __declspec(dllimport)
            #endif
			#define EASTDC_LOCAL
		#elif defined(__CYGWIN__)
			#define EASTDC_API      __attribute__((dllimport))
			#define EASTDC_LOCAL
		#elif (defined(__GNUC__) && (__GNUC__ >= 4))
			#define EASTDC_API      __attribute__ ((visibility("default")))
			#define EASTDC_LOCAL    __attribute__ ((visibility("hidden")))
		#else
			#define EASTDC_API
			#define EASTDC_LOCAL
		#endif
	#else
		#define EASTDC_API
		#define EASTDC_LOCAL
	#endif
#endif
