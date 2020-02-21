///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Implements a stopwatch-style timer. This is useful for both benchmarking
// and for implementing runtime timing.
/////////////////////////////////////////////////////////////////////////////


#ifndef EASTDC_EASTOPWATCH_H
#define EASTDC_EASTOPWATCH_H


#include <EAStdC/internal/Config.h>
#include <EABase/eabase.h>


/////////////////////////////////////////////////////////////////////////////
// EASTDC_STOPWATCH_USE_GETTIMEOFDAY / EASTDC_STOPWATCH_USE_CLOCK_GETTIME
//
// Defined as 0 or 1.
// Identifies if we are using clock_gettime or gettimeofday for GetCPUCycle.
//
#if defined(EA_PLATFORM_POSIX) // Posix means Linux, Unix, and Macintosh OSX, among others (including Linux-based mobile platforms).
	#include <time.h>

	// If using Dinkumware's standard library instead of GlibC and similar, then use clock_gettime (which is theoretically better).
	// To try to do: Use clock_gettime whenever possible in preference over gettimeofday.
	#if (defined(CLOCK_REALTIME) || defined(CLOCK_MONOTONIC))
		#define EASTDC_STOPWATCH_USE_GETTIMEOFDAY  0
		#define EASTDC_STOPWATCH_USE_CLOCK_GETTIME 1
	#else
		#define EASTDC_STOPWATCH_USE_GETTIMEOFDAY  1
		#define EASTDC_STOPWATCH_USE_CLOCK_GETTIME 0
	#endif
#else
	#define EASTDC_STOPWATCH_USE_GETTIMEOFDAY  0
	#define EASTDC_STOPWATCH_USE_CLOCK_GETTIME 0
#endif
/////////////////////////////////////////////////////////////////////////////





//////////////////////////////////////////////////////////////////////////////
/// EASTDC_STOPWATCH_DISABLE_CPU_CALIBRATION()
///
/// Use this macro in a module within a Win32 DLL or EXE to disable CPU
/// calibration on startup, which takes about half a second. Doing this causes
/// GetCPUFrequency() and CPU cycle based stopwatches to return placeholder
/// timing, although GetCPUCycle() and stopwatch cycle based measurements are
/// unaffected.
///
/// The initializer declared by this macro must be called before the default
/// stopwatch initializer executes. In VC++, the easiest way to do this is to
/// declare #pragma init_seg(compiler) before the auto-initializer. For this
/// reason, it is best to place this initializer in its own .cpp file.
///
/// Example usage:
///     #if defined(_MSC_VER)
///         #pragma warning(disable: 4074)
///         #pragma init_seg(compiler)
///     #endif
/// 
///     EASTDC_STOPWATCH_DISABLE_CPU_CALIBRATION()
///
///     int main(int argc, char** argv)
///     {
///         . . .
///     }
///
#ifndef EASTDC_STOPWATCH_DISABLE_CPU_CALIBRATION
	// To consider: provide a means for the user to #define the CPU frequency to use 
	// before using this macro. Otherwise the user has to accept the default (0) or 
	// would have to manually call EAStdCStopwatchDisableCPUCalibration instead of
	// using this macro.

	#if defined(__GNUC__)
		#define EASTDC_STOPWATCH_DISABLE_CPU_CALIBRATION()                                  \
			EASTDC_API void EAStdCStopwatchDisableCPUCalibration(uint64_t cpuFrequency = 0);\
																							\
			namespace EA                                                                    \
			{                                                                               \
				namespace StdC                                                              \
				{                                                                           \
					void AutoStopwatchDisableCPUCalibration() __attribute__((constructor)); \
					void AutoStopwatchDisableCPUCalibration()                               \
					{                                                                       \
						EAStdCStopwatchDisableCPUCalibration(0);                            \
					}                                                                       \
				}                                                                           \
			}
	#else
		#define EASTDC_STOPWATCH_DISABLE_CPU_CALIBRATION()                                  \
			EASTDC_API void EAStdCStopwatchDisableCPUCalibration(uint64_t cpuFrequency = 0);\
																							\
			namespace EA                                                                    \
			{                                                                               \
				namespace StdC                                                              \
				{                                                                           \
					struct AutoStopwatchDisableCPUCalibration                               \
					{                                                                       \
						AutoStopwatchDisableCPUCalibration()                                \
							{ EAStdCStopwatchDisableCPUCalibration(0); }                    \
					} gAutoStopwatchDisableCPUCalibration;                                  \
				}                                                                           \
			}
	#endif

