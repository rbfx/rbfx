///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Implements a suite of functionality to manage dates, times, and calendars.
///////////////////////////////////////////////////////////////////////////////


#include <EAStdC/internal/Config.h>
#include <EAStdC/EADateTime.h>
#include <EAStdC/EAString.h>
#include <EAStdC/EASprintf.h>
#include <EAStdC/EACType.h>
#include <EAStdC/EAStopwatch.h>
#include <EAStdC/EAMemory.h>
#include <float.h>
#include <math.h>
#include <string.h>
#include <EAAssert/eaassert.h>

#if EASTDC_TIME_H_AVAILABLE
	#include <time.h>
#else
	static tm sTm;

	time_t time(time_t*)
	{
		return 0;
	}

	tm* gmtime(const time_t*)
	{
		return &sTm;
	}

	tm* localtime(const time_t*)
	{
		return &sTm;
	}

#endif

#if EASTDC_LOCALE_H_AVAILABLE
	#include <locale.h>
#endif

#if   defined(EA_PLATFORM_APPLE)
	#include <mach/mach.h>
	#include <mach/mach_time.h>
#endif

#if defined(EA_PLATFORM_MICROSOFT)
	#if defined(_MSC_VER)
		#pragma warning(push, 0)
	#endif
	#if   (defined(EA_PLATFORM_WINDOWS) && !EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP))
		#include <Windows.h>
	#else
		#include <WinSock2.h>
	#endif
	#if defined(_MSC_VER)
		#pragma warning(pop)
	#endif

	// Microsoft has a GetLocalTime function, but it isn't available for all platform build targets. 
	// The following is the most Microsoft-portable workaround version we can come up with.
	void GetLocalTimeAlternative(SYSTEMTIME* pSystemTime)
	{
		#if defined(EA_PLATFORM_WINDOWS) || defined(EA_PLATFORM_WINDOWS_PHONE)
			GetLocalTime(pSystemTime);
		#else
			// Get current date/time
			FILETIME time, localTime;

			::GetSystemTimeAsFileTime(&time);
			::FileTimeToLocalFileTime(&time, &localTime);
			::FileTimeToSystemTime(&localTime, pSystemTime);
		#endif
	}

	#if EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)
		bool EAStdCGetDateFormat(DWORD dwFlags, const SYSTEMTIME* lpDate, char* lpDateStr, size_t cchDate)
		{
			// size_t cchDate is unsigned and 32 or 64 bit so needs to be cast to pass to GetDateFormat
			return (GetDateFormatA(LOCALE_USER_DEFAULT, dwFlags, lpDate, NULL, lpDateStr, static_cast<int>(cchDate)) != 0);
		}
		bool EAStdCGetTimeFormat(DWORD dwFlags, const SYSTEMTIME* lpTime, char* timeStr, size_t cchTime)
		{
			// size_t cchTime is unsigned and 32 or 64 bit so needs to be cast to pass to GetTimeFormat
			return (GetTimeFormatA(LOCALE_USER_DEFAULT, dwFlags, lpTime, NULL, timeStr, static_cast<int>(cchTime)) != 0);
		}

		int EAStdCGetLocaleInfo(LCTYPE lcType, char* lcData, size_t cchData)
		{
			// size_t cchData is unsigned and 32 or 64 bit so needs to be cast to pass to GetLocaleInfo
			return GetLocaleInfoA(LOCALE_USER_DEFAULT, lcType, lcData, static_cast<int>(cchData));
		}
	#else
		bool EAStdCGetDateFormat(DWORD dwFlags, const SYSTEMTIME* lpDate, char* dateStr, size_t cchDate)
		{
			wchar_t* temp = static_cast<wchar_t*>(EAAlloca(cchDate * sizeof(wchar_t)));
			const bool res = GetDateFormatEx(LOCALE_NAME_USER_DEFAULT, dwFlags, lpDate, NULL, temp, static_cast<int>(cchDate), NULL) != 0;
			EA::StdC::Strlcpy(dateStr, temp, cchDate);
			return res;
		}

		bool EAStdCGetTimeFormat(DWORD dwFlags, const SYSTEMTIME* lpTime, char* timeStr, size_t cchTime)
		{
			wchar_t* temp = static_cast<wchar_t*>(EAAlloca(cchTime * sizeof(wchar_t)));
			const bool res = GetTimeFormatEx(LOCALE_NAME_USER_DEFAULT, dwFlags, lpTime, NULL, temp, static_cast<int>(cchTime)) != 0;
			EA::StdC::Strlcpy(timeStr, temp, cchTime);
			return res;
		}

		int EAStdCGetLocaleInfo(LCTYPE lcType, char* lcData, size_t cchData)
		{
			wchar_t* temp = lcData ? static_cast<wchar_t*>(EAAlloca(cchData * sizeof(wchar_t))) : nullptr;
			const int res = GetLocaleInfoEx(LOCALE_NAME_USER_DEFAULT, lcType, temp, static_cast<int>(cchData));
			if (lcData)
				EA::StdC::Strlcpy(lcData, temp, cchData);
			return res;
		}
	#endif
#else
	typedef struct _FILETIME
	{
		uint32_t dwLowDateTime;
		uint32_t dwHighDateTime;
	} FILETIME;

	typedef struct _SYSTEMTIME
	{
		uint16_t wYear;
		uint16_t wMonth;
		uint16_t wDayOfWeek;
		uint16_t wDay;
		uint16_t wHour;
		uint16_t wMinute;
		uint16_t wSecond;
		uint16_t wMilliseconds;
	} SYSTEMTIME;
#endif


#ifdef _MSC_VER
	#pragma warning(push)
	#pragma warning(disable: 4365) // 'argument' : conversion from 'int' to 'uint32_t', signed/unsigned mismatch
#endif


namespace EA
{
namespace StdC
{

	// static table containing number of days for each month
	static const uint32_t kDaysInMonth[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
	
	// static table containing total number of days within a year up to a given month
	static const uint32_t  kDaysInYear[26]  = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365,   // for regular years
												0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366 }; // for leap years


	// macro for computing number of leap years that occured up to the specified year
	#define EADT_COUNT_LEAP_YEARS(Y) (((Y - 1) / 4) - ((Y - 1) / 100) + ((Y - 1) / 400))


	#ifndef EASTDC_ABS
		#define EASTDC_ABS(x) (x >= 0 ? x : -x)
	#endif




	///////////////////////////////////////////////////////////////////////
	// GetTime
	//
	// Returns nanoseconds since January 1, 1970.
	// There are 584 years worth of nanoseconds that can be stored in a uint64_t.
	// See the declaration for a more precise specification.
	//
	EASTDC_API uint64_t GetTime()
	{
		// This code is thread-unsafe for the case that the first ever call to GetTime 
		// occurs by two threads at the same time, and/or there is a memory view latency
		// between two CPUs that call this. The fix is probably to use a formal atomic in 
		// initializing the stopwatch and sSystemTimeDiffAtCapture variables.
		static EA::StdC::Stopwatch sStopwatch(Stopwatch::kUnitsNanoseconds, true);
		static uint64_t            sInitialTime = 0;
		uint64_t                   t = sStopwatch.GetElapsedTime();

		if(sInitialTime == 0) // If this is the first time this function is called...
		{
			timeval tv;
			GetTimeOfDay(&tv, NULL, true); // Returns time in the form of seconds/useconds since 1970.
			sInitialTime = (uint64_t)((tv.tv_sec * UINT64_C(1000000000)) + (tv.tv_usec * UINT64_C(1000)));
		}

		return sInitialTime + t;
	}

	///////////////////////////////////////////////////////////////////////
	// GetTimeMilliseconds
	//
	// Convenient wrapper for converting the nanosecond resolution of GetTime()
	// to milliseconds.
	EASTDC_API uint64_t GetTimeMilliseconds()
	{
		return GetTime() / 1000000;
	}

	#if 0 // Alternative variation, which on the surface seems more efficient.
	/*
	EASTDC_API uint64_t GetTime()
	{
		static struct TimeWatch
		{
			EA::StdC::Stopwatch stopwatch;
			uint64_t initialTime;
		
			TimeWatch() :
				stopwatch(Stopwatch::kUnitsNanoseconds, true)
			{
				timeval tv;
				GetTimeOfDay(&tv, NULL, true);
				initialTime = (uint64_t)((tv.tv_sec * UINT64_C(1000000000)) + (tv.tv_usec * UINT64_C(1000)));
			}
		} sTimeWatch;

		return sTimeWatch.initialTime + sTimeWatch.stopwatch.GetElapsedTime();
	}
	*/
	#endif


	///////////////////////////////////////////////////////////////////////
	// GetTimePrecision
	//
	// Returns the precision of the GetTime and GetTimeOfDay functions.
	//
	EASTDC_API uint64_t GetTimePrecision()
	{
		#if   defined(EA_PLATFORM_MICROSOFT)
			return 100;                         // 100-nanosecond precision. 

		#elif defined(EA_PLATFORM_APPLE) || defined(__APPLE__) || defined(EA_PLATFORM_UNIX) || defined(EA_PLATFORM_IPHONE)
			return 1000;                        // Microsecond precision.
 
		#else
			return UINT64_C(1000000000);        // Second-level precision.

		#endif
	}



	///////////////////////////////////////////////////////////////////////////
	// DateTimeParameters
	///////////////////////////////////////////////////////////////////////////

	DateTimeParameters::DateTimeParameters()
	{
		EA_COMPILETIME_ASSERT(kDateTimeIgnored == 0xffffffff);
		memset(this, 0xff, sizeof(DateTimeParameters));
	}




