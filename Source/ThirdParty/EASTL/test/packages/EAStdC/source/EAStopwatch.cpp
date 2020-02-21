///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Implements a stopwatch-style stopwatch. This is useful for both benchmarking
// and for implementing runtime timing.
/////////////////////////////////////////////////////////////////////////////


#include <EAStdC/EAStopwatch.h>
#include <EAAssert/eaassert.h>
#if   defined(EA_PLATFORM_MICROSOFT)
	#pragma warning(push, 0)
	#include <Windows.h>
	#pragma warning(pop)
	#if (defined(EA_PLATFORM_WINDOWS) && !EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)) || defined(EA_PLATFORM_WINDOWS_PHONE)
		EA_DISABLE_ALL_VC_WARNINGS()
		#include <chrono>
		#include <thread>
		EA_RESTORE_ALL_VC_WARNINGS()
	#endif
#elif defined(EA_PLATFORM_SONY)
	#include <kernel.h>
#elif defined(EA_PLATFORM_POSIX)
	#include <unistd.h>
#endif




///////////////////////////////////////////////////////////////////////////////
// Auto-setup code
//
// The code below is for setting up the stopwatch system, and it's needed
// because it allows us to know at runtime what the stopwatch frequency is
// without having to spend CPU cycles repeatedly calculating it at runtime.
// The code below is set up to auto-execute on startup when possible, but 
// it's OK if it doesn't because the Stopwatch constructor auto-checks for
// initialization and calls if it not already. However, the auto code below
// is still useful because it's better to get this taken care of on startup
// rather than unexpectedly during runtime (as it takes a few milliseconds 
// on some platforms).
///////////////////////////////////////////////////////////////////////////////

#if defined(_MSC_VER)
	#ifndef EASTDC_INIT_SEG_DEFINED
		#define EASTDC_INIT_SEG_DEFINED 
		// Set initialization order between init_seg(compiler) (.CRT$XCC) and
		// init_seg(lib) (.CRT$XCL). The MSVC linker sorts the .CRT sections
		// alphabetically so we simply need to pick a name that is between
		// XCC and XCL. This works on both Windows and XBox.
		#pragma warning(disable: 4075) // "initializers put in unrecognized initialization area"
		#pragma init_seg(".CRT$XCF")
	#endif

#elif defined(__GNUC__)
	// By adding the 'constructor' attribute to this function, we tell GCC 
	// to have this function be called on startup before main(). We have the 
	// AutoStopwatchSetup mechanism below, but GCC ignores it because the 
	// object isn't externally reference and GCC thinks it can thus be eliminated.
	void EAStdCStopwatchSetupCoefficients();
	void EAStdCStopwatchSetup() __attribute__ ((constructor));
	void EAStdCStopwatchDisableCPUCalibration(uint64_t cpuFrequency);

#else
	// Some compilers require these functions to be pre-declared.
	void EAStdCStopwatchSetupCoefficients();
	void EAStdCStopwatchSetup();
	void EAStdCStopwatchDisableCPUCalibration(uint64_t cpuFrequency);

#endif






// Anonymous namespace
// Has the effect of making things declared within it be static, but with some improvements.
namespace 
{
	// Stopwatch cycle metrics
	uint64_t mnStopwatchFrequency(1); // Set to one to prevent possible div/0 errors.
	float    mfStopwatchCyclesToNanosecondsCoefficient;
	float    mfStopwatchCyclesToMicrosecondsCoefficient;
	float    mfStopwatchCyclesToMillisecondsCoefficient;
	float    mfStopwatchCyclesToSecondsCoefficient;
	float    mfStopwatchCyclesToMinutesCoefficient;

	// CPU cycle metrics
	uint64_t mnCPUFrequency(1); // Set to one to prevent possible div/0 errors.
	float    mfCPUCyclesToNanosecondsCoefficient;
	float    mfCPUCyclesToMicrosecondsCoefficient;
	float    mfCPUCyclesToMillisecondsCoefficient;
	float    mfCPUCyclesToSecondsCoefficient;
	float    mfCPUCyclesToMinutesCoefficient;

	#if EASTDC_STOPWATCH_OVERHEAD_ENABLED
		uint64_t mnCPUCycleReadingOverhead(0);
		uint64_t mnStopwatchCycleReadingOverhead(0);
	#endif