#endif


namespace EA
{
namespace StdC
{
	//////////////////////////////////////////////////////////////////////////////
	/// Stopwatch 
	///
	/// The Stopwatch class acts very much like a hand-held stopwatch. You can start 
	/// it, stop it, start it again, reset it, and get the elapsed time. Elapsed time 
	/// acts just like a stopwatch -- if the stopwatch is running, elapsed time is the 
	/// current stopwatch time; if the stopwatch is stopped, elapsed time is the 
	/// time up till the stop.
	///
	/// Important things to know about Stopwatch:
	///     - There is a distinction between stopwatch cycles and CPU cycles. While it
	///        may be the case that the stopwatch uses a CPU cycle counter as its basis,
	///        this also may not be the case. In fact, using a CPU cycles counter 
	///        as the basis for a stopwatch is often a dangerous thing to do because 
	///        processors these days will sometimes switch frequencies on the fly.
	///
	///     - You won't get very accurate timings if you use a millisecond stopwatch
	///        repeatedly to time tiny sections of code that take only nanoseconds.
	///
	///     - You can start and stop a stopwatch at various times and it will do the 
	///        right thing, which is sum up the times during which it was running.
	///
	///     - Timing CPU cycles (clock ticks) accurately can be hard if you are
	///        trying to time very small pieces of code.  
	///
	///     - You don't have to stop a stopwatch that is running; it doesn't take 
	///        up CPU time to be running. It is not an error to not stop a stopwatch.
	///
	///     - You don't have to worry about multi-processing issues, even when 
	///        measuring clock ticks. If you start a stopwatch while your thread is 
	///        running on one CPU, you can stop it while running on a different CPU.
	///
	///     - You can call GetElapsedTime while the stopwatch is running and it will
	///        act as you expect.
	///
	///     - The construction and destruction instances of Stopwatch is fast; don't
	///        worry about it. Similarly, a Stopwatch instance is small in size.
	///
	/// Example usage:
	///     Stopwatch stopwatch(Stopwatch::kUnitsMilliseconds);
	///     stopwatch.Start();
	///     DoSomethingThatYouWantToMeasure();
	///     printf("Time to do something: %u.\n", stopwatch.GetElapsedTime());
	///
	/// Example usage:
	///     Stopwatch stopwatch(Stopwatch::kUnitsCycles);
	///     stopwatch.Start();
	///     DoSomethingSmall();
	///     printf("Time to do something small: %u.\n", stopwatch.GetElapsedTime());
	///     stopwatch.Restart();
	///     DoSomethingElseSmall();
	///     printf("Time to do something else small: %d.\n", stopwatch.GetElapsedTime());
	///
	/// Example usage:
	///     Stopwatch stopwatch(Stopwatch::kUnitsMilliseconds);
	///     stopwatch.Start();
	///     stopwatch.SetElapsedTime(100);
	///     printf("This should print out 100 (or maybe 101): %u.\n", stopwatch.GetElapsedTime());
	///
	/// Don't do this!:
	/// It seems that quite often people unfamiliar with a C++ stopwatch tend to revert away from
	/// using the stopwatch as it was designed and try to do timings manually, like shown below.
	/// The code below is more complicated and is less precise, as the high internal resolution 
	/// of the stopwatch is lost.
	///     Stopwatch stopwatch(Stopwatch::kUnitsMilliseconds);
	///     stopwatch.Start();
	///     uint64_t timeStart = stopwatch.GetElapsedTime();
	///     DoSomething();
	///     uint64_t timeElapsed = stopwatch.GetElapsedTime() - time;
	///
	/// Todo: Change all floats to doubles due to precision issues, especially when the Stopwatch
	/// is improperly used. Probably retain as float for PS2 platform.
	///
	class EASTDC_API Stopwatch
	{
	public:
		/// Units
		/// Defines common timing units plus a user-definable set of units.
		enum Units
		{
			kUnitsCycles       =    0,  /// Stopwatch clock ticks. May or may not match CPU clock ticks 1:1, depending on your hardware and operating system. Some CPUs' low level cycle count instruction counts every 16 cycles instead of every cycle. 
			kUnitsCPUCycles    =    1,  /// CPU clock ticks (or similar equivalent for the platform). Not recommended for use in shipping softare as many systems alter their CPU frequencies at runtime.
			kUnitsNanoseconds  =    2,  /// For a 1GHz processor, 1 nanosecond is the same as 1 clock tick.
			kUnitsMicroseconds =    3,  /// For a 1GHz processor, 1 microsecond is the same as 1,000 clock ticks.
			kUnitsMilliseconds =    4,  /// For a 1GHz processor, 1 millisecond is the same as 1,000,000 clock ticks.
			kUnitsSeconds      =    5,  /// For a 1GHz processor, 1 second is the same as 1,000,000,000 clock ticks.
			kUnitsMinutes      =    6,  /// For a 1GHz processor, 1 minute is the same as 60,000,000,000 clock ticks.
			kUnitsUserDefined  = 1000   /// User defined units, such as animation frames, video vertical retrace, etc.
		};