	///////////////////////////////////////////////////////////////////////////
	// DateTime
	///////////////////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////////////
	// GetParameter
	//
	// Gets the given parameter. If you want to get the year, 
	// you would call GetParameter(kParameterYear).
	//
	uint32_t DateTime::GetParameter(Parameter parameter) const
	{
		uint32_t result = 0;

		switch (parameter)
		{
			// Refers to full year value, such as 1994, 2006, etc. but not 
			// a two digit value such as 94 or 04. The valid range is 0 - INT_MAX.      
			case kParameterYear:                 
			{
				// compute total number of days
				const int64_t nDays = (mnSeconds / kSecondsPerDay);

				// compute the year
				const int64_t leapYearCount = EADT_COUNT_LEAP_YEARS(nDays / 365);
				result = 1 + (uint32_t)((nDays - 1 - leapYearCount) / 365);

				break;
			}

			// Refers to month of year, starting with 1 for January. The valid range is 1 - 12.
			case kParameterMonth:    
			{
				// get all the data we need
				const uint32_t nYear      = GetParameter(kParameterYear);
				const uint32_t nDayOfYear = GetParameter(kParameterDayOfYear);

				// check if the year is a leap year
				const bool bLeap = IsLeapYear(nYear);   

				// compute the month
				for(uint32_t nMonth = kMonthJanuary; nMonth <= kMonthDecember; nMonth++)
				{
					if(nDayOfYear <= kDaysInYear[nMonth + (13 * bLeap)])
					{
						result = nMonth;
						break;
					}
				}

				if(result == 0)
					result = kMonthJanuary;

				break;
			}

			// Refers to a day of the year, starting with 1 for January 1st.
			// The valid range is 1 - 366.
			case kParameterDayOfYear:
			{
				// get the year
				const uint32_t nYear = GetParameter(kParameterYear);

				// compute total number of days
				const int64_t nDays = (mnSeconds / kSecondsPerDay);

				// compute the day of the year
				const uint32_t leapYearCount = EADT_COUNT_LEAP_YEARS(nYear);
				result = (uint32_t)(nDays - (((nYear - 1) * 365) + leapYearCount));

				if(result == 0)
					result = 1;

				break;
			}

			// Refers to the day of the month, starting with 1 for the first 
			// of the month. The valid range is 1 - 31.
			case kParameterDayOfMonth:
			{
				// get all the data we need
				const uint32_t nYear      = GetParameter(kParameterYear);
				const uint32_t nMonth     = GetParameter(kParameterMonth);
				const uint32_t nDayOfYear = GetParameter(kParameterDayOfYear);

				// get the day of the month
				const int isLeapYear = IsLeapYear(nYear) ? 1 : 0;   
				result = nDayOfYear - kDaysInYear[(nMonth - 1) + (13 * isLeapYear)];

				break;
			}

			// Refers to the day of the week, starting with 1 for Sunday. 
			// The valid range is 1-7.
			case kParameterDayOfWeek:
			{
				// compute total number of days
				const int64_t nDays = (mnSeconds / kSecondsPerDay);

				// compute the day of the week
				result = 1 + (uint32_t)(nDays % 7);

				break;
			}

			// Refers to the hour of a day in 24 hour format, starting 
			// with 0 for midnight. The valid range is 0-23.
			case kParameterHour:
				result = (uint32_t)((mnSeconds / kSecondsPerHour) % 24);
				break;

			// Refers to the minute of the hour, starting with 0 for 
			// the first minute. The valid range is 0-59.
			case kParameterMinute:
				result = (uint32_t)((mnSeconds / kSecondsPerMinute) % 60);
				break;

			// Refers to the second of the minute, starting with 0 
			// for the first second. The valid range is 0-59.
			case kParameterSecond:
				result = (uint32_t)(mnSeconds % 60);
				break;

			case kParameterNanosecond:
				result = mnNanosecond;
				break;

			// Refers to the week of the year, starting with 1 for the 
			// week of January 1. The valid range is 1-52.
			case kParameterWeekOfYear:
			{
				const uint32_t nDayOfYear = GetParameter(kParameterDayOfYear);
				result = 1 + (nDayOfYear - 1) / 7;

				break;
			}

			// Refers to the week of the month, starting with 1 for the 
			// first week. The valid range is 1-5.
			case kParameterWeekOfMonth:
			{
				const uint32_t nDayOfMonth = GetParameter(kParameterDayOfMonth);
				result = 1 + (nDayOfMonth - 1) / 7;

				break;
			}

			case kParameterUnknown:
			default:
				break; // This removes compiler warnings about unused cases.
		}

		return result;
	}



	///////////////////////////////////////////////////////////////////////
	// SetParameter
	//
	// Sets the given parameter. If you want to set the year to 1999, 
	// you would call SetParameter(kParameterYear, 1999).
	//
	void DateTime::SetParameter(Parameter parameter, uint32_t nValue)
	{
		switch (parameter)
		{
			// straight-forward calls to set
			case kParameterYear:
				Set(nValue,        kValueIgnored, kValueIgnored, kValueIgnored, kValueIgnored, kValueIgnored, kValueIgnored);
				break; 
			
			case kParameterMonth:
				Set(kValueIgnored, nValue,        kValueIgnored, kValueIgnored, kValueIgnored, kValueIgnored, kValueIgnored);
				break; 

			case kParameterDayOfMonth:
				Set(kValueIgnored, kValueIgnored, nValue,        kValueIgnored, kValueIgnored, kValueIgnored, kValueIgnored);
				break;

			case kParameterHour:
				Set(kValueIgnored, kValueIgnored, kValueIgnored, nValue,        kValueIgnored, kValueIgnored, kValueIgnored);
				break;

			case kParameterMinute:
				Set(kValueIgnored, kValueIgnored, kValueIgnored, kValueIgnored, nValue,        kValueIgnored, kValueIgnored);
				break;

			case kParameterSecond:
				Set(kValueIgnored, kValueIgnored, kValueIgnored, kValueIgnored, kValueIgnored, nValue       , kValueIgnored);
				break;

			case kParameterNanosecond:
				Set(kValueIgnored, kValueIgnored, kValueIgnored, kValueIgnored, kValueIgnored, kValueIgnored, nValue        );
				break;

			// set the day of the year
			case kParameterDayOfYear:
			{
				// update the total number of seconds by the offset from the current day of the year
				const uint32_t nDayOfYear = GetParameter(kParameterDayOfYear);
				mnSeconds += (int32_t)(nValue - nDayOfYear) * kSecondsPerDay;
				break;
			}

			// set the day of the week
			case kParameterDayOfWeek:
			{
				// make sure the new value is valid
				if((nValue >= kDayOfWeekSunday) && (nValue <= kDayOfWeekSaturday))
				{
					// update the total number of seconds by the offset from the current day of the week
					const uint32_t nDayOfWeek = GetParameter(kParameterDayOfWeek);
					mnSeconds += (int32_t)(nValue - nDayOfWeek) * kSecondsPerDay;
				}
				else
				{
					EA_FAIL_M("EAStdC DateTime");
				}
				break;
			}

			// set the week of the year or month
			case kParameterWeekOfYear:
				// Fall through.
			case kParameterWeekOfMonth:
			{
				// update the total number of seconds by the offset from the current week of the year
				const uint32_t nParameterValue = GetParameter(parameter);
				mnSeconds += (int32_t)(nValue - nParameterValue) * 7 * kSecondsPerDay;
				break;
			}

			case kParameterUnknown:
			default:
				break; // This removes compiler warnings about unused cases.
		}
	}



	///////////////////////////////////////////////////////////////////////
	// Set
	//
	// Sets the time and date based on various inputs. If any input is 
	// kValueIgnored, then the input is ignored and the current value is used. 
	// If any of the cyclic inputs is beyond its valid range, the modulo
	// of the value is used and the division of the value is added to the 
	// next higher bracket. For example, if the input nMinute is 65, then 
	// the minute used is 5 and 1 is added to the current hour value.
	//
	void DateTime::Set(uint32_t nYear, uint32_t nMonth,  uint32_t nDayOfMonth, 
					   uint32_t nHour, uint32_t nMinute, uint32_t nSecond, uint32_t nNanosecond)
	{
		if(nYear       == kValueIgnored || !nYear) 
			nYear       = GetParameter(kParameterYear);
		if(nMonth      == kValueIgnored || !nMonth) 
			nMonth      = GetParameter(kParameterMonth);
		if(nDayOfMonth == kValueIgnored || !nDayOfMonth) 
			nDayOfMonth = GetParameter(kParameterDayOfMonth);
		if(nHour       == kValueIgnored) 
			nHour       = GetParameter(kParameterHour);
		if(nMinute     == kValueIgnored) 
			nMinute     = GetParameter(kParameterMinute);
		if(nSecond     == kValueIgnored) 
			nSecond     = GetParameter(kParameterSecond);
		if(nNanosecond == kValueIgnored) 
			nNanosecond = mnNanosecond;

		// wrap the month value
		if(nMonth > 12)
		{
			nYear +=  (nMonth - 1) / 12;
			nMonth = ((nMonth - 1) % 12) + 1;
		}

		// compute total number of days for the given year adding all leap days
		const uint32_t leapYearCount = EADT_COUNT_LEAP_YEARS(nYear);
		int64_t nDays = ((nYear - 1) * 365) + leapYearCount;

		// add month and day
		const int isLeapYear = (IsLeapYear(nYear) ? 1 : 0);
		nDays += kDaysInYear[(nMonth - 1) + (13 * isLeapYear)] + (nDayOfMonth - 0); // Should this -0 be -1?

		// convert the number of days to seconds
		mnSeconds = nDays * kSecondsPerDay;

		// add current time
		mnSeconds += nHour   * kSecondsPerHour;
		mnSeconds += nMinute * kSecondsPerMinute;
		mnSeconds += nSecond;

		mnSeconds += (nNanosecond / 1000000000);

		mnNanosecond = (nNanosecond % 1000000000);
	}



	///////////////////////////////////////////////////////////////////////
	// Set
	//
	// Sets the time based on the current time. If the timeFrame is 
	// kTimeFrameUTC, the time is set to what the current UTC time is, 
	// which will be a fixed number of hours off of what the current 
	// local time is.
	//
	// We have the option of not reading nanoseconds, because reading nanoseconds means calling the GetTimeOfDay
	// function, and the GetTimeOfDay function needs to indirectly call SetTimeZoneBias, which in turn
	// calls Set. So we need to break the possibility of an infinite loop, and we can do that by having 
	// SetTimeZoneBias call SetInternal(timeFrame, *false*); SetTimeZoneBias doesn't need the nanosecond 
	// precision anyway.
	//
	void DateTime::Set(TimeFrame timeFrame, bool bSetNanoseconds)
	{
		#if   defined(EA_PLATFORM_MICROSOFT)
			SYSTEMTIME s;

			if(timeFrame == kTimeFrameLocal)
				GetLocalTimeAlternative(&s);
			else
				GetSystemTime(&s);

			Set(s.wYear,                                            // wYear: 1601 through 30827.
				s.wMonth,                                           // wMonth: 1 through 12.
				s.wDay,                                             // wDay: 1 through 31.
				s.wHour,                                            // wHour: 0 through 23.
				s.wMinute,                                          // wMinute: 0 through 59.
				s.wSecond,                                          // wSecond: 0 through 59.
				bSetNanoseconds ? s.wMilliseconds * 1000000 : 0);   // wMilliseconds: 0 through 999.
		#else
			const  time_t    nTime  = time(NULL);
			struct tm* const pTime  = ((timeFrame == kTimeFrameUTC) ? gmtime(&nTime) : localtime(&nTime));
			struct tm  const tmCopy = *pTime; // Need to make a copy because calling external code below could change what pTime points to, given that pTime comes from a static pointer inside the C Standard Library.

			timeval tv;
			if(bSetNanoseconds)
				GetTimeOfDay(&tv, NULL, (timeFrame == kTimeFrameUTC));
			else
				tv.tv_usec = 0;

			Set(tmCopy.tm_year + 1900, tmCopy.tm_mon + kMonthJanuary, tmCopy.tm_mday, tmCopy.tm_hour, tmCopy.tm_min, tmCopy.tm_sec, (uint32_t)(tv.tv_usec * 1000));
		#endif
	}