	#if defined(EA_PLATFORM_MICROSOFT)
		void EAStdCThreadSleep(int ms)
		{
			#if (defined(EA_PLATFORM_WINDOWS) && !EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)) || defined(EA_PLATFORM_WINDOWS_PHONE)
				std::chrono::milliseconds duration(ms);
				std::this_thread::sleep_for(duration);
			#else
				::SleepEx((DWORD)ms, TRUE);
			#endif
		}
	#endif
}



void EAStdCStopwatchSetupCoefficients()
{
	// Calculate coefficients.
	mfStopwatchCyclesToMinutesCoefficient      = 1.f / 60.f   / (int64_t)mnStopwatchFrequency;    // Some compilers require the 
	mfStopwatchCyclesToSecondsCoefficient      = 1.f          / (int64_t)mnStopwatchFrequency;    // conversion to int64_t for the math.
	mfStopwatchCyclesToMillisecondsCoefficient = 1000.f       / (int64_t)mnStopwatchFrequency;
	mfStopwatchCyclesToMicrosecondsCoefficient = 1000000.f    / (int64_t)mnStopwatchFrequency;
	mfStopwatchCyclesToNanosecondsCoefficient  = 1000000000.f / (int64_t)mnStopwatchFrequency;

	#if EASTDC_STOPWATCH_OVERHEAD_ENABLED
		// Here we make a rough measurement of the start and stop overhead of
		// the stopwatch code. It is hard to say what a good way to determine this is,
		// as the runtime use of the stopwatch will actually cause the overhead to 
		// vary somewhat between uses. It is perhaps even debateable whether we
		// should even be attempting to do such overhead calculations.
		uint64_t nCurrentStopwatchCycleValue1;
		uint64_t nCurrentStopwatchCycleValue2;
		uint64_t nLowestStopwatchCycleReadingOverhead(UINT64_MAX);
		uint64_t nCurrentStopwatchCycleReadingOverhead;

		for(int t(0); t < 8; t++)
		{
			nCurrentStopwatchCycleValue1 = EA::StdC::Stopwatch::GetStopwatchCycle();
			nCurrentStopwatchCycleValue2 = EA::StdC::Stopwatch::GetStopwatchCycle();
			nCurrentStopwatchCycleReadingOverhead = nCurrentStopwatchCycleValue2 - nCurrentStopwatchCycleValue1;
			if(nLowestStopwatchCycleReadingOverhead > nCurrentStopwatchCycleReadingOverhead)
				nLowestStopwatchCycleReadingOverhead = nCurrentStopwatchCycleReadingOverhead;
		}
		mnStopwatchCycleReadingOverhead = nLowestStopwatchCycleReadingOverhead;
	#endif

	// CPU Frequencies
	mfCPUCyclesToMinutesCoefficient      = 1.f / 60.f   / (int64_t)mnCPUFrequency;
	mfCPUCyclesToSecondsCoefficient      = 1.f          / (int64_t)mnCPUFrequency;
	mfCPUCyclesToMillisecondsCoefficient = 1000.f       / (int64_t)mnCPUFrequency;
	mfCPUCyclesToMicrosecondsCoefficient = 1000000.f    / (int64_t)mnCPUFrequency;
	mfCPUCyclesToNanosecondsCoefficient  = 1000000000.f / (int64_t)mnCPUFrequency;

	#if EASTDC_STOPWATCH_OVERHEAD_ENABLED
		uint64_t nCurrentCPUCycleValue1, nCurrentCPUCycleValue2;
		uint64_t nLowestCPUCycleReadingOverhead(UINT64_MAX);
		uint64_t nCurrentCPUCycleReadingOverhead;

		for(int c = 0; c < 8; c++)
		{
			nCurrentCPUCycleValue1          = EA::StdC::Stopwatch::GetCPUCycle();
			nCurrentCPUCycleValue2          = EA::StdC::Stopwatch::GetCPUCycle();
			nCurrentCPUCycleReadingOverhead = nCurrentCPUCycleValue2 - nCurrentCPUCycleValue1;

			if(nLowestCPUCycleReadingOverhead > nCurrentCPUCycleReadingOverhead)
				nLowestCPUCycleReadingOverhead = nCurrentCPUCycleReadingOverhead;
		}

		mnCPUCycleReadingOverhead = nLowestCPUCycleReadingOverhead;
	#endif
}