	public:
		/// Stopwatch
		/// Constructor for Stopwatch, allows user to specify units. 
		/// If units are kUnitsUserDefined,  you'll need to either subclass 
		/// Stopwatch and implement GetUserDefinedStopwatchCycle or call 
		/// SetUserDefinedUnitsToStopwatchCyclesRatio in order to allow it 
		/// to know how to convert units.
		explicit Stopwatch(int nUnits = kUnitsCycles, bool bStartImmediately = false);

		/// Stopwatch
		/// Copy constructor.
		Stopwatch(const Stopwatch& stopwatch);

		/// ~Stopwatch
		/// Destructor.
	   ~Stopwatch();

		/// operator=
		/// Assignment operator.
		Stopwatch& operator=(const Stopwatch& stopwatch);

		/// GetUnits
		/// Gets the current units. Returns one of enum Units or kUnitsUserDefined+.
		int GetUnits() const;

		/// SetUnits
		/// Sets the current units. One of enum Units or kUnitsUserDefined+.
		void SetUnits(int nUnits);

		/// Start
		/// Starts the stopwatch. Continues where it was last stopped. 
		/// Does nothing if the stopwatch is already started.
		void Start();

		/// Stop
		/// Stops the stopwatch it it was running and retaines the elasped time.
		void Stop();

		/// Reset
		/// Stops the stopwatch if it was running and clears the elapsed time.
		void Reset();

		/// Restart
		/// Clears the elapsed time and starts the stopwatch if it was not 
		/// already running. Has the same effect as Reset(), Start().
		void Restart();

		/// IsRunning
		/// Returns true if the stopwatch is running.
		bool IsRunning() const;

		/// GetElapsedTime
		/// Gets the elapsed time, which properly takes into account any 
		/// intervening stops and starts. Works properly whether the stopwatch 
		/// is running or not.
		uint64_t GetElapsedTime() const;

		/// SetElapsedTime
		/// Allows you to set the elapsed time. Erases whatever is current. 
		/// Works properly whether the stopwatch is running or not.
		void SetElapsedTime(uint64_t nElapsedTime);

		/// GetElapsedTimeFloat
		/// Float version, which is useful for counting fractions of 
		/// seconds or possibly milliseconds.
		float GetElapsedTimeFloat() const;

		/// SetElapsedTimeFloat
		/// Allows you to set the elapsed time. Erases whatever is current. 
		/// Works properly whether the stopwatch is running or not.
		void SetElapsedTimeFloat(float fElapsedTime);

		/// SetCyclesPerUnit
		/// Allows the user to manually override the frequency of the 
		/// timer. 
		void SetCyclesPerUnit(float fCyclesPerUnit);

		/// GetCyclesPerUnit
		/// Returns the value number of cycles per unit. If the user 
		/// set a manual value via SetCyclesPerUnit, this function returns 
		/// that value.
		float GetCyclesPerUnit() const;

		/// GetStopwatchCycle
		/// Gets the current stopwatch cycle on the current machine.
		/// Note that a stopwatch cycle may or may not be the same thing
		/// as a CPU cycle. We provide the distinction between stopwatch
		/// cycles and CPU cycles in order to accommodate  platforms (e.g.
		/// desktop platforms) in which CPU cycle counting is unreliable.
		static uint64_t GetStopwatchCycle();

		/// GetStopwatchFrequency
		/// Note that the stopwatch frequency may or may not be the same thing
		/// as the CPU frequency. We provide the distinction between stopwatch
		/// cycles and CPU cycles in order to accommodate platforms (e.g.
		/// desktop platforms) in which CPU cycle counting is unreliable.
		static uint64_t GetStopwatchFrequency();