	///////////////////////////////////////////////////////////////////////
	// Compare
	//
	// This function compares this object with another DateTime object 
	// and returns an integer result. The return value is the same as with 
	// the strcmp string comparison function. If this object is less than 
	// the argument dateTime, the return value is < 0. Comparison operators 
	// are defined outside this class, though they use the Compare function 
	// to do their work.
	//
	int DateTime::Compare(const DateTime& dateTime, bool bCompareDate, bool bCompareTime) const
	{
		// Note: I believe the code below which just does / and % is valid. The reason it wouldn't 
		// be valid is if some days have more seconds than others. Yes there are leap seconds that 
		// occur every few years, but leap seconds don't affect the recorded time of day; they reflect
		// the measurement of time passage in the real world. Also, leap years (which are in fact a
		// way in which the recorded days in a year differ) don't affect this, because they add whole
		// days as opposed to fractional ones. Daylight savings time doesn't affect this because
		// it too doesn't affect recorded time but rather affects real world exerpeience only.
		// Time zones don't affect this calculation, as this function assumes that both DateTime 
		// objects being compared are within the same time zone.

		bool bCompareNanoseconds = true;  // Until proven false below.

		// this is what we will be comparing
		int64_t nValueA = mnSeconds;
		int64_t nValueB = dateTime.GetSeconds();

		if(bCompareDate && !bCompareTime)
		{
			// Date only - compare the total number of days.
			// Make nValueA and nValueB be days instead of seconds.
			nValueA = nValueA / kSecondsPerDay;
			nValueB = nValueB / kSecondsPerDay;
			bCompareNanoseconds = false;
		}
		else if(!bCompareDate && bCompareTime)
		{
			// Time of day only - extract the time portion of the total seconds value.
			// Make nValueA and nValueB be seconds since the start of the day instead of absolute time seconds.
			nValueA = nValueA % kSecondsPerDay;
			nValueB = nValueB % kSecondsPerDay;
		}
		// else compare both date and time.

		if(bCompareNanoseconds && (nValueA == nValueB))
		{
			nValueA = mnNanosecond;
			nValueB = dateTime.mnNanosecond;
		}

		if(nValueA == nValueB) 
			return 0;
		else if(nValueA < nValueB)
			return -1;
		return 1;
	}



	///////////////////////////////////////////////////////////////////////
	// AddTime
	//
	// Allows you to increment (or decrement) the given parameter by the given amount.
	//
	void DateTime::AddTime(Parameter parameter, int64_t nValue)
	{
		switch (parameter)
		{
			case kParameterYear:     
				SetParameter(kParameterYear, (uint32_t)(GetParameter(kParameterYear) + nValue));
				break;

			case kParameterMonth:      
			{
				// first - compute new year
				uint32_t nYear = (uint32_t)(GetParameter(kParameterYear) + (nValue / 12));

				// now - how many months do we have left?
				nValue = nValue % 12;

				// compute new month value
				const uint32_t nMonth = GetParameter(kParameterMonth);
				nValue = (int64_t)nMonth + nValue;

				// did we cross a year boundary ?
				if(nValue < 1)
				{
					nYear--;
					nValue = nValue + 12; // compute new month value
				}
				else if(nValue > 12)
				{
					nYear++;
					nValue = nValue - 12; // compute new month value
				}

				// set the new year and month
				Set(nYear, (uint32_t)(uint64_t)nValue, kValueIgnored, kValueIgnored, kValueIgnored, kValueIgnored);

				break;
			}

			// There really is no difference between the handling of the following 
			// 3 parameter types - each offsets the date by a given number of days.
			case kParameterDayOfMonth: 
			case kParameterDayOfYear:
			case kParameterDayOfWeek:
				mnSeconds += (int64_t)(nValue * kSecondsPerDay);
				break;

			case kParameterHour:      
				mnSeconds += (int64_t)(nValue * kSecondsPerHour);
				break;

			case kParameterMinute:    
				mnSeconds += (int64_t)(nValue * kSecondsPerMinute);
				break;

			case kParameterSecond:
				mnSeconds += nValue;
				break;

			case kParameterNanosecond:
			{
				int64_t newNanoseconds = GetParameter(kParameterNanosecond) + nValue;
				int64_t addedSeconds   = (newNanoseconds / 1000000000);
				newNanoseconds %= 1000000000;

				EA_ASSERT(addedSeconds < INT64_C(0xffffffff));
				AddTime(kParameterSecond, (uint32_t)addedSeconds);

				EA_ASSERT(newNanoseconds < INT64_C(0xffffffff));
				SetParameter(kParameterNanosecond, (uint32_t)newNanoseconds);

				break;
			}

			// kParameterWeekOfYear and kParameterWeekOfMonth act the same 
			// for this: each offsets the date by a given number of weeks
			case kParameterWeekOfYear:
				// Fall through
			case kParameterWeekOfMonth:
				mnSeconds += (int64_t)(nValue * 7 * kSecondsPerDay);
				break;

			case kParameterUnknown:
			default:
				break; // This removes compiler warnings about unused cases.
		}

		EA_ASSERT(mnSeconds >= 0); // Verify that the operation didn't cause integer wraparound.
		if(mnSeconds < 0)
			mnSeconds = 0;
	}


	int64_t DateTime::GetSeconds() const { return mnSeconds; }
	void DateTime::SetSeconds(int64_t nSeconds) { mnSeconds = nSeconds; }


	uint64_t DateTime::GetMilliseconds() const { return (uint64_t)mnSeconds * 1000 + mnNanosecond / 1000000; }
	void DateTime::SetMilliseconds(uint64_t milliseconds)
	{
		mnSeconds = milliseconds / 1000;
		mnNanosecond = (milliseconds % 1000) * 1000000;
	}


	EA::StdC::int128_t DateTime::GetNanoseconds() const
	{
		return (EA::StdC::int128_t(mnSeconds) * 1000000000) + mnNanosecond;
	}

	void DateTime::SetNanoseconds(const EA::StdC::int128_t& nanoseconds)
	{
		EA::StdC::int128_t seconds = nanoseconds / 1000000000;
		EA::StdC::int128_t nanosecond = nanoseconds % 1000000000;

		mnSeconds = seconds.AsInt64();
		mnNanosecond = nanosecond.AsUint32();
	}


	///////////////////////////////////////////////////////////////////////
	// IsLeapYear
	//
	// Returns true if the given year is a leap year.
	// Algorithm from K & R, "The C Programming Language", 1st ed.
	//
	EASTDC_API bool IsLeapYear(uint32_t nYear)
	{
		return (!(nYear & 3) && (nYear % 100)) || !(nYear % 400);       
		
		// Alternative:
		//if((nYear % 4) == 0)
		//{
		//    if((nYear % 100) == 0)
		//        return ((nYear % 400) == 0);
		//    else
		//        return true;
		//}
		//return false;
	}



	///////////////////////////////////////////////////////////////////////
	// GetDaysInYear
	//
	// Returns the number of days in the given year. The value will vary 
	// based on whether the year is a leap year or not.
	//
	EASTDC_API uint32_t GetDaysInYear(uint32_t nYear)
	{
		return IsLeapYear(nYear) ? (uint32_t)366 : (uint32_t)365;
	}


	///////////////////////////////////////////////////////////////////////
	// GetDaysInMonth
	//
	// Implemented by Blazej Stompel
	//
	// Returns the number of days in the given month. The value will vary 
	// based on the month and based on whether the year is a leap year. 
	// The input nMonth takes one of enum Month or an integer equivalent.
	//
	EASTDC_API uint32_t GetDaysInMonth(uint32_t nMonth, uint32_t nYear)
	{
		// Make sure the month value is valid
		if((nMonth >= kMonthJanuary) && (nMonth <= kMonthDecember))
		{
			// Special case for leap years
			if(nMonth == kMonthFebruary)
			{
				const bool isLeapYear = IsLeapYear(nYear);

				if(isLeapYear)
					return kDaysInMonth[nMonth - 1] + 1;
			}

			return kDaysInMonth[nMonth - 1];
		}

		return 0;
	}


	///////////////////////////////////////////////////////////////////////
	// GetDayOfYear
	//
	// The input nMonth takes one of enum Month or an integer equivalent.
	// The input nDayOfMonth takes an integer consistent with enum DayOfMonth.
	//
	EASTDC_API uint32_t GetDayOfYear(uint32_t nMonth, uint32_t nDayOfMonth, uint32_t nYear)
	{
		const DateTime sDateTime(nYear, nMonth, nDayOfMonth, 0, 0, 0);

		return sDateTime.GetParameter(kParameterDayOfYear);
	}




	// Code to regenerate the kEpochSeconds array.
	//
	// kEpochSeconds[kEpochJulian]         = DateTime((uint32_t)-4712,  1,  1, 12,  0,  0).GetSeconds(); // -4712/01/01/12:00:00
	// kEpochSeconds[kEpochModifiedJulian] = DateTime((uint32_t) 1858, 11, 17,  0,  0,  0).GetSeconds(); //  1858/11/17/00:00:00
	// kEpochSeconds[kEpochGregorian]      = DateTime((uint32_t) 1752,  9, 14,  0,  0,  0).GetSeconds(); //  1752/09/14/00:00:00
	// kEpochSeconds[kEpoch1900]           = DateTime((uint32_t) 1900,  1,  1,  0,  0,  0).GetSeconds(); //  1900/01/01/00:00:00
	// kEpochSeconds[kEpoch1950]           = DateTime((uint32_t) 1950,  1,  1,  0,  0,  0).GetSeconds(); //  1950/01/01/00:00:00
	// kEpochSeconds[kEpoch1970]           = DateTime((uint32_t) 1970,  1,  1,  0,  0,  0).GetSeconds(); //  1970/01/01/00:00:00
	// kEpochSeconds[kEpoch2000]           = DateTime((uint32_t) 2000,  1,  1,  0,  0,  0).GetSeconds(); //  2000/01/01/00:00:00
	// kEpochSeconds[kEpochJ2000]          = DateTime((uint32_t) 2000,  1,  1, 11, 58, 55).GetSeconds(); //  2000/01/01/11:58:55
	// kEpochSeconds[kEpochDateTime]       = 0;

	static int64_t kEpochSeconds[10] = 
	{
		INT64_C(0),                 // kEpochUnknown
		INT64_C(89839426968000),    // kEpochJulian          Began at -4712/01/01/12:00:00 (Year 1858, January 1, noon).
		INT64_C(55278460800),       // kEpochGregorian       Began at  1752/09/14/00:00:00 (Year 1752, January 1, midnight). Beginning of the Gregorian calendar.
		INT64_C(58628966400),       // kEpochModifiedJulian  Began at  1858/11/17/00:00:00 (Year 1858, January 1, midnight). 2,400,000.5 days after Julian epoch began.
		INT64_C(59926694400),       // kEpoch1900            Began at  1900/01/01/00:00:00 (Year 1900, January 1, midnight). Same epoch used by the Network Time Protocol.
		INT64_C(61504531200),       // kEpoch1950            Began at  1950/01/01/00:00:00 (Year 1950, January 1, midnight). Used by some gaming systems.
		INT64_C(62135683200),       // kEpoch1970            Began at  1970/01/01/00:00:00 (Year 1970, January 1, midnight). Same epoch used by C runtime library and Unix OS.
		INT64_C(63082368000),       // kEpoch2000            Began at  2000/01/01/00:00:00 (Year 2000, January 1, midnight). Same epoch used by Apple file systems.
		INT64_C(63082411135),       // kEpochJ2000           Began at  2000/01/01/11:58:55 (Year 2000, January 1, ~noon).    Coordinated Universal Time, also includes 816 milliseconds.
		INT64_C(0),                 // kEpochDateTime        Began at  0000/01/01/00:00:00 (Year 0000, January 1, midnight).
	};


	///////////////////////////////////////////////////////////////////////
	// ConvertEpochSeconds
	//
	EASTDC_API int64_t ConvertEpochSeconds(Epoch srcEpoch, int64_t srcSeconds, Epoch destEpoch)
	{
		if((srcEpoch < kEpochCount) && (destEpoch < kEpochCount))
			return (srcSeconds + kEpochSeconds[srcEpoch]) - kEpochSeconds[destEpoch];

		return 0;
	}