void EAStdCStopwatchSetup()
{
	if(mnStopwatchFrequency <= 1) // If we haven't already calculated this...
	{
		#if defined(EA_PLATFORM_SONY)
			// According to Sony: A time stamp counter exists for each CPU core, but the frequency is the same 
			// value for all CPU cores. This frequency will not change during the lifetime of a process.
			mnCPUFrequency       = sceKernelGetProcessTimeCounterFrequency();
			mnStopwatchFrequency = mnCPUFrequency;

		#elif defined(__APPLE__)
			mach_timebase_info_data_t timebaseInfo;
		
			mach_timebase_info(&timebaseInfo);
			mnCPUFrequency       = (UINT64_C(1000000000) * (uint64_t)timebaseInfo.denom) / (uint64_t)timebaseInfo.numer;

			////////////////////////////////////////////////////
			// Stopwatch Frequency
			mnStopwatchFrequency = mnCPUFrequency;

		#elif defined(EA_PLATFORM_XBOXONE) || defined(CS_UNDEFINED_STRING)
			// Microsoft has stated (https://forums.xboxlive.com/AnswerPage.aspx?qid=c3fcdad5-f3e4-46d9-85f9-d337506f0d6b&tgt=1) that  
			// QueryPerformanceCounter / QueryPerformanceFrequency map to rdtsc and they are stable throughout the life of the process.
			// Thus we can use QueryPerformanceFrequency to portable tell the CPU frequency for our usage of rdtsc.
			QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER*>(&mnStopwatchFrequency));
			mnCPUFrequency = mnStopwatchFrequency;

		#elif defined(EA_PLATFORM_MICROSOFT)
			// On Windows, the only way to tell the CPU-based timer frequency is to manually
			// time it. There is no function in Windows which tells you this information. 
			// Indeed such a function might not be useful for CPU frequency-switching systems.

			// SetPriorityClass / SetThreadPriority APIs not available for Windows 8 Metro apps
			#if EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)
			HANDLE process           = ::GetCurrentProcess();
			DWORD  oldPriorityClass  = ::GetPriorityClass(process);
			HANDLE thread            = ::GetCurrentThread();
			int    oldThreadPriority = ::GetThreadPriority(thread);

			// Set process/thread priorities.
			::SetPriorityClass(process, REALTIME_PRIORITY_CLASS);
			::SetThreadPriority(thread, THREAD_PRIORITY_TIME_CRITICAL);
			#endif


			////////////////////////////////////////////////////
			// Stopwatch Frequency
				////////////////////////////////////////////////////
				// CPU Frequency
				// Unfortunately, you can't call Win32 QueryPerformanceFrequency and
				// expect that it will give you the processor frequency in megahertz.
				// Doing such a thing would work for some CPUs and not others. In fact, 
				// under Windows 2000, it works for my PIII-733 but not for my PII-300.
				// So what we do instead is simply find the ratio of the QueryPerformanceCounter
				// and our Stopwatch::GetCPUCycle functions. 
				mnCPUFrequency = 0;
				for(int i(0); i < 5; i++) // Give N chances at this, in case OS pre-emption occurs.
				{
					uint64_t nQPCCounter; ::QueryPerformanceCounter((LARGE_INTEGER*)&nQPCCounter);
					uint64_t nCPUCounter = EA::StdC::Stopwatch::GetCPUCycle();

					double dRatio = (double)nCPUCounter / (double)nQPCCounter;

					if((dRatio > 0.98 && dRatio < 1.02))
					{
						::QueryPerformanceFrequency((LARGE_INTEGER*)&mnCPUFrequency);
						break;
					}
				}

				if(!mnCPUFrequency)
				{
					// Do our own manual timing of clock ticks.
					// We use Win32 QueryPerformanceCounter and QueryPerformanceFrequency to 
					// calibrate our CPU cycle counter.
					uint64_t nQPFrequency, nQPCCounter1, nQPCCounter2;
					uint64_t nCPUCounter1, nCPUCounter2=0;
					double  dQPCSeconds(0);

					::QueryPerformanceFrequency((LARGE_INTEGER*)&nQPFrequency);
					::QueryPerformanceCounter((LARGE_INTEGER*)&nQPCCounter1);
					nCPUCounter1 = EA::StdC::Stopwatch::GetCPUCycle();

					#if EASTDC_STOPWATCH_FORCE_CPU_CYCLE_USAGE
						static const double TIME_IN_SECONDS_TO_MEASURE_TICKS = 0.300;
					#else
						// Given that ticks are considered to be unreliable we can tolerate lower accuracy measurement
						// of number of ticks per second.  https://en.wikipedia.org/wiki/Time_Stamp_Counter
						// Here the largest limitation will be the MS Perf counter accuracy which is less than 1us.
						static const double TIME_IN_SECONDS_TO_MEASURE_TICKS = 0.005;
					#endif

					while(dQPCSeconds < TIME_IN_SECONDS_TO_MEASURE_TICKS)
					{
						::QueryPerformanceCounter((LARGE_INTEGER*)&nQPCCounter2);
						nCPUCounter2 = EA::StdC::Stopwatch::GetCPUCycle();
						dQPCSeconds  = (nQPCCounter2 - nQPCCounter1) / (double)nQPFrequency;
					}

					mnCPUFrequency = (uint64_t)((nCPUCounter2 - nCPUCounter1) / dQPCSeconds);
				}

			#if EASTDC_STOPWATCH_FORCE_CPU_CYCLE_USAGE
				mnStopwatchFrequency = mnCPUFrequency;
			#else
				::QueryPerformanceFrequency((LARGE_INTEGER*)&mnStopwatchFrequency);
			#endif

			// Reset process/thread priorities.
			#if EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)
			::SetPriorityClass(process, oldPriorityClass);
			::SetThreadPriority(thread, oldThreadPriority);
			#endif
		#else

			////////////////////////////////////////////////////
			// CPU Frequency
			#ifdef EASTDC_CPU_FREQ_CALCULATED
				// On Unix, the only way to tell the CPU-based timer frequency is to manually
				// time it. There is no function in Unix which tells you this information. 
				// Indeed such a function might not be useful for CPU frequency-switching systems.

				uint64_t nTimeCounter1 = EA::StdC::Stopwatch::GetStopwatchCycle();
				uint64_t nCPUCounter1  = EA::StdC::Stopwatch::GetCPUCycle();

				usleep(250000); // Sleep for a ~quarter second.

				uint64_t nCPUCounter2   = EA::StdC::Stopwatch::GetCPUCycle();
				uint64_t nTimeCounter2  = EA::StdC::Stopwatch::GetStopwatchCycle();
				uint64_t nTimeDeltaUs   = nTimeCounter2 - nTimeCounter1;
				uint64_t nCPUDeltaTicks = nCPUCounter2 - nCPUCounter1;

				// GetStopwatchCycle will have varying resolution so we need to account for that accordingly
				#if EASTDC_STOPWATCH_USE_CLOCK_GETTIME
					mnCPUFrequency = (nCPUDeltaTicks * 1000000000 / nTimeDeltaUs); // We are using clock_gettime, which works in nanoseconds.
				#elif EASTDC_STOPWATCH_USE_GETTIMEOFDAY
					mnCPUFrequency = (nCPUDeltaTicks * 1000000 / nTimeDeltaUs); // We are using gettimeofday, which works in microseconds.
				#else
					mnCPUFrequency = (nCPUDeltaTicks * 1000000 / nTimeDeltaUs);
				#endif

				#if EASTDC_STOPWATCH_OVERHEAD_ENABLED
					// To do. Slightly less accuracy without this implemented.
				#endif

			#elif EASTDC_STOPWATCH_USE_CLOCK_GETTIME
				mnCPUFrequency = UINT64_C(1000000000); // We are using clock_gettime, which works in nanoseconds.

			#elif EASTDC_STOPWATCH_USE_GETTIMEOFDAY
				mnCPUFrequency = 1000000;              // We are using gettimeofday, which works in microseconds.
			#else
				#error
				mnCPUFrequency = 1;
			#endif

			////////////////////////////////////////////////////
			// Stopwatch Frequency
			#if EASTDC_STOPWATCH_USE_CLOCK_GETTIME
				mnStopwatchFrequency = UINT64_C(1000000000); // We are using clock_gettime, which works in nanoseconds.
			#elif EASTDC_STOPWATCH_USE_GETTIMEOFDAY
				mnStopwatchFrequency = 1000000;              // We are using gettimeofday, which works in microseconds.
			#else
				mnStopwatchFrequency = mnCPUFrequency;
			#endif
		#endif

		EAStdCStopwatchSetupCoefficients();
	}
}