		/// GetUnitsPerStopwatchCycle
		/// Returns the number of specified units per cycle.  
		/// If the unit is seconds, the return value would be the frequency of 
		/// the stopwatch timer and thus be the same value as returned by
		/// GetStopwatchFrequency().
		///
		/// Example usage:
		///     const float    fFrequencyInverse = Stopwatch::GetUnitsPerStopwatchCycle(Stopwatch::kUnitsSeconds);
		///     const uint64_t nStartCycle = Stopwatch::GetStopwatchCycle();
		///     <Do something>
		///     const uint64_t nElapsedTime = (Stopwatch::GetStopwatchCycle() - nStartStopwatchCycle) * fFrequencyInverse;
		///
		static float GetUnitsPerStopwatchCycle(Units units);

		/// GetCPUCycle
		/// Gets the current CPU-based timer cycle on the current processor 
		/// (if in a multiprocessor system). Note that this doesn't necessarily
		/// get the actual machine CPU clock cycle; rather it returns the 
		/// CPU-based timer cycle. One some platforms the CPU-based timer is
		/// a 1:1 relation to the CPU clock, while on others it is some multiple
		/// of it.
		/// Note that on some systems you can't rely on kUnitsCycles being consistent 
		/// at runtime, especially on x86 PCs with their multiple desynchronized CPUs 
		/// and variable runtime clock speed.
		static uint64_t GetCPUCycle();

		/// GetCPUFrequency
		/// Gets the frequency of the CPU-based timer. Note that this doesn't 
		/// necessarily get the actual machine CPU clock frequency; rather it returns  
		/// the CPU-based timer frequency. One some platforms the CPU-based timer is
		/// a 1:1 relation to the CPU clock, while on others it is some multiple of it.
		static uint64_t GetCPUFrequency(); 

		/// GetUnitsPerCPUCycle
		/// Returns the number of CPU cycles per the given unit.  
		/// If the unit is seconds, the return value would be the frequency 
		/// of the CPU-based timer.
		///
		/// Example usage:
		///     const float    fFrequencyInverse = Stopwatch::GetUnitsPerCPUCycle(Stopwatch::kUnitsSeconds);
		///     const uint64_t nStartCycle = Stopwatch::GetCPUCycle();
		///     <Do something>
		///     const uint64_t nElapsedTime = (Stopwatch::GetCPUCycle() - nStartStopwatchCycle) * fFrequencyInverse;
		///
		static float GetUnitsPerCPUCycle(Units units);

	protected:
		uint64_t    mnStartTime;                            /// Start time; always in cycles.
		uint64_t    mnTotalElapsedTime;                     /// Elapsed time; always in cycles.
		int         mnUnits;                                /// Stopwatch units. One of enum Units or kUnitsUserDefined+.
		float       mfStopwatchCyclesToUnitsCoefficient;    /// Coefficient is defined seconds per cycle (assuming you want to measure seconds). This is the inverse of the frequency, done this way for speed. Time passed = cycle count * coefficient.                 

	}; // class Stopwatch



	//////////////////////////////////////////////////////////////////////////////
	/// LimitStopwatch
	///
	/// Implements a stopwatch whose purpose is to tell whether a given amount of 
	/// time has passed. This has the same effect as using a Stopwatch and checking
	/// that its elapsed time >= its start time plus delta time. However, it is more
	/// efficient to have a LimitStopwatch because it can precalculate the end 
	/// condition and its IsTimeUp function merely compares the current tick with
	/// the end tick and thus no multiplication, division or other calculations need occur.
	///
	/// Example usage:
	///     LimitStopwatch limitStopwatch(Stopwatch::kUnitsMilliseconds, 1000, true);
	///     while(!limitStopwatch.IsTimeUp())
	///         printf("waiting\n");
	///
	/// Example usage of re-using a LimitStopwatch:
	///     LimitStopwatch limitStopwatch(Stopwatch::kUnitsSeconds);
	///     limitStopwatch.SetTimeLimit(10, true);
	///     while(!limitStopwatch.IsTimeUp() 
	///         { }
	///     limitStopwatch.SetTimeLimit(10, true);
	///     while(!limitStopwatch.IsTimeUp()
	///         { }
	///
	class EASTDC_API LimitStopwatch : public Stopwatch
	{
	public:
		/// LimitStopwatch
		/// Constructor
		LimitStopwatch(int nUnits = kUnitsCycles, uint64_t nLimit = 0, bool bStartImmediately = false);