	///////////////////////////////////////////////////////////////////////
	// GetCurrent
	//
	// Returns the current year, month, hour, etc.
	//
	EASTDC_API uint32_t GetCurrent(Parameter parameter, TimeFrame timeFrame)
	{
		const DateTime sDateTime(timeFrame);

		return sDateTime.GetParameter(parameter);
	}


	///////////////////////////////////////////////////////////////////////
	// IsDST
	//
	// Returns true if the time is daylight savings time. This function 
	// assumes that DST is valid for the given current locale; some locales 
	// within the United States don't observe DST.
	//
	EASTDC_API bool IsDST()
	{
		time_t nTime = time(NULL);
		struct tm* const pTime = localtime(&nTime); // Find out if the local time is in DST

		return pTime->tm_isdst > 0;
	}


	///////////////////////////////////////////////////////////////////////
	// IsDSTDateTime
	//
	EASTDC_API bool IsDSTDateTime(int64_t dateTimeSeconds)
	{
		// DateTime seconds, based on 0000/01/01/00:00:00 (Year 0000, January 1, midnight) (not time_t).
		int64_t timeTSeconds = DateTimeSecondsToTimeTSeconds(dateTimeSeconds);
		time_t  time = (time_t)timeTSeconds; // For some platforms this might result in a 64 to 32 bit chop, though the list bits should be all zero.

		struct tm* const pTime = localtime(&time);
		return pTime->tm_isdst > 0;
	}


	///////////////////////////////////////////////////////////////////////
	// GetDaylightSavingsBias
	//
	// Returns the number of seconds that the current time is daylight 
	// savings adjusted from the conventional time. Adding this value 
	// to the conventional time yields the time when adjusted for 
	// daylight savings. Some locations implement daylight savings offsets
	// that are a half hour instead of an hour. We ignore these, as they 
	// are uncommon and problematic.
	//
	EASTDC_API int64_t GetDaylightSavingsBias()
	{  
		return 3600;
	}



	///////////////////////////////////////////////////////////////////////
	// GetTimeZoneBias
	//
	// Returns the number of seconds that the local time zone is off of UTC.
	// Adding this value to the current UTC yields the current local time.
	// For locales in the United states, this is usually a negative number
	// like -28800 (8 hours behind UTC). For locales East of Europe it will
	// be a positive value (hours ahead of UTC).
	//
	EASTDC_API int64_t GetTimeZoneBias()
	{
		#if defined(EA_PLATFORM_WINDOWS)
			TIME_ZONE_INFORMATION tz;            
			DWORD rv = GetTimeZoneInformation(&tz);
			EA_ASSERT(rv != TIME_ZONE_ID_INVALID);
			EA_UNUSED(rv);

			return (tz.Bias * -60);

		#elif defined(EA_PLATFORM_APPLE)
			// http://linux.die.net/man/2/gettimeofday
			struct timeval  tv;
			struct timezone tz;
			gettimeofday(&tv, &tz);
			return (tz.tz_minuteswest * -60);

		#elif defined(EA_PLATFORM_ANDROID) && __ANDROID_API__ >= 8
			tzset();
			return -timezone; // Seconds West of GMT

		// Disabled until we can verify these are correct on their respective machines:
		//#elif defined(EA_PLATFORM_LINUX)
		//    // http://linux.die.net/man/3/daylight
		//    return timezone;

		#elif defined(EA_PLATFORM_UNIX)
			// BSD doesn't support the timezone parameter of gettimeofday. 
			// However, we can deduce the time zone offset by taking an properly-chosen GM time_t 
			// value and convert it with both localtime() and gmtime() and see what the seconds difference is.
			const time_t jan3rd1970 = (60 * 60 * 24 * 2);
			struct tm    tmGM;

			gmtime_r(&jan3rd1970, &tmGM);
			time_t tLocal = mktime(&tmGM);
			return (jan3rd1970 - tLocal); // This will be a negative number like -28800 (PST time zone).

		#elif EASTDC_UTC_TIME_AVAILABLE
			DateTime sDateTimeLocal(0);
			DateTime sDateTimeUTC(0);
			int64_t  nSecondsLocal;
			int64_t  nSecondsUTC;
			bool     bIsDST;
			
			// The reason the logic below exists is because this platform don't provide functions 
			// to get the time zone bias, but often let you infer it because they provide 
			// functions to tell the local time and the UTC time. One possibility is the
			// case that the second turned over between the two time readings.

			//Debug code.
			//#if defined(EA_PLATFORM_MICROSOFT)
			//    _putenv_s("TZ", "GST-1GDT");
			//    _tzset();
			//#endif

			sDateTimeLocal.Set(kTimeFrameLocal, false); // Intentionally use 'false' here so that SetInternal doesn't call GetTimeOfDay, which can recursively call us back here and loop indefinitely.
			bIsDST = IsDST(); // Intentionally call this in between the two Set calls.
			sDateTimeUTC.Set(kTimeFrameUTC, false);

			nSecondsLocal = sDateTimeLocal.GetSeconds();
			nSecondsUTC   = sDateTimeUTC.GetSeconds();

			// These seconds should be an even number of minutes apart. If they aren't then 
			// the second must have turned over between the two Set calls above. So we detect
			// that and handle it.
			const int64_t difference      = (nSecondsUTC > nSecondsLocal) ? (nSecondsUTC - nSecondsLocal) : (nSecondsLocal - nSecondsUTC);
			const int64_t differenceMod60 = difference % 60; // Usually this will be zero.

			if(differenceMod60)
			{
				if(nSecondsUTC > nSecondsLocal)
					nSecondsUTC -= differenceMod60;
				else
					nSecondsUTC -= (60 - differenceMod60);
			}

			if(bIsDST)
				nSecondsLocal -= 3600;

			return (nSecondsLocal - nSecondsUTC);
		#else
			return 0;
		#endif
	}


	///////////////////////////////////////////////////////////////////////
	// GetTimeZoneName
	//
	EASTDC_API bool GetTimeZoneName(char* pName, bool bDaylightSavingsName)
	{
		#if defined(EA_PLATFORM_MICROSOFT) && defined(_MSC_VER) && (_MSC_VER >= 1400)
			EA_COMPILETIME_ASSERT(EASTDC_UTC_TIME_AVAILABLE == 1);

			size_t  size   = 0;                        // "The supplied pName must have a capacity of at least 8 bytes."
			errno_t result = _get_tzname(&size, pName, kTimeZoneNameCapacity, bDaylightSavingsName ? 1 : 0);
			return (result == 0);

		#elif defined(EA_PLATFORM_UNIX) && EASTDC_UNIX_TZNAME_AVAILABLE
			EA_COMPILETIME_ASSERT(EASTDC_UTC_TIME_AVAILABLE == 1); // If this assertion fails then EASTDC_UTC_TIME_AVAILABLE is 0 whereas it really could be 1.

			const char* pTZName = tzname[bDaylightSavingsName ? 1 : 0];
			Strncpy(pName, pTZName, kTimeZoneNameCapacity); // "The supplied pName must have a capacity of at least 8 bytes."
			pName[7] = 0;
			return true;

		#else
			EA_UNUSED(bDaylightSavingsName);

			#if EASTDC_UTC_TIME_AVAILABLE
				int64_t b = GetTimeZoneBias();
				Snprintf(pName, kTimeZoneNameCapacity, "+%6lld", b);
			#else
				Strlcpy(pName, "LT", kTimeZoneNameCapacity); // Local Time. This is merely our convention for when we have no time zone information other than to say it's the local time.
			#endif

			return true;
		#endif
	}


	///////////////////////////////////////////////////////////////////////
	// DateTimeToTm
	//
	// Converts a DateTime to a C tm struct.
	//
	EASTDC_API void DateTimeToTm(const DateTime& dateTime, tm& time)
	{
		// time doesn't have a field for anything more precise than seconds, so kParameterNanosecond is irrelevant, as we don't apply rounding to nanoseconds.
		time.tm_sec   = (int)dateTime.GetParameter(kParameterSecond);
		time.tm_min   = (int)dateTime.GetParameter(kParameterMinute);
		time.tm_hour  = (int)dateTime.GetParameter(kParameterHour);
		time.tm_mday  = (int)dateTime.GetParameter(kParameterDayOfMonth);
		time.tm_mon   = (int)dateTime.GetParameter(kParameterMonth) - kMonthJanuary;
		time.tm_year  = (int)dateTime.GetParameter(kParameterYear) - 1900;
		time.tm_wday  = (int)dateTime.GetParameter(kParameterDayOfWeek) - kDayOfWeekSunday;
		time.tm_yday  = (int)dateTime.GetParameter(kParameterDayOfYear) - 1;
		time.tm_isdst = 0; // We don't have a way to tell if an arbitrary dateTime object is daylight savings time. 
	}


	///////////////////////////////////////////////////////////////////////
	// TmToDateTime
	//
	// Converts a C tm struct to a DateTime.
	//
	EASTDC_API void TmToDateTime(const tm& time, DateTime& dateTime)
	{
		dateTime.Set((uint32_t)(time.tm_year + 1900), (uint32_t)(time.tm_mon + kMonthJanuary), 
					 (uint32_t)time.tm_mday, (uint32_t)time.tm_hour, (uint32_t)time.tm_min, (uint32_t)time.tm_sec);
	}


	///////////////////////////////////////////////////////////////////////
	// DateTimeToFileTime
	//
	// Converts a DateTime to a FILETIME struct.
	// A FILETIME contains a 64-bit value representing the number of 
	// 100-nanosecond intervals since January 1, 1601 (UTC).
	//
	EASTDC_API void DateTimeToFileTime(const DateTime& dateTime, _FILETIME& time)
	{
		_SYSTEMTIME systemTime;
		DateTimeToSystemTime(dateTime, systemTime); 

		#if defined(EA_PLATFORM_MICROSOFT)
			SystemTimeToFileTime(&systemTime, &time); // OS call.
		#else
			int64_t month, year;

			if(systemTime.wMonth >= 3) // If month is after a leap year day could occur)...
			{
				month = systemTime.wMonth + 1;
				year  = systemTime.wYear;
			} 
			else
			{
				month = systemTime.wMonth + 13;
				year  = systemTime.wYear - 1;
			}

			const int64_t endOfCenturyLeapYearCount = ((3 * (year / 100) + 3) / 4);   // http://en.wikipedia.org/wiki/Century_leap_year

			// Convert the dateTime value to 100 ns intervals since Jan 1, 1601.
			const int64_t day = ((36525 * year) / 100) - endOfCenturyLeapYearCount +
								 ((1959 * month) / 64) + systemTime.wDay - 584817;   // Subtract 584817 to make the time based on 1601-01-01.

			const int64_t time64 = ((((day                       * kHoursPerDay      + 
									   systemTime.wHour)         * kMinutesPerHour   + 
									   systemTime.wMinute)       * kSecondsPerMinute + 
									   systemTime.wSecond)       * 1000              +     // 1000 = milliseconds per second.
									   systemTime.wMilliseconds) * 10000;                  // 10000 == (100 ns intervals per millisecond).

			time.dwLowDateTime  = (uint32_t)time64;
			time.dwHighDateTime = (uint32_t)(time64 >> INT64_C(32));

		#endif
	}