///////////////////////////////////////////////////////////////////////////////
// EAStdCStopwatchDisableCPUCalibration
//
// This function circumvents the EAStdCStopwatchSetup function for the purpose
// of making it so that EAStdCStopwatchSetup doesn't take a long time to execute
// on startup. If this function is executed before EAStdCStopwatchSetup then
// if EAStdCStopwatchSetup is later executed then it will just immediately exit.
// This function can be considered an alternative to the EAStdCStopwatchSetup 
// function that executes much faster. The downside is that the CPU frequency
// will not be calculated or known and thus the CPU-based timing functions won't
// be available (though the system time-based timing functions will be). If you 
// have a utility app that you need to run which needs to execute quickly and 
// doesn't need CPU-based clock tick timing precision, you might want to use 
// this function. See the EASTDC_STOPWATCH_DISABLE_CPU_CALIBRATION function for
// how to do this. The most important thing is that this function needs to be 
// called before the EAStdCStopwatchSetup function, which may require some 
// compiler/platform-specific trickery as documented with EASTDC_STOPWATCH_DISABLE_CPU_CALIBRATION.
// 
EASTDC_API void EAStdCStopwatchDisableCPUCalibration(uint64_t cpuFrequency)
{
	if(cpuFrequency)
		mnCPUFrequency = cpuFrequency;
	else
		mnCPUFrequency = UINT64_C(2000000000); // We use a moderate guess here of 2GHz. To consider: Make this a very inaccurate value so that it's obvious it's wrong at runtime.

	////////////////////////////////////////////////////
	// Stopwatch Frequency
	#if EASTDC_STOPWATCH_FORCE_CPU_CYCLE_USAGE
		mnStopwatchFrequency = mnCPUFrequency;
	#else
		#if defined(EA_PLATFORM_MICROSOFT)
			::QueryPerformanceFrequency((LARGE_INTEGER*)&mnStopwatchFrequency);
		#else
			// To do.
		#endif
	#endif

	EAStdCStopwatchSetupCoefficients();
}