		/// SetTimeLimit
		/// Sets the time limit and lets you start the stopwatch at the same time.
		void SetTimeLimit(uint64_t nLimit, bool bStartImmediately = false); 

		/// IsTimeUp
		/// Returns true if the limit has been reached. Highly efficient.
		bool IsTimeUp() const;

		/// GetTimeRemaining
		/// More expensive than IsTimeUp, as it returns a value.
		int64_t GetTimeRemaining() const;

		/// GetTimeRemainingFloat
		/// More expensive than IsTimeUp, as it returns a value.
		float GetTimeRemainingFloat() const;

	protected:
		uint64_t mnEndTime;     /// The precomputed end time used by limit timing functions.

	}; // class LimitStopwatch

} // namespace StdC

} // namespace EA



#if defined(EA_PLATFORM_MICROSOFT) && (defined(EA_PROCESSOR_ARM) || defined(EA_PLATFORM_WINDOWS_PHONE))
	#ifndef WIN32_LEAN_AND_MEAN
		#define EASTDC_WIN32_LEAN_AND_MEAN_DEFINED
		#define WIN32_LEAN_AND_MEAN
	#endif
	#include <windows.h>
	#ifdef EASTDC_WIN32_LEAN_AND_MEAN_DEFINED
		#undef EASTDC_WIN32_LEAN_AND_MEAN_DEFINED
		#undef WIN32_LEAN_AND_MEAN
	#endif

	inline uint64_t EA::StdC::Stopwatch::GetCPUCycle()
	{
		LARGE_INTEGER perfCounter;
		QueryPerformanceCounter(&perfCounter);
		return static_cast<uint64_t>(perfCounter.QuadPart);
	}


// x86-64/VC++
// EA_PROCESSOR_X86 is the processor for standard Windows PCs.
#elif defined(EA_PROCESSOR_X86_64) && defined(EA_COMPILER_MSVC)
	#pragma warning(push, 0)
	#include <math.h>       // VS2008 has an acknowledged bug that requires math.h (and possibly also string.h) to be #included before intrin.h.
	#if (_MSC_VER >= 1500)  // if VS2008 or later... Earlier versions of intrin.h are acknowledged as broken by Microsoft.
		#include <intrin.h>
	#else
		extern "C" unsigned __int64 __rdtsc(void);
		#pragma intrinsic(__rdtsc)
	#endif
	#pragma warning(pop)

	#define EASTDC_CPU_FREQ_CALCULATED

	__forceinline uint64_t EA::StdC::Stopwatch::GetCPUCycle()
	{
		return __rdtsc();
	}


// x86/VC++
// EA_PROCESSOR_X86 is the processor for standard Windows PCs.
#elif defined(EA_PROCESSOR_X86) && defined(EA_ASM_STYLE_INTEL)
	#define EASTDC_CPU_FREQ_CALCULATED

	#if defined(EA_COMPILER_MSVC)
		#if(EA_COMPILER_VERSION >= 1300)  // VC7.0 or later
			__forceinline
		#else
			__declspec(naked) inline // A problem with __declsped(naked) under VC6 and 
		#endif                       // earlier is that it makes a function non-inlinable.
	#else
		inline
	#endif

	uint64_t EA::StdC::Stopwatch::GetCPUCycle()
	{
		// Note that we do not call cpuid before rdtsc to serialize the instruction queue. If you are doing tiny measurements and 
		// need this serialization to prevent instructions from effectively executing after rdtsc, then use asm cpuid or use rdtscp (newer CPUs or XBox One platform).
		#if !defined(EA_COMPILER_MSVC) || (EA_COMPILER_VERSION >= 1200)
			__asm rdtsc // Read the CPU time stamp counter into edx:eax.
		#else
			__asm __emit 0x0f // 0x0f31 -> rdtsc for older compilers.
			__asm __emit 0x31
		#endif
		#if defined(EA_COMPILER_MSVC) && (EA_COMPILER_VERSION <= 1200)
			__asm ret // Problem: need to deal with stdcall vs. cdecl calling convention.
		#endif
	}