	///////////////////////////////////////////////////////////////////////
	// FileTimeToDateTime
	//
	// Converts a FILETIME struct to a DateTime.
	//
	EASTDC_API void FileTimeToDateTime(const _FILETIME& time, DateTime& dateTime)
	{
		#if defined(EA_PLATFORM_MICROSOFT)
			_SYSTEMTIME systemTime;
			FileTimeToSystemTime(&time, &systemTime);   // OS call.
			SystemTimeToDateTime(systemTime, dateTime);
		#else
			// A FILETIME contains a 64-bit value representing the number of 
			// 100-nanosecond intervals since January 1, 1601 (UTC).
			EA_UNUSED(time);
			memset(&dateTime, 0, sizeof(dateTime));

			// To do: Implement this.
		#endif
	}



	///////////////////////////////////////////////////////////////////////
	// DateTimeToSystemTime
	//
	// Converts a DateTime to a SYSTEMTIME struct.
	//
	EASTDC_API void DateTimeToSystemTime(const DateTime& dateTime, _SYSTEMTIME& time)
	{
		time.wYear          = (uint16_t) dateTime.GetParameter(kParameterYear);
		time.wMonth         = (uint16_t) dateTime.GetParameter(kParameterMonth);
		time.wDayOfWeek     = (uint16_t)(dateTime.GetParameter(kParameterDayOfWeek) - 1);
		time.wDay           = (uint16_t) dateTime.GetParameter(kParameterDayOfMonth);
		time.wHour          = (uint16_t) dateTime.GetParameter(kParameterHour);
		time.wMinute        = (uint16_t) dateTime.GetParameter(kParameterMinute);
		time.wSecond        = (uint16_t) dateTime.GetParameter(kParameterSecond);
		time.wMilliseconds  = (uint16_t)(dateTime.GetParameter(kParameterNanosecond) / 1000000);
	}


	///////////////////////////////////////////////////////////////////////
	// SystemTimeToDateTime
	//
	// Converts a SYSTEMTIME struct to a DateTime.
	//
	EASTDC_API void SystemTimeToDateTime(const _SYSTEMTIME& time, DateTime& dateTime)
	{
		dateTime = DateTime(time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond);
		dateTime.SetParameter(kParameterNanosecond, time.wMilliseconds * 1000000);
	}


	///////////////////////////////////////////////////////////////////////
	// GetTimeOfDay
	//
	// This behaves the same as the Posix gettimeofday function, with the 
	// addition that pTZ is formally supported and that bUTC specifies to 
	// get the time of day in UTC instead of local time.
	// pTZ is an output parameter and it's input value has no affect on the
	// function behavior and return value.
	// Note that a timeval has the same meaning as time_t except that it contains
	// sub-second information.
	//
	EASTDC_API int GetTimeOfDay(timeval* pTV, timezone_* pTZ, bool bUTC)
	{
		timezone_ tz;

		if(!pTZ)
			pTZ = &tz;

		#if defined(EA_PLATFORM_MICROSOFT)
			#if EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)
				static bool tzsetCalled = false;

				if(!tzsetCalled)
				{
					_tzset();
					tzsetCalled = true;
				}
			#endif

			if(pTV)
			{
				FILETIME      ft;
				LARGE_INTEGER li;
				int64_t       t;

				GetSystemTimeAsFileTime(&ft);       // Microsoft call. The information is in Coordinated Universal Time (UTC) format.

				li.LowPart  = ft.dwLowDateTime;
				li.HighPart = ft.dwHighDateTime;

				t  = li.QuadPart;                   // In 100-nanosecond intervals
				t -= INT64_C(116444736000000000);   // Offset to the Epoch time
				t /= 10;                            // In microseconds

				pTV->tv_sec  = (long)(t / 1000000);
				pTV->tv_usec = (long)(t % 1000000);

				if(!bUTC) // If should convert to local time...
					pTV->tv_sec -= _timezone;
			}

			pTZ->tz_minuteswest = _timezone / 60;  // _timezone is a positive value, as opposed to the negative value that 'time zone bias' is.
			pTZ->tz_dsttime     = _daylight;

			return 0;

		#elif EASTDC_CLOCK_GETTIME_AVAILABLE

			timeval tv;

			if(!pTV) // Unix gettimeofday requires a valid timeval pointer.
				pTV = &tv;

			timespec ts;
			int result = clock_gettime(CLOCK_REALTIME, &ts);

			// To do: Handle time zone. See Unix code below.
			memset(pTZ, 0, sizeof(timezone_));

			if((result == 0) && pTV) // If OK and if the user specified a pTV to fill in...
			{
				// Convert timespec to timeval. They are similar except for the tv_usec/tv_nsec member.
				pTV->tv_sec  = ts.tv_sec;
				pTV->tv_usec = (suseconds_t)(ts.tv_nsec / 1000);

				if(!bUTC) // If should convert to local time...
					pTV->tv_sec -= ((pTZ->tz_minuteswest * 60) - (pTZ->tz_dsttime ? 3600 : 0));
			}

			return result;

		#elif defined(EA_PLATFORM_UNIX) || defined(EA_PLATFORM_IPHONE)

			timeval tv;

			if(!pTV) // Unix gettimeofday requires a valid timeval pointer.
				pTV = &tv;

			int result = gettimeofday(pTV, pTZ); // The gettimeofday function obtains the current time, expressed as seconds and microseconds since 00:00 Coordinated Universal Time (UTC), January 1, 1970

			#if defined(EA_PLATFORM_LINUX) && !defined(EA_PLATFORM_ANDROID) // pTZ parameter is not always supported on linux platforms. timezone not defined on some Android devices (i.e. Sony Xperia X10i).
				pTZ->tz_minuteswest = timezone / 60; //timezone is seconds west of GMT
				time_t nowtm = pTV->tv_sec;
				tm tmResult;
				tm* ptmResult;
				ptmResult = localtime_r(&nowtm, &tmResult);
				result = result != 0 ? result : !(ptmResult == &tmResult); //if gettimeofday failed return that error code, otherwise return 1 if localtime_r returned wrong pointer, otherwise 0
				pTZ->tz_dsttime = tmResult.tm_isdst;
			#endif            

			if((result == 0) && pTV) // If OK and if the user specified a pTV to fill in...
			{
				if(!bUTC) // If should convert to local time...
					pTV->tv_sec -= ((pTZ->tz_minuteswest * 60) - (pTZ->tz_dsttime ? 3600 : 0));
			}

			return result;

		#else

			// We implement the most basic version which is likely to compile: 
			// use just the time_t function and don't support time zones.

			pTZ->tz_minuteswest = (int)GetTimeZoneBias() / -60; // tz_minuteswest is a positive value in the US, as opposed to the negative value that 'time zone bias' is.
			pTZ->tz_dsttime     = 0; // Assume there is never daylight savings time.

			if(pTV)
			{
				pTV->tv_sec  = time(NULL); // The time function shall return the value of time in seconds since the Epoch (00:00 Coordinated Universal Time (UTC), January 1, 1970). 
				pTV->tv_usec = 0;

				if(!bUTC) // If should convert to local time...
					pTV->tv_sec -= ((pTZ->tz_minuteswest * 60) - (pTZ->tz_dsttime ? 3600 : 0));
			}
			return 0;
		#endif
	}


	///////////////////////////////////////////////////////////////////////
	// TimevalDifference
	//
	// Calculates the result of TVA - TVB.
	// Returns 1 if TVA >= TVB, 0 if TVA == TVB, -1 if TVA < TVB. Much like strcmp().
	// Note that a timeval has the same meaning as time_t except that it contains
	// sub-second information.
	//
	EASTDC_API int TimevalDifference(const timeval* pTVA, const timeval* pTVB, timeval* pTVResult)
	{
		timeval tva(*pTVA);
		timeval tvb(*pTVB);

		// Perform the carry for the later subtraction by updating y.
		if(tva.tv_usec < tvb.tv_usec)
		{
			const int nsec = ((tvb.tv_usec - tva.tv_usec) / 1000000) + 1;

			tvb.tv_usec -= 1000000 * nsec;
			tvb.tv_sec  += nsec;
		}

		if((tva.tv_usec - tvb.tv_usec) > 1000000)
		{
			const int nsec = (tvb.tv_usec - tva.tv_usec) / 1000000;

			tvb.tv_usec += 1000000 * nsec;
			tvb.tv_sec  -= nsec;
		}

		// Compute the time remaining to wait. tv_usec is always positive.
		pTVResult->tv_sec  = tva.tv_sec  - tvb.tv_sec;
		pTVResult->tv_usec = tva.tv_usec - tvb.tv_usec;

		if(tva.tv_sec == tvb.tv_sec)
		{
			if(tva.tv_usec == tvb.tv_usec)
				return 0;

			return (tva.tv_usec > tvb.tv_usec) ? 1 : -1;
		}

		return (tva.tv_sec > tvb.tv_sec) ? 1 : -1;
	}


	namespace Internal
	{
		#define SUNDAY_BASED_WEEK_NUMBER(pTM)  (((pTM)->tm_yday + 7 - ((pTM)->tm_wday)) / 7)
		#define MONDAY_BASED_WEEK_NUMBER(pTM)  (((pTM)->tm_yday + 7 - ((pTM)->tm_wday ? (pTM)->tm_wday - 1 : 6)) / 7)


		static const TimeLocale gDefaultTimeLocale = 
		{
			{ "Sun","Mon","Tue","Wed","Thu","Fri","Sat" },
			{ "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" },
			{ "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" },
			{ "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December" },
			{ "AM", "PM" },
			"%a %b %d %H:%M:%S %Y",
			"%m/%d/%y",
			"%H:%M:%S",
			"%I:%M:%S %p"
		};


		static bool Append(const char* EA_RESTRICT p, char* EA_RESTRICT & pTimeString, size_t& capacity)
		{
			for(; capacity; ++pTimeString, --capacity)
			{
				if((*pTimeString = *p++) == 0)
					return true;
			}

			return false;
		}


		static bool WriteInt(int n, int digits, char pad, bool removeLeadingZeroes, char* EA_RESTRICT & pTimeString, size_t& capacity) 
		{
			char  buffer[10];
			char* p;

			buffer[9] = 0;
			for(p = buffer + 8; (n > 0) && (p > buffer); n /= 10, --digits)
				*p-- = (char)((n % 10) + '0');

			while((p > buffer) && (digits-- > 0))
				*p-- = (char)pad;

			if(removeLeadingZeroes)
			{
				while((p[1] == '0') || (p[1] == ' '))
					++p;

				if (p[1] == 0)
					--p;
			}

			return Append(++p, pTimeString, capacity);
		}


		#if defined(EA_PLATFORM_WINDOWS)
			#define EASTDC_WINDOWS_TIME_FUNCTIONS_AVAILABLE 1   // The Microsoft API supports these functions only for desktop target platforms (e.g. Win32, Win64).
		#else
			#define EASTDC_WINDOWS_TIME_FUNCTIONS_AVAILABLE 0
		#endif