namespace EA
{
	namespace StdC
	{
		//We have this #if !defined() commented out because we aren't sure that all GCC versions act the same across all platforms.
		//#if !defined(__GNUC__) // GCC uses __attribute__((constructor)) instead of this to call EAStdCStopwatchSetup.
			// AutoStopwatchSetup
			// We create this static class in order to trigger 
			// automatic calling of the code below.
			struct AutoStopwatchSetup
			{
				AutoStopwatchSetup()
					{ EAStdCStopwatchSetup(); }
			};

			AutoStopwatchSetup gAutoStopwatchSetup EA_INIT_PRIORITY(1000);
		//#endif
	}
}



///////////////////////////////////////////////////////////////////////////////
// Stopwatch
///////////////////////////////////////////////////////////////////////////////

EA::StdC::Stopwatch::Stopwatch(int nUnits, bool bStartImmediately)
  : mnStartTime(0),
	mnTotalElapsedTime(0),
	mnUnits(0),
	mfStopwatchCyclesToUnitsCoefficient(1.f)
{
	SetUnits(nUnits);
	if(mnStopwatchFrequency <= 1) // If not already initialized...
		EAStdCStopwatchSetup();
	if(bStartImmediately)
		Start();
}


void EA::StdC::Stopwatch::SetUnits(int nUnits)
{
	mnUnits = nUnits;

	mfStopwatchCyclesToUnitsCoefficient = 1;

	switch (mnUnits)
	{
		case kUnitsCPUCycles:
			mfStopwatchCyclesToUnitsCoefficient = 1; // Timing is reported directly with GetCPUCycle().
			break;

		case kUnitsCycles:
			mfStopwatchCyclesToUnitsCoefficient = 1; // Timing is reported directly with GetStopwatchCycle().
			break;

		case kUnitsNanoseconds:
			mfStopwatchCyclesToUnitsCoefficient = mfStopwatchCyclesToNanosecondsCoefficient;
			break;

		case kUnitsMicroseconds:
			mfStopwatchCyclesToUnitsCoefficient = mfStopwatchCyclesToMicrosecondsCoefficient;
			break;

		case kUnitsMilliseconds:
			mfStopwatchCyclesToUnitsCoefficient = mfStopwatchCyclesToMillisecondsCoefficient;
			break;

		case kUnitsSeconds:
			mfStopwatchCyclesToUnitsCoefficient = mfStopwatchCyclesToSecondsCoefficient;
			break;

		case kUnitsMinutes:
			mfStopwatchCyclesToUnitsCoefficient = mfStopwatchCyclesToMinutesCoefficient;
			break;
	}
}