// Apple OSs
#elif defined(EA_PLATFORM_APPLE)
	#include <mach/mach_time.h>
	inline uint64_t EA::StdC::Stopwatch::GetCPUCycle()
	{
	   return mach_absolute_time();
	}


#elif defined(EA_PLATFORM_SONY)
	// According to Sony, there is a possible ~100 clock tick discrepancy between cores.
	// Thus GetCPUCycle values could go "back in time" if called on two cores, one very soon
	// after the other. https://ps4.scedev.net/forums/thread/18192/

	// sceKernelGetProcessTimeCounter is a userland function in a library and thus occurs
	// a small amount of overhead.  Our measurements show the overhead is ~6 cycles
	// compared to the previously implementation which utilized rdtsc directly. 

	// Previous implementation for reference:
	//
	// inline
	// uint64_t EA::StdC::Stopwatch::GetCPUCycle()
	// {
	//     // Note that we do not call cpuid before rdtsc to serialize the instruction queue. If you are doing tiny measurements  
	//     // and need this serialization to prevent instructions from effectively executing after rdtsc then use rdtscp.
	//     // Note that as of this writing clang doesn't support __builtin_ia32_rdtsc().
	//     #if EA_COMPILER_HAS_BUILTIN(__builtin_ia32_rdtsc)
	//         return __builtin_ia32_rdtsc();
	//     #else
	//         uint32_t edxHigh32;
	//         uint64_t raxLow32;

	//         asm volatile("rdtsc" : "=a" (raxLow32), "=d" (edxHigh32)); // rdtsc clears the high 32 bits of rax and so we can use result directly here.
	//         return ((uint64_t)edxHigh32 << 32) | raxLow32;
	//     #endif
	// }

	#include <kernel.h>
	inline uint64_t EA::StdC::Stopwatch::GetCPUCycle()
	{
		return sceKernelGetProcessTimeCounter();
	}


// x86,x86-64/GNUC or Clang
#elif (defined(EA_PROCESSOR_X86) || defined(EA_PROCESSOR_X86_64)) && (defined(EA_COMPILER_GNUC) || defined(EA_COMPILER_CLANG))
	#define EASTDC_CPU_FREQ_CALCULATED

	inline
	uint64_t EA::StdC::Stopwatch::GetCPUCycle()
	{
		// Note that we do not call cpuid before rdtsc to serialize the instruction queue. If you are doing tiny measurements and 
		// need this serialization to prevent instructions from effectively executing after rdtsc, then use asm cpuid or use rdtscp (newer CPUs).
		#if EA_COMPILER_HAS_BUILTIN(__builtin_ia32_rdtsc)
			return __builtin_ia32_rdtsc();
		#else
			uint32_t eaxLow32, edxHigh32;
			uint64_t result;

			asm volatile("rdtsc" : "=a" (eaxLow32), "=d" (edxHigh32));
			result = ((uint64_t)edxHigh32 << 32) | ((uint64_t)eaxLow32);

			return result;
		#endif
	}

// PowerPC64 and GCC
#elif defined(EA_PROCESSOR_POWERPC) && (EA_PLATFORM_WORD_SIZE == 8) && defined(EA_COMPILER_GNUC)
	#define EASTDC_CPU_FREQ_CALCULATED

	// This is a generic GCC version for 64 bit PowerPC machines.
	inline uint64_t EA::StdC::Stopwatch::GetCPUCycle()
	{
		uint64_t nTimeBase;
		__asm__ __volatile__ ("mftb %0" : "=r" (nTimeBase));
		return nTimeBase;
	}

// PowerPC32 and GCC
#elif defined(EA_PROCESSOR_POWERPC) && (EA_PLATFORM_WORD_SIZE == 4) && defined(EA_COMPILER_GNUC)
	#define EASTDC_CPU_FREQ_CALCULATED

	// This is a generic GCC version for 32 bit PowerPC machines.
	// We need to loop because we are reading two registers and the high one might change.
	inline uint64_t EA::StdC::Stopwatch::GetCPUCycle()
	{
		uint32_t high1, high2, low;

		__asm__ __volatile__ 
		(
			"1: mftbu %0\n"
			   "mftb  %1\n"
			   "mftbu %2\n"
			   "cmpw  %3,%4\n"
			   "bne-  1b\n"
			 : "=r" (high1), "=r" (low), "=r" (high2)
			 : "0" (high1), "2" (high2)
		);
		
		return (uint64_t(high1) << 32) | low;
	}