		#if !EASTDC_WINDOWS_TIME_FUNCTIONS_AVAILABLE
			// This function converts any "%" chars in the format string to "%#", writing the new format to the supplied buffer.
			// Returns NULL for the only possible error condition: pBuffer's capacity was insufficient.
			static char* ConvertFormatSpecifiersToAlternates(char* pBuffer, size_t bufferCapacity, const char* pFormat)
			{
				char* pBufferCurrent = pBuffer;
				char* pBufferEnd     = pBuffer + (bufferCapacity - 2); // -2 because we can write two chars per loop below. If bufferCapacity is < 2 then the loop below will quit right away, which is what we want.

				while(*pFormat && (pBufferCurrent < pBufferEnd))
				{
					*pBufferCurrent++ = *pFormat;

					if(*pFormat++ == '%')
						*pBufferCurrent++ = '#';
				}

				*pBufferCurrent = 0;

				return (*pFormat == 0) ? pBuffer : NULL;

			}
		#endif
	}


	// Posix alternative formats:
	// "Some conversion specifiers can be modified by the E or O modifier characters 
	//  to indicate that an alternative format or specification should be used rather 
	//  than the one normally used by the unmodified conversion specifier. If the 
	//  alternative format or specification does not exist for the current locale, 
	//  (see ERA in the XBD specification, Section 5.3.5) the behaviour will be as if 
	//  the unmodified conversion specification were used."
	//
	// Microsoft alternative formats:
	// Also, Microsoft (alternatively to Posix  E and O) supports using the # char 
	// after % to indicate alternative behaviour as follows:
	//     %#a, %#A, %#b, %#B, %#h, %#p, %#X, %#z, %#Z, %#%             # flag is ignored.
	//     %#c                                                          Long date and time representation, appropriate for current locale. For example: "Tuesday, March 14, 1995, 12:41:29".
	//     %#x                                                          Long date representation, appropriate to current locale. For example: "Tuesday, March 14, 1995".
	//     %#d, %#H, %#I, %#j, %#m, %#M, %#S, %#U, %#w, %#W, %#y, %#Y   Remove leading zeros (if any).

	EASTDC_API size_t Strftime(char* EA_RESTRICT pTimeString, size_t timeStringCapacity, 
							   const char* EA_RESTRICT pFormat, const tm* EA_RESTRICT pTM, const TimeLocale* EA_RESTRICT pTimeLocale)
	{
		using namespace Internal;

		size_t capacity = timeStringCapacity;
		bool   bGMT     = false; // To consider: provide a way for the user to set this to true or to directly specify the time zone.
		char   buffer[256];

		if(!pTimeLocale)
			pTimeLocale = &gDefaultTimeLocale;

		for(; *pFormat; ++pFormat)
		{
			if(*pFormat == '%')
			{
				char cAlt = 0;  // Alternative format specifier.

				if((*++pFormat == 'E') || (*pFormat == 'O') || (*pFormat == '#'))
					cAlt = *pFormat++;

				switch(*pFormat)
				{
					case '\0':      // We are at the end of the string, with a (not valid) trailing '%' char.
						EA_FAIL_M("EAStdC Strftime");  // Incomplete format specifier.
						--pFormat;
						break;

					case '%':   // %% is replaced by a % char.
						break;

					case 'a': // is replaced by the locale's abbreviated weekday name. 
						if((pTM->tm_wday < 0) || (pTM->tm_wday > 6))
							return 0;

						if(!Append(pTimeLocale->mAbbrevDay[pTM->tm_wday], pTimeString, capacity))
							return 0;
						continue;

					case 'A': // is replaced by the locale's full weekday name. 
						if((pTM->tm_wday < 0) || (pTM->tm_wday > 6))
							return 0;

						if(!Append(pTimeLocale->mDay[pTM->tm_wday], pTimeString, capacity))
							return 0;
						continue;

					case 'b': // is replaced by the locale's abbreviated month name. 
					case 'h':
						if((pTM->tm_mon < 0) || (pTM->tm_mon > 11))
							return 0;

						if(!Append(pTimeLocale->mAbbrevMonth[pTM->tm_mon], pTimeString, capacity))
							return 0;
						continue;

					case 'B': // is replaced by the locale's full month name. 
						if((pTM->tm_mon < 0) || (pTM->tm_mon > 11))
							return 0;

						if(!Append(pTimeLocale->mMonth[pTM->tm_mon], pTimeString, capacity))
							return 0;
						continue;

					case 'c': // is replaced by the locale's appropriate date and time representation.
					{
						#if EASTDC_WINDOWS_TIME_FUNCTIONS_AVAILABLE
							const SYSTEMTIME systemTime = { WORD(pTM->tm_year + 1900), WORD(pTM->tm_mon + 1), WORD(pTM->tm_wday), WORD(pTM->tm_mday), WORD(pTM->tm_hour), WORD(pTM->tm_min), WORD(pTM->tm_sec), 0 };
							const uint32_t   dateFormat = (cAlt == '#') ? DATE_SHORTDATE : DATE_LONGDATE;
							const char       empty[2]   = { ' ', 0 };

							if(!EAStdCGetDateFormat(dateFormat, &systemTime, buffer, sizeof buffer) || !Append(buffer, pTimeString, capacity) || !Append(empty, pTimeString, capacity))
								return 0;
							if(!EAStdCGetTimeFormat(0, &systemTime, buffer, sizeof buffer) || !Append(buffer, pTimeString, capacity))
								return 0;
						#else
							char formatBuffer[256]; formatBuffer[0] = 0;
							const char* pFormatTemp = pTimeLocale->mDateTimeFormat;

							if (cAlt == '#')
								pFormatTemp = ConvertFormatSpecifiersToAlternates(formatBuffer, sizeof formatBuffer, pFormatTemp);

							const size_t len = Strftime(pTimeString, capacity, pFormatTemp, pTM);
							if(!len)
								return 0;
							pTimeString += len;
							capacity    -= len;
						#endif
						continue;
					}

					case 'C': // is replaced by the century number (the year divided by 100 and truncated to an integer) as a decimal number [00-99]. 
					{
						int year = (pTM->tm_year + 1900) / 100;
						if(year == 0 && cAlt == '#')
						{
							if(!WriteInt(year, 1, '0', false, pTimeString, capacity))
								return 0;
						}
						else
						{
							if(!WriteInt(year, 2, '0', cAlt == '#', pTimeString, capacity))
								return 0;
						}
						continue;
					}

					case 'd': // is replaced by the day of the month as a decimal number [01,31]. 
						if(!WriteInt(pTM->tm_mday, 2, '0', cAlt == '#', pTimeString, capacity))
							return 0;
						continue;

					case 'D': // same as %m/%d/%y. 
					{
						const size_t len = Strftime(pTimeString, capacity, "%m/%d/%y", pTM);
						if(!len)
							return 0;
						pTimeString += len;
						capacity    -= len;
						continue;
					}

					case 'e': // is replaced by the day of the month as a decimal number [1,31]; a single digit is preceded by a space. 
						if(!WriteInt(pTM->tm_mday, 2, ' ', cAlt == '#', pTimeString, capacity))
							return 0;
						continue;
					
					case 'F': // same as %Y-%m-%d 
					{
						const size_t len = Strftime(pTimeString, capacity, "%Y-%m-%d", pTM);
						if(!len)
							return 0;
						pTimeString += len;
						capacity    -= len;
						continue;
					}

					case 'g': //Replaced by the last 2 digits of the week-based year (see below) as a decimal number [00,99]
					{
						//Unsupported as of yet
						continue;
					}

					case 'G': //Replaced by the week-based year (see below) as a decimal number (for example, 1977).
					{
						//Unsupported as of yet
						continue;
					}

					case 'H': // is replaced by the hour (24-hour clock) as a decimal number [00,23]. 
						if(!WriteInt(pTM->tm_hour, 2, '0', cAlt == '#', pTimeString, capacity))
							return 0;
						continue;

					case 'I': // is replaced by the hour (12-hour clock) as a decimal number [01,12]. 
						if(!WriteInt((pTM->tm_hour % 12) ? (pTM->tm_hour % 12) : 12, 2, '0', cAlt == '#', pTimeString, capacity))
							return 0;
						continue;

					case 'j': // is replaced by the day of the year as a decimal number [001,366]. 
						if(!WriteInt(pTM->tm_yday + 1, 3, '0', cAlt == '#', pTimeString, capacity))
							return 0;
						continue;

					case 'M': // is replaced by the minute as a decimal number [00,59]. 
						if(!WriteInt(pTM->tm_min, 2, '0', cAlt == '#', pTimeString, capacity))
							return 0;
						continue;

					case 'm': // is replaced by the month as a decimal number [01,12]. 
						if(!WriteInt(pTM->tm_mon + 1, 2, '0', cAlt == '#', pTimeString, capacity))
							return 0;
						continue;

					case 'n': // is replaced by a newline character.
						buffer[0] = '\n';
						buffer[1] = 0;
						if(!Append(buffer, pTimeString, capacity))
							return 0;
						continue;

					case 'p': // is replaced by the locale's equivalent of either a.m. or p.m. 
						if(!Append(pTimeLocale->mAmPm[pTM->tm_hour >= 12], pTimeString, capacity))
							return 0;
						continue;

					case 'r': // is replaced by the time in a.m. and p.m. notation; in the POSIX locale this is equivalent to %I:%M:%S %p. 
					{
						const size_t len = Strftime(pTimeString, capacity, pTimeLocale->mTimeFormatAmPm, pTM);
						if(!len)
							return 0;
						pTimeString += len;
						capacity    -= len;
						continue;
					}

					case 'R': // is replaced by the time in 24 hour notation (%H:%M). 
					{
						const size_t len = Strftime(pTimeString, capacity, "%H:%M", pTM);
						if(!len)
							return 0;
						pTimeString += len;
						capacity    -= len;
						continue;
					}

					case 'S': // is replaced by the second as a decimal number [00,61]. 
						if(!WriteInt(pTM->tm_sec, 2, '0', cAlt == '#', pTimeString, capacity))
							return 0;
						continue;

					case 't': // is replaced by a tab character. 
						buffer[0] = '\t';
						buffer[1] = 0;
						if(!Append(buffer, pTimeString, capacity))
							return 0;
						continue;

					case 'T': // is replaced by the time (%H:%M:%S). 
					{
						const size_t len = Strftime(pTimeString, capacity, "%H:%M:%S", pTM);
						if(!len)
							return 0;
						pTimeString += len;
						capacity    -= len;
						continue;
					}

					case 'u': // is replaced by the weekday as a decimal number [1,7], with 1 representing Monday. 
						if(!WriteInt(pTM->tm_wday ? pTM->tm_wday : 7, 1, '0', cAlt == '#', pTimeString, capacity))
							return 0;
						continue;

					case 'U': // is replaced by the week number of the year (Sunday as the first day of the week) as a decimal number [00,53]. 
					{
						int wday = SUNDAY_BASED_WEEK_NUMBER(pTM);
						if(wday == 0 && cAlt == '#')
						{
							if(!WriteInt(wday, 1, '0', false, pTimeString, capacity))
								return 0;
						}
						else
						{
							if(!WriteInt(wday, 2, '0', cAlt == '#', pTimeString, capacity))
								return 0;
						}
						continue;
					}

					case 'V': // is replaced by the week number of the year (Monday as the first day of the week) as a decimal number [01,53]. If the week containing 1 January has four or more days in the new year, then it is considered week 1. Otherwise, it is the last week (53) of the previous year, and the next week is week 1. 
					{
						int week = MONDAY_BASED_WEEK_NUMBER(pTM);
						int days = ((pTM->tm_yday + 7 - (pTM->tm_wday ? pTM->tm_wday - 1 : 6)) % 7);

						if(days >= 4)
							week++;
						else if(week == 0)
							week = 53;

						if(!WriteInt(week, 2, '0', cAlt == '#', pTimeString, capacity))
							return 0;

						continue;
					}

					case 'w': // is replaced by the weekday as a decimal number [0,6], with 0 representing Sunday. 
						if(!WriteInt(pTM->tm_wday, 1, '0', false, pTimeString, capacity))
							return 0;
						continue;

					case 'W': // %W is replaced by the week number of the year (Monday as the first day of the week) as a decimal number [00,53]. All days in a new year preceding the first Monday are considered to be in week 0. 
					{
						int wday = MONDAY_BASED_WEEK_NUMBER(pTM);
						if(wday == 0 && cAlt == '#')
						{
							if(!WriteInt(wday, 1, '0', false, pTimeString, capacity))
								return 0;
						}
						else
						{
							if(!WriteInt(wday, 2, '0', cAlt == '#', pTimeString, capacity))
								return 0;
						}
						continue;
					}

					case 'x': // %x is replaced by the locale's appropriate date representation. 
					{
						#if EASTDC_WINDOWS_TIME_FUNCTIONS_AVAILABLE
							const SYSTEMTIME systemTime = { WORD(pTM->tm_year + 1900), WORD(pTM->tm_mon + 1), WORD(pTM->tm_wday), WORD(pTM->tm_mday), WORD(pTM->tm_hour), WORD(pTM->tm_min), WORD(pTM->tm_sec), 0 };
							const uint32_t   dateFormat = (cAlt == '#') ?  DATE_SHORTDATE : DATE_LONGDATE;

							if(!EAStdCGetDateFormat(dateFormat, &systemTime, buffer, sizeof buffer) || !Append(buffer, pTimeString, capacity))
								return 0;
						#else
							char formatBuffer[256]; formatBuffer[0] = 0;
							const char* pFormatTemp = pTimeLocale->mDateFormat;

							if (cAlt == '#')
								pFormatTemp = ConvertFormatSpecifiersToAlternates(formatBuffer, sizeof formatBuffer, pFormatTemp);

							const size_t len = Strftime(pTimeString, capacity, pFormatTemp, pTM);
							if(!len)
								return 0;
							pTimeString += len;
							capacity    -= len;
						#endif
						continue;
					}

					case 'X': // %X is replaced by the locale's appropriate time representation. 
					{
						#if EASTDC_WINDOWS_TIME_FUNCTIONS_AVAILABLE
							const SYSTEMTIME systemTime = { WORD(pTM->tm_year + 1900), WORD(pTM->tm_mon + 1), WORD(pTM->tm_wday), WORD(pTM->tm_mday), WORD(pTM->tm_hour), WORD(pTM->tm_min), WORD(pTM->tm_sec), 0 };

							if(!EAStdCGetTimeFormat(0, &systemTime, buffer, sizeof buffer) || !Append(buffer, pTimeString, capacity))
								return 0;
						#else
							char formatBuffer[256]; formatBuffer[0] = 0;
							const char* pFormatTemp = pTimeLocale->mTimeFormat;

							if (cAlt == '#')
								pFormatTemp = ConvertFormatSpecifiersToAlternates(formatBuffer, sizeof formatBuffer, pFormatTemp);

							const size_t len = Strftime(pTimeString, capacity, pFormatTemp, pTM);
							if(!len)
								return 0;
							pTimeString += len;
							capacity    -= len;
						#endif
						continue;
					}

					case 'y': // %y is replaced by the year without century as a decimal number [00,99]. 
					{
						int year = (pTM->tm_year + 1900) % 100;
						if(year == 0 && cAlt == '#')
						{
							if(!WriteInt(year, 1 /*strlen("99")*/, '0', false, pTimeString, capacity))
								return 0;
						}
						else
						{
							if(!WriteInt(year, 2 /*strlen("99")*/, '0', cAlt == '#', pTimeString, capacity))
								return 0;
						}
						continue;
					}
					case 'Y': // %Y is replaced by the year with century as a decimal number. 
						if(!WriteInt((pTM->tm_year + 1900), 4 /*strlen("9999")*/, '0', cAlt == '#', pTimeString, capacity))
							return 0;
						continue;

					case 'z': //Replaced by the offset from UTC in the ISO 8601:2000 standard format ( +hhmm or -hhmm ), or by no characters if no timezone is determinable. 
							  //For example, "-0430" means 4 hours 30 minutes behind UTC (west of Greenwich). [CX]   If tm_isdst is zero, the standard time offset is used. 
							  //If tm_isdst is greater than zero, the daylight savings time offset is used. If tm_isdst is negative, no characters are returned.  [ tm_isdst]
					{
						int tzBias = (int)GetTimeZoneBias();  // tzBias will be a negative number in the United States.
						int hour   = abs(tzBias / 3600);
						int min    = (abs(tzBias) - (hour * 3600)) / 60;
			 
						buffer[5] = '\0';
						buffer[4] = (char)((min % 10) + '0');
						min /= 10;
						buffer[3] = (char)((min % 10) + '0');
						buffer[2] = (char)((hour % 10) + '0');
						hour /= 10;
						buffer[1] = (char)((hour % 10) + '0');
						buffer[0] = (tzBias < 0) ? '-' : '+';

						if(!Append(buffer, pTimeString, capacity))
							return 0;
						continue;
					}

					case 'Z': // %Z is replaced by the timezone name or abbreviation, or by no bytes if no timezone information exists. 
					{
						const char* pTZName = bGMT ? "GMT" : buffer;

						if(!bGMT)
							GetTimeZoneName(buffer, pTM->tm_isdst != 0);

						if(pTZName && !Append(pTZName, pTimeString, capacity))
							return 0;
						continue;
					}

					default:
						EA_FAIL_M("EAStdC Strftime"); // Unsupported format specifier. Just print it as-is.
						break;
				}
			}

			if(!capacity--)
				return 0;

			*pTimeString++ = *pFormat;
		}

		*pTimeString = 0;

		return (timeStringCapacity - capacity);

	} // Strftime