void EA::StdC::Stopwatch::Stop()
{
	if(mnStartTime) // Check to make sure the stopwatch is actually running
	{
		// Note that below we compare the elapsed time (from the most recent start
		// to the this stop) to sStopwatchCycleReadingOverhead. For most timing situations,
		// the elapsed time will be *much* greater than the overhead. For some 
		// cases (e.g. timing 10-20 lines of C code) the elapsed time will be only 
		// 3-10 times the value of sStopwatchCycleReadingOverhead. In these cases, the 
		// value of subtracting this overhead may be useful. For some cases the 
		// code being timed is so small or brief that sStopwatchCycleReadingOverhead may
		// actually come out to be higher than the stretch of code. If this is the
		// case, you really don't want to be trying to time this code with a 
		// softare-based stopwatch. What we do is simply set the elapsed time to a 
		// small value such as 1, in order for code that is using this stopwatch to at
		// least believe that something is happening.

		const uint64_t nCurrentTime(mnUnits == kUnitsCPUCycles ? GetCPUCycle() : GetStopwatchCycle());

		// EA_ASSERT(nCurrentTime >= mnStartTime); We might want to enable this, at least for modern platforms.

		#if EASTDC_STOPWATCH_OVERHEAD_ENABLED
			const uint64_t nElapsedTime(nCurrentTime - mnStartTime);

			if(nElapsedTime > mnStopwatchCycleReadingOverhead)
				mnTotalElapsedTime += nElapsedTime - mnStopwatchCycleReadingOverhead;
			else
				mnTotalElapsedTime += 1; // We pretend that just one stopwatch cycle has elapsed.
		#else
			mnTotalElapsedTime += (nCurrentTime - mnStartTime);
		#endif

		mnStartTime = 0;
	}
}


uint64_t EA::StdC::Stopwatch::GetElapsedTime() const
{
	uint64_t nFinalTotalElapsedTime64(mnTotalElapsedTime);

	if(mnStartTime) // We we are currently running, then take into account time passed since last start.
	{

		uint64_t nCurrentTime;

		// See the 'Stop' function for an explanation of the code below.
		if(mnUnits == kUnitsCPUCycles)
			nCurrentTime = GetCPUCycle();
		else
			nCurrentTime = GetStopwatchCycle();

		uint64_t nElapsed = nCurrentTime - mnStartTime;

		// EA_ASSERT(nCurrentTime >= mnStartTime); We might want to enable this, at least for modern platforms.

		#if EASTDC_STOPWATCH_OVERHEAD_ENABLED
			const uint64_t nElapsedTime = nElapsed;

			if(nElapsedTime > mnStopwatchCycleReadingOverhead)
				nFinalTotalElapsedTime64 += nElapsedTime - mnStopwatchCycleReadingOverhead;
			else
				nFinalTotalElapsedTime64 += 1; // We pretend that just one stopwatch cycle has elapsed.
		#else
			nFinalTotalElapsedTime64 += nElapsed;
		#endif

	} // Now nFinalTotalElapsedTime64 holds the elapsed time in stopwatch cycles. 

	if(mfStopwatchCyclesToUnitsCoefficient == 0) // If the stopwatch was started before the global EAStdCStopwatchSetup function was executed...
	{
		// We can recover from this by calling SetUnits here, which will re-read the global setup results.
		const_cast<Stopwatch*>(this)->SetUnits(mnUnits);
		// EA_ASSERT(mfStopwatchCyclesToUnitsCoefficient != 0);
	}

	// We are doing a float to int cast here, which is a relatively slow operation on some CPUs.
	return (uint64_t)((nFinalTotalElapsedTime64 * mfStopwatchCyclesToUnitsCoefficient) + 0.49999f);
}




void EA::StdC::Stopwatch::SetElapsedTime(uint64_t nElapsedTime)
{
	if(IsRunning()) 
		Restart();

	// Concern: The division here is not lightning fast and also is subject to precision error.
	mnTotalElapsedTime = (uint64_t)((nElapsedTime / mfStopwatchCyclesToUnitsCoefficient) + 0.49999f);
}