#elif defined(EA_PLATFORM_POSIX) // Posix means Linux, Unix, and Macintosh OSX, among others (including Linux-based mobile platforms).
	#include <time.h>

	#if   EASTDC_STOPWATCH_USE_CLOCK_GETTIME
		// http://linux.die.net/man/3/clock_gettime
		#include <errno.h>

		inline uint64_t EA::StdC::Stopwatch::GetCPUCycle()
		{
			timespec ts;
			int      result = clock_gettime(CLOCK_MONOTONIC, &ts);

			if(result == EINVAL 
				)
				result = clock_gettime(CLOCK_REALTIME, &ts);

			const uint64_t nNanoseconds = (uint64_t)ts.tv_nsec + ((uint64_t)ts.tv_sec * UINT64_C(1000000000));
			return nNanoseconds;
		}

	#else // EASTDC_STOPWATCH_USE_GETTIMEOFDAY
		#include <sys/time.h>
		#include <unistd.h>

		inline uint64_t EA::StdC::Stopwatch::GetCPUCycle()
		{
			struct timeval tv;
			gettimeofday(&tv, NULL);
			const uint64_t nMicroseconds = (uint64_t)tv.tv_usec + ((uint64_t)tv.tv_sec * 1000000);
			return nMicroseconds;
		}
	#endif

#else

	#error EA::StdC::Stopwatch::GetCPUCycle not implemented.

#endif






#if defined(EA_PLATFORM_MICROSOFT) && !EASTDC_STOPWATCH_FORCE_CPU_CYCLE_USAGE
	// PC computers (x86 and otherwise) have the property whereby they change 
	// CPU frequencies on the fly. As such, you cannot safely use GetCPUCycle
	// on these computers to tell you how much time has passed. This is not the
	// case with XBOX because it is specifically designed not to do this.
	// You can disable usage of QueryPerformanceCounter below by defining
	// EASTDC_STOPWATCH_FORCE_CPU_CYCLE_USAGE as 1.
	// hardcode prototype here so we don't pull in <windows.h>
	extern "C" __declspec(dllimport) int __stdcall QueryPerformanceCounter(union _LARGE_INTEGER *lpPerformanceCount);   // Urho3D: _Out_ is not defined on MinGW

	inline uint64_t EA::StdC::Stopwatch::GetStopwatchCycle()
	{
		 uint64_t nReturnValue;
		 QueryPerformanceCounter((union _LARGE_INTEGER*)&nReturnValue);
		 return nReturnValue;
	}


// Apple OSs
#elif defined(__APPLE__)
   #include <mach/mach_time.h>
   inline uint64_t EA::StdC::Stopwatch::GetStopwatchCycle()
   {
	  return mach_absolute_time();
   }


#elif defined(EA_PLATFORM_SONY)
	// See the discussion above in GetCPUCycle for EA_PLATFORM_SONY.
	// Previous implementation for reference:
	//
	// inline uint64_t EA::StdC::Stopwatch::GetStopwatchCycle()
	// {
	//     uint32_t eaxLow32, edxHigh32;
	//     uint64_t result;
	//
	//     asm volatile("rdtsc" : "=a" (eaxLow32), "=d" (edxHigh32));
	//     result = ((uint64_t)edxHigh32 << 32) | ((uint64_t)eaxLow32);
	//
	//     return result;
	// }

	#include <kernel.h>
	inline uint64_t EA::StdC::Stopwatch::GetStopwatchCycle()
	{
		return sceKernelGetProcessTimeCounter();
	}


#elif defined(EA_PLATFORM_POSIX)
	#include <time.h>

	#if EASTDC_STOPWATCH_USE_CLOCK_GETTIME
		// http://linux.die.net/man/3/clock_gettime
		#include <errno.h>

		inline uint64_t EA::StdC::Stopwatch::GetStopwatchCycle()
		{
			timespec ts;
			int      result = clock_gettime(CLOCK_MONOTONIC, &ts);

			if(result == EINVAL 
				)
				result = clock_gettime(CLOCK_REALTIME, &ts);

			const uint64_t nNanoseconds = (uint64_t)ts.tv_nsec + ((uint64_t)ts.tv_sec * UINT64_C(1000000000));
			return nNanoseconds;
		}

	#elif EASTDC_STOPWATCH_USE_GETTIMEOFDAY
		#include <sys/time.h>
		#include <unistd.h>

		inline uint64_t EA::StdC::Stopwatch::GetStopwatchCycle()
		{
			struct timeval tv;
			gettimeofday(&tv, NULL);
			const uint64_t nMicroseconds = (uint64_t)tv.tv_usec + ((uint64_t)tv.tv_sec * 1000000);
			return nMicroseconds;
		}
	#else
		inline uint64_t EA::StdC::Stopwatch::GetStopwatchCycle()
		{
			return GetCPUCycle();
		}
	#endif