	static bool ReadInt(const char*& pString, int& n, int nMin, int nMax)
	{
		int result = 0;
		int rMax   = nMax;
	 
		if((*pString >= '0') && (*pString <= '9'))
		{
			do {
				result *= 10;
				result += *pString++ - '0';
				rMax /= 10;
			} while(rMax && (*pString >= '0') && (*pString <= '9') && ((result * 10) <= nMax));
		 
			if((result >= nMin) && (result <= nMax))
			{
				n = result;
				return true;
			}
		}

		return false;
	}

	static bool ParseDate(bool bAlt, const char *&p, tm* EA_RESTRICT pTM, const TimeLocale *pTimeLocale)
	{
		#if EASTDC_WINDOWS_TIME_FUNCTIONS_AVAILABLE
			EA_UNUSED(pTimeLocale);
			const int formatSize = 256;
			char buffer[formatSize]; buffer[0] = 0;
			char* c;
			char* tokenizerContext = NULL;
			size_t index = 0;

			LCTYPE localeDateType = bAlt ? LOCALE_SSHORTDATE : LOCALE_SLONGDATE;

			size_t bufferSize = static_cast<size_t>(EAStdCGetLocaleInfo(localeDateType, NULL, 0));
			char* dateFormat = static_cast<char *>(EAAlloca(bufferSize + 1));
			char* tmp = static_cast<char *>(EAAlloca(bufferSize + 1));
			EA_ANALYSIS_ASSUME(dateFormat != NULL);
			EAStdCGetLocaleInfo(localeDateType, dateFormat, static_cast<int>(bufferSize));
			dateFormat[bufferSize] = 0;

			Strlcpy(tmp, dateFormat, bufferSize + 1);
			c = Strtok(tmp, "/, -", &tokenizerContext);
			while(c != 0)
			{
				if(Strncmp(c, "d", formatSize) == 0)
					Strlcat(buffer, "%#d", formatSize);
				else if(Strncmp(c, "dd", formatSize) == 0)
					Strlcat(buffer, "%d", formatSize);
				else if(Strncmp(c, "ddd", formatSize) == 0)
					Strlcat(buffer, "%#a", formatSize);
				else if(Strncmp(c, "dddd", formatSize) == 0)
					Strlcat(buffer, "%a", formatSize);
				else if(Strncmp(c, "M", formatSize) == 0)
					Strlcat(buffer, "%#m", formatSize);
				else if(Strncmp(c, "MM", formatSize) == 0)
					Strlcat(buffer, "%m", formatSize);
				else if(Strncmp(c, "MMM", formatSize) == 0)
					Strlcat(buffer, "%#b", formatSize);
				else if(Strncmp(c, "MMMM", formatSize) == 0)
					Strlcat(buffer, "%b", formatSize);
				else if(Strncmp(c, "y", formatSize) == 0)
					Strlcat(buffer, "%#y", formatSize);
				else if(Strncmp(c, "yy", formatSize) == 0)
					Strlcat(buffer, "%y", formatSize);
				else if(Strncmp(c, "yyyy", formatSize) == 0)
					Strlcat(buffer, "%Y", formatSize);
				else if(Strncmp(c, "yyyyy", formatSize) == 0)
					Strlcat(buffer, "%Y", formatSize);
				else
					return false;

				index += Strlen(c);
				size_t separators = Strcspn(&dateFormat[index], "dmyM");
				char copiedChar = dateFormat[index + separators];
				dateFormat[index + separators] = 0;
				Strlcat(buffer, &dateFormat[index], formatSize);
				dateFormat[index + separators] = copiedChar;
				index += separators;

				c = Strtok(NULL, "/, -", &tokenizerContext);
			}

			if((p = Strptime(p, buffer, pTM)) == NULL)
				return false;
		#else
			char formatBuffer[256]; formatBuffer[0] = 0;
			const char* pFormatTemp = pTimeLocale->mDateFormat;

			if(bAlt)
				pFormatTemp = Internal::ConvertFormatSpecifiersToAlternates(formatBuffer, sizeof formatBuffer, pFormatTemp);

			if((p = Strptime(p, pFormatTemp, pTM)) == NULL)
				return false;
		#endif

		return true;
	}