float EA::StdC::Stopwatch::GetElapsedTimeFloat() const
{
	uint64_t nFinalTotalElapsedTime64(mnTotalElapsedTime);

	if(mnStartTime){ //We we are currently running, then take into account time passed since last start.

		uint64_t nCurrentTime;

		// See the 'Stop' function for an explanation of the code below.
		if(mnUnits == kUnitsCPUCycles)
			nCurrentTime = GetCPUCycle();
		else
			nCurrentTime = GetStopwatchCycle();

		uint64_t nElapsed = nCurrentTime - mnStartTime;

		// EA_ASSERT(nCurrentTime >= mnStartTime); We might want to enable this, at least for modern platforms.

		#if EASTDC_STOPWATCH_OVERHEAD_ENABLED
			const uint64_t nElapsedTime = nElapsed;

			if(nElapsedTime > mnStopwatchCycleReadingOverhead)
				nFinalTotalElapsedTime64 += nElapsedTime - mnStopwatchCycleReadingOverhead;
			else
				nFinalTotalElapsedTime64 += 1; // We pretend that just one stopwatch cycle has elapsed.
		#else
			nFinalTotalElapsedTime64 += nElapsed;
		#endif

	} // Now nFinalTotalElapsedTime64 holds the elapsed time in stopwatch cycles. 

	// We are doing a float to int cast here, which is a relatively slow operation on some CPUs.
	return (float)(nFinalTotalElapsedTime64 * mfStopwatchCyclesToUnitsCoefficient);
}


void EA::StdC::Stopwatch::SetElapsedTimeFloat(float fElapsedTime)
{
	if(IsRunning()) 
		Restart();

	// Concern: The division here is not lightning fast and also is subject to precision error.
	mnTotalElapsedTime = (uint64_t)(fElapsedTime / mfStopwatchCyclesToUnitsCoefficient);
}


float EA::StdC::Stopwatch::GetUnitsPerStopwatchCycle(Units units)
{
	switch (units)
	{
		case kUnitsNanoseconds:
			return mfStopwatchCyclesToNanosecondsCoefficient;

		case kUnitsMicroseconds:
			return mfStopwatchCyclesToMicrosecondsCoefficient;

		case kUnitsMilliseconds:
			return mfStopwatchCyclesToMillisecondsCoefficient;

		case kUnitsSeconds:
			return mfStopwatchCyclesToSecondsCoefficient;

		case kUnitsMinutes:
			return mfStopwatchCyclesToMinutesCoefficient;

		case kUnitsCycles:
		case kUnitsCPUCycles:
		case kUnitsUserDefined:
		default:
			break;
	}

	return 1;
}


float EA::StdC::Stopwatch::GetUnitsPerCPUCycle(Units units)
{
	switch (units)
	{
		case kUnitsNanoseconds:
			return mfCPUCyclesToNanosecondsCoefficient;

		case kUnitsMicroseconds:
			return mfCPUCyclesToMicrosecondsCoefficient;

		case kUnitsMilliseconds:
			return mfCPUCyclesToMillisecondsCoefficient;

		case kUnitsSeconds:
			return mfCPUCyclesToSecondsCoefficient;

		case kUnitsMinutes:
			return mfCPUCyclesToMinutesCoefficient;

		case kUnitsCycles:
		case kUnitsCPUCycles:
		case kUnitsUserDefined:
		default:
			break;
	}

	return 1;
}


// This function is here instead of inlined in the associated header file
// because it uses mnStopwatchFrequency, which is a variable local to this 
// source file.
uint64_t EA::StdC::Stopwatch::GetStopwatchFrequency()
{
	return mnStopwatchFrequency;
}


// This function is here instead of inlined in the associated header file
// because it uses mnStopwatchFrequency, which is a variable local to this 
// source file.
uint64_t EA::StdC::Stopwatch::GetCPUFrequency()
{
	return mnCPUFrequency;
}



///////////////////////////////////////////////////////////////////////////////
// LimitStopwatch
///////////////////////////////////////////////////////////////////////////////

void EA::StdC::LimitStopwatch::SetTimeLimit(uint64_t nLimit, bool bStartImmediately)
{
	const uint64_t nCurrentTime = GetStopwatchCycle();

	mnEndTime = nCurrentTime + (uint64_t)(nLimit / mfStopwatchCyclesToUnitsCoefficient);

	if(bStartImmediately)
		Start();
}


float EA::StdC::LimitStopwatch::GetTimeRemainingFloat() const
{
	const uint64_t nCurrentTime = GetStopwatchCycle();

	const float fTimeRemaining = (float)((int64_t)(mnEndTime - nCurrentTime)) * mfStopwatchCyclesToUnitsCoefficient;

	return fTimeRemaining;
}