#else
	// Other supported processors have fixed-frequency CPUs and thus can 
	// directly use the GetCPUCycle functionality for maximum precision
	// and speed.
	inline uint64_t EA::StdC::Stopwatch::GetStopwatchCycle()
	{
		return GetCPUCycle();
	}
#endif




///////////////////////////////////////////////////////////////////////////////
// Inline functions
//
// We choose to make these functions inline because they are so small that 
// making them inline would likely have no effect on code size but would 
// significantly improve speed and possibly instruction cache friendliness.
// Also, the PS2 compiler is picky about ordering of inline functions, so
// the StopWatch class inlines are placed after the GetCPUCycle functions.
///////////////////////////////////////////////////////////////////////////////

inline
EA::StdC::Stopwatch::Stopwatch(const Stopwatch& stopwatch)
{
	mnUnits = stopwatch.mnUnits;
	mnStartTime = stopwatch.mnStartTime;
	mnTotalElapsedTime = stopwatch.mnTotalElapsedTime;
	mfStopwatchCyclesToUnitsCoefficient = stopwatch.mfStopwatchCyclesToUnitsCoefficient;
}


inline
EA::StdC::Stopwatch::~Stopwatch()
{
	// Empty
}


inline
EA::StdC::Stopwatch& EA::StdC::Stopwatch::operator =(const Stopwatch& stopwatch)
{
	mnUnits = stopwatch.mnUnits;
	mnStartTime = stopwatch.mnStartTime;
	mnTotalElapsedTime = stopwatch.mnTotalElapsedTime;
	mfStopwatchCyclesToUnitsCoefficient = stopwatch.mfStopwatchCyclesToUnitsCoefficient;
	return *this;
}


inline
int EA::StdC::Stopwatch::GetUnits() const
{
	return mnUnits;
}


inline
void EA::StdC::Stopwatch::Start()
{
	if(!mnStartTime) // If not already started...
	{
		if(mnUnits == kUnitsCPUCycles)
			mnStartTime = GetCPUCycle();
		else
			mnStartTime = GetStopwatchCycle();

	}
}


inline
void EA::StdC::Stopwatch::Reset()
{
	mnStartTime = 0;
	mnTotalElapsedTime = 0;
}


inline
void EA::StdC::Stopwatch::Restart()
{
	mnStartTime = 0;
	mnTotalElapsedTime = 0;
	Start();
}


inline
bool EA::StdC::Stopwatch::IsRunning() const
{
	return (mnStartTime != 0);
}


inline
void EA::StdC::Stopwatch::SetCyclesPerUnit(float fCyclesPerUnit)
{
	mfStopwatchCyclesToUnitsCoefficient = fCyclesPerUnit;
}


inline
float EA::StdC::Stopwatch::GetCyclesPerUnit() const
{
	return mfStopwatchCyclesToUnitsCoefficient;
}




inline
EA::StdC::LimitStopwatch::LimitStopwatch(int nUnits, uint64_t nLimit, bool bStartImmediately) 
	: Stopwatch(nUnits, false)
{
	SetTimeLimit(nLimit, bStartImmediately);
}


inline
bool EA::StdC::LimitStopwatch::IsTimeUp() const
{
	const uint64_t nCurrentTime = GetStopwatchCycle();
	return (int64_t)(mnEndTime - nCurrentTime) < 0; // We do this instead of a straight compare to deal with possible integer wraparound issues.
}
			
			
inline
int64_t EA::StdC::LimitStopwatch::GetTimeRemaining() const
{
	const uint64_t nCurrentTime = GetStopwatchCycle();
	const int64_t  nTimeRemaining = (int64_t)((int64_t)(mnEndTime - nCurrentTime) * mfStopwatchCyclesToUnitsCoefficient);
	return nTimeRemaining;
}



#endif // header guard