	static bool ParseTime(bool bAlt, const char *&p, tm* EA_RESTRICT pTM, const TimeLocale *pTimeLocale)
	{
		#if EASTDC_WINDOWS_TIME_FUNCTIONS_AVAILABLE
			EA_UNUSED(pTimeLocale);
			EA_UNUSED(bAlt);

			const int formatSize = 256;
			char buffer[formatSize]; buffer[0] = 0;

			size_t bufferSize = static_cast<size_t>(EAStdCGetLocaleInfo(LOCALE_STIMEFORMAT, NULL, 0));
			char* timeFormat = static_cast<char *>(EAAlloca(bufferSize + 1));
			char* tmp = static_cast<char *>(EAAlloca(bufferSize + 1));
			EA_ANALYSIS_ASSUME(timeFormat != NULL);

			EAStdCGetLocaleInfo(LOCALE_STIMEFORMAT, timeFormat, static_cast<int>(bufferSize));
			Strlcpy(tmp, timeFormat, bufferSize);

			char* tokenizerContext = NULL;
			size_t index = 0;
			char*  c = Strtok(tmp, ": ", &tokenizerContext);

			while(c != 0)
			{
				if(Strncmp(c, "h", formatSize) == 0)
					Strlcat(buffer, "%#I", formatSize);
				else if(Strncmp(c, "hh", formatSize) == 0)
					Strlcat(buffer, "%I", formatSize);
				else if(Strncmp(c, "H", formatSize) == 0)
					Strlcat(buffer, "%#H", formatSize);
				else if(Strncmp(c, "HH", formatSize) == 0)
					Strlcat(buffer, "%H", formatSize);
				else if(Strncmp(c, "m", formatSize) == 0)
					Strlcat(buffer, "%#M", formatSize);
				else if(Strncmp(c, "mm", formatSize) == 0)
					Strlcat(buffer, "%M", formatSize);
				else if(Strncmp(c, "s", formatSize) == 0)
					Strlcat(buffer, "%#S", formatSize);
				else if(Strncmp(c, "ss", formatSize) == 0)
					Strlcat(buffer, "%S", formatSize);
				else if(Strncmp(c, "t", formatSize) == 0)
					Strlcat(buffer, "%#p", formatSize);
				else if(Strncmp(c, "tt", formatSize) == 0)
					Strlcat(buffer, "%p", formatSize);
				else
					return false;

				index += Strlen(c);
				size_t separators = Strcspn(&timeFormat[index], "hHmst");
				char copiedChar = timeFormat[index + separators];
				timeFormat[index + separators] = 0;
				Strlcat(buffer, &timeFormat[index], formatSize);
				timeFormat[index + separators] = copiedChar;

				index += separators;
				c = Strtok(NULL, ": ", &tokenizerContext);
			}

			if((p = Strptime(p, buffer, pTM)) == NULL)
				return false;
		#else
			char formatBuffer[256]; formatBuffer[0] = 0;
			const char* pFormatTemp = pTimeLocale->mTimeFormat;

			if(bAlt)
				pFormatTemp = Internal::ConvertFormatSpecifiersToAlternates(formatBuffer, sizeof formatBuffer, pFormatTemp);

			if((p = Strptime(p, pFormatTemp, pTM)) == NULL)
				return false;

		#endif

		return true;
	}


	EASTDC_API char* Strptime(const char* EA_RESTRICT pTimeString, const char* EA_RESTRICT pFormat, tm* EA_RESTRICT pTM, const TimeLocale* EA_RESTRICT pTimeLocale)
	{
		using namespace Internal;

		const char* p = pTimeString;
		size_t      len = 0;
		int         i = 0;
		bool        bSplitYear = false;
		bool        bAlt = false;
		char        c;

		if(!pTimeLocale)
			pTimeLocale = &gDefaultTimeLocale;

		while((c = *pFormat) != 0)
		{
			if(Isspace(c))          // If the current format char is whitespace...
			{                       // 
				while(Isspace(*p))  // then eat any time string whitespace.
					p++;
	 
				pFormat++;
				continue;
			}

			c = *pFormat++;         // c will be non-whitespace.

			if(c != '%')            // If we have a literal char that is outside of a % format sequence...
			{
				if(c != *p++)
					return NULL;
				continue;
			}

			bAlt = false;
			FormatBegin:
			c = *pFormat++;

			switch (c)
			{
				case '%':   // Replaced by %.
					if(c != *p++)
						return NULL;
					break;

				case 'E':   // E alternate representation. 
				case 'O':   // O alternate representation.
				case '#':   // # alternate representation.
					bAlt = true;
					goto FormatBegin;

				case 'a':   // The day of the week, using the locale's weekday names; either the abbreviated or full name may be specified.
				case 'A':   // Equivalent to %a.
				{
					for(i = 0; i < 7; i++)
					{
						len = Strlen(pTimeLocale->mDay[i]);

						if(Strnicmp(pTimeLocale->mDay[i], p, len) == 0)
							break;
			 
						len = Strlen(pTimeLocale->mAbbrevDay[i]);

						if(Strnicmp(pTimeLocale->mAbbrevDay[i], p, len) == 0)
							break;
					}

					// Nothing matched.
					if(i == 7)
						return NULL;

					pTM->tm_wday = i;
					p += len;
					break;
				}

				case 'b':   // The month, using the locale's month names; either the abbreviated or full name may be specified.
				case 'B':   // Equivalent to %b.
				case 'h':   // Equivalent to %b.
				{
					for(i = 0; i < 12; i++)
					{
						len = Strlen(pTimeLocale->mMonth[i]);

						if(Strnicmp(pTimeLocale->mMonth[i], p, len) == 0)
							break;

						len = Strlen(pTimeLocale->mAbbrevMonth[i]);

						if(Strnicmp(pTimeLocale->mAbbrevMonth[i], p, len) == 0)
							break;
					}

					if(i == 12)
						return NULL;

					pTM->tm_mon = i;
					p += len;
					break;
				}

				case 'c':   // Replaced by the locale's appropriate date and time representation.
				{
					#if EASTDC_WINDOWS_TIME_FUNCTIONS_AVAILABLE
						if(!ParseDate(bAlt, p, pTM, pTimeLocale))
							return NULL;

						while(Isspace(*p))
							++p;

						if(!ParseTime(bAlt, p, pTM, pTimeLocale))
							return NULL;
					#else
						char buffer[256]; buffer[0] = 0;
						const char* pFormatTemp = pTimeLocale->mDateTimeFormat;

						if(bAlt)
							pFormatTemp = ConvertFormatSpecifiersToAlternates(buffer, sizeof buffer, pFormatTemp);
						
						if((p = Strptime(p, pFormatTemp, pTM)) == NULL)
							return NULL;                  
					#endif

					break;
				}

				case 'C':   // The century number [00,99]; leading zeros are permitted but not required.
					if(!ReadInt(p, i, 0, 99))
						return NULL;

					if(bSplitYear)
						pTM->tm_year = (pTM->tm_year % 100) + (i * 100);
					else
					{
						pTM->tm_year = i * 100;
						bSplitYear   = true;
					}
					break;

				case 'd':   // The day of the month [01,31]; leading zeros are permitted but not required.
				case 'e':   // Equivalent to %d. 
					if(!ReadInt(p, pTM->tm_mday, 1, 31))
						return NULL;
					break;

				case 'D':   // The date as %m / %d / %y.
					if((p = Strptime(p, "%m/%d/%y", pTM)) == NULL)
						return NULL;
					break;

				case 'H':   // The hour (24-hour clock) [00,23]; leading zeros are permitted but not required.
					if(!ReadInt(p, pTM->tm_hour, 0, 23))
						return NULL;
					break;

				case 'I':   // The hour (12-hour clock) [01,12]; leading zeros are permitted but not required.
					if(!ReadInt(p, pTM->tm_hour, 1, 12))
						return NULL;
					break;

				case 'j':   // The day number of the year [001,366]; leading zeros are permitted but not required.
					if(!ReadInt(p, i, 1, 366))
						return NULL;
					pTM->tm_yday = i - 1;
					break;

				case 'M':   // The minute [00,59]; leading zeros are permitted but not required.
					if(!ReadInt(p, pTM->tm_min, 0, 59))
						return NULL;
					break;

				case 'm':   // The month number [01,12]; leading zeros are permitted but not required.
					if(!ReadInt(p, i, 1, 12))
						return NULL;
					pTM->tm_mon = i - 1;
					break;

				case 'n':   // Any white space.
				case 't':   // Any white space.
					while(Isspace(*p))
						p++;
					break;

				case 'p':   // The locale's equivalent of a.m or p.m.
				{
					size_t lenAM = Strlen(pTimeLocale->mAmPm[0]);
					size_t lenPM = Strlen(pTimeLocale->mAmPm[1]);
					if(Strnicmp(pTimeLocale->mAmPm[0], p, lenAM) == 0) // Test AM
					{
						if (pTM->tm_hour == 12)
							pTM->tm_hour = 0;

						p += lenAM;
						break;
					}
					else if(Strnicmp(pTimeLocale->mAmPm[1], p, lenPM) == 0) // Test PM
					{
						if(pTM->tm_hour <= 11)
							pTM->tm_hour += 12;

						if (pTM->tm_hour > 23)
							return NULL;

						p += lenPM;
						break;
					}

					return NULL;
				}

				case 'r':   // 12-hour clock time using the AM/PM notation if t_fmt_ampm is not an empty string in the LC_TIME portion of the current locale; in the POSIX locale, this shall be equivalent to %I : %M : %S %p.
					if((p = Strptime(p, "%I:%M:%S %p", pTM)) == NULL)
						return NULL;
					break;

				case 'R':   // The time as %H : %M.
					if((p = Strptime(p, "%H:%M", pTM)) == NULL)
						return NULL;
					break;

				case 'S':   // The seconds [00,60]; leading zeros are permitted but not required.
					if(!ReadInt(p, pTM->tm_sec, 0, 61))
						return NULL;
					break;

				case 'T':   // The time as %H : %M : %S.
					if((p = Strptime(p, "%H:%M:%S", pTM)) == NULL)
						return NULL;
					break;

				case 'U':   // The week number of the year (Sunday as the first day of the week) as a decimal number [00,53]; leading zeros are permitted but not required.
				case 'W':   // The week number of the year (Monday as the first day of the week) as a decimal number [00,53]; leading zeros are permitted but not required.
					// It's hard to calculate this, as we need the rest of the information 
					// to implement it right. Or we could possibly delay calculation of it.
				{
					//Unsupported as of yet
					break;
				}

				case 'w':   // The weekday as a decimal number [0,6], with 0 representing Sunday; leading zeros are permitted but not required.
					if(!ReadInt(p, pTM->tm_wday, 0, 6))
						return NULL;
					break;

				case 'x':   // The date, using the locale's date format.
				{
					if (!ParseDate(bAlt, p, pTM, pTimeLocale))
						return NULL;
					break;
				}

				case 'X':   // The time, using the locale's time format.
				{
					if (!ParseTime(bAlt, p, pTM, pTimeLocale))
						return NULL;
					break;
				}
				case 'Y':   // The year, including the century (for example, 1988).
					if(!ReadInt(p, pTM->tm_year, 0, 9999))
						return NULL;
					//Set the year according to the 1900epoch
					pTM->tm_year -= 1900;
					break;

				case 'y':   // The year within century. When a century is not otherwise specified, values in the range [69,99] shall refer to years 1969 to 1999 inclusive, and values in the range [00,68] shall refer to years 2000 to 2068 inclusive; leading zeros shall be permitted but shall not be required.
					if(!ReadInt(p, i, 0, 99))
						return NULL;

					if(bSplitYear)
					{
						pTM->tm_year = ((pTM->tm_year / 100) * 100) + i;
						break;
					}

					bSplitYear = true;

					if(i <= 68)
						pTM->tm_year = (2000 - 1900) + i;
					else
						pTM->tm_year = i;
					break;

				default:
					EA_FAIL_M("EAStdC Strptime"); // Unsupported format specifier. Just print it as-is.
					return NULL;
			}
		}

		return (char*)p;

	} // Strptime


	#undef EADT_COUNT_LEAP_YEARS
	#undef SUNDAY_BASED_WEEK_NUMBER
	#undef MONDAY_BASED_WEEK_NUMBER

} // namespace StdC
} // namespace EA


#ifdef _MSC_VER
	#pragma warning(pop)
#endif





