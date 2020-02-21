///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Implements a suite of functionality to manage dates, times, and calendars.
///////////////////////////////////////////////////////////////////////////////


#ifndef EASTDC_EADATETIME_H
#define EASTDC_EADATETIME_H


#include <EAStdC/internal/Config.h>
#include <EAStdC/Int128_t.h>

#if EASTDC_TIME_H_AVAILABLE
	#include <time.h>
#else
	typedef int64_t time_t;
	typedef int32_t suseconds_t;
	typedef int64_t clock_t;

	struct tm
	{
		int tm_sec;
		int tm_min;
		int tm_hour;
		int tm_mday;
		int tm_mon;
		int tm_year;
		int tm_wday;
		int tm_yday;
		int tm_isdst;
	};

	time_t time(time_t*);
	tm*    gmtime(const time_t*);
	tm*    localtime(const time_t*);
#endif

#if   defined(EA_PLATFORM_MICROSOFT)
	#if (defined(EA_PLATFORM_WINDOWS) && !EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP))
		typedef int32_t suseconds_t;
		#define EASTDC_TIMEVAL_NEEDED
		#define EASTDC_TIMEZONE_NEEDED
	#else
		struct timeval;                 // We have a problem: Microsoft defines timeval, but only within the Windows header system.
		typedef time_t suseconds_t;     // And we have a rule against #including windows headers within header files. So we forward
		#define EASTDC_TIMEZONE_NEEDED  // declare timeval and expect the user to #include <WinSock2.h> in order to use it.
	#endif

#elif EASTDC_SYS_TIME_H_AVAILABLE
	#include <sys/time.h>           // Defines timeval and timezone
	struct timezone_ : public timezone{};

#elif EASTDC_SYS__TIMEVAL_H_AVAILABLE
	#include <sys/_timeval.h>       // Defines timeval
	#define EASTDC_TIMEZONE_NEEDED

#else
	#define EASTDC_TIMEVAL_NEEDED
	#define EASTDC_TIMEZONE_NEEDED
#endif


#ifdef EASTDC_TIMEVAL_NEEDED
	struct timeval {
		time_t      tv_sec;     // seconds
		suseconds_t tv_usec;    // microseconds
	};
#endif

#ifdef EASTDC_TIMEZONE_NEEDED
	struct timezone_ {          // Defined as timezone_ instead of timezone because otherwise it conflicts with some platforms' existing identifiers.
		int tz_minuteswest;     // This is the number of minutes west of GMT.
		int tz_dsttime;         // If nonzero, daylight savings time applies during some part of the year. 
	};
#endif



// Forward declare the Microsoft time structs. We do this for all platforms, 
// including non-Microsoft platforms in order to simply the code. This won't
// result in excess code as these functions go away when unused.
struct _FILETIME;
struct _SYSTEMTIME;



///////////////////////////////////////////////////////////////////////////////
// EASTDC_UTC_TIME_AVAILABLE
//
// Defined as 0 or 1. It's value is fixed based on the platform.
// This is a read-only value which is set by this module.
// If you become aware of a way to get UTC time for a platform which 
// currently is flagged as unavailable here, notify the package owners.
// For platforms where UTC time is not available, UTC time is reported to 
// be the same as local time. If UTC time is not available, then time 
// zone information is by definition not available either.
//
#if !defined(EASTDC_UTC_TIME_AVAILABLE)
		#define EASTDC_UTC_TIME_AVAILABLE 1
#endif



namespace EA
{
namespace StdC
{
	// Used for Parameter values (see below)
	static const uint32_t kDateTimeDefault = 0xffffffff;
	static const uint32_t kDateTimeIgnored = 0xffffffff;

	/// Month
	enum Month
	{
		kMonthUnknown        =  0,
		kMonthJanuary        =  1,
		kMonthFebruary       =  2,
		kMonthMarch          =  3,
		kMonthApril          =  4,
		kMonthMay            =  5,
		kMonthJune           =  6,
		kMonthJuly           =  7,
		kMonthAugust         =  8,
		kMonthSeptember      =  9,
		kMonthOctober        = 10,
		kMonthNovember       = 11,
		kMonthDecember       = 12
	};

	enum DayOfMonth
	{
		kDayOfMonthUnknown   =  0,
		kDayOfMonthMin       =  1,
		kDayOfMonthMax       = 31   /// The actual max month is dependent on which month is being referred to.
	};

	enum DayOfWeek
	{
		kDayOfWeekUnknown    =  0,
		kDayOfWeekSunday     =  1,
		kDayOfWeekMonday     =  2,
		kDayOfWeekTuesday    =  3,
		kDayOfWeekWednesday  =  4,
		kDayOfWeekThursday   =  5,
		kDayOfWeekFriday     =  6,
		kDayOfWeekSaturday   =  7
	};

	/// TimeFrame
	enum TimeFrame
	{
		kTimeFrameUnknown = 0,  /// Unspecified time frame.
		kTimeFrameUTC     = 1,  /// Universal Coordinated Time. This is the time based on the time zone at Greenwich, England.
		kTimeFrameLocal   = 2   /// Same time as current machine.
	};

	/// TimeZone
	/// Standard time zone definitions, with their values corresponding to the nmuber of hours they are
	/// off relative to UTC (Universal Coordinated Time, which is the time at Greenwich England). 
	/// Note, for example, that kTimeZonePacific is -8 hours relative to Greenwich, England.
	/// It should be noted that time zone biases (e.g. central australia) are properly represented in 
	/// seconds and not hours, as some time zone biases relative to UTC are on half-hour increments. 
	/// Such time zones are not listed below, as the list below is merely the UTC standard 24 hour list.
	enum TimeZone
	{
		kTimeZoneEniwetok    = -12,
		kTimeZoneSamoa       = -11,
		kTimeZoneHawaii      = -10,
		kTimeZoneAlaska      =  -9,
		kTimeZonePacific     =  -8,
		kTimeZoneMountain    =  -7,
		kTimeZoneCentral     =  -6,
		kTimeZoneEastern     =  -5,
		kTimeZoneAltantic    =  -4,
		kTimeZoneBrazilia    =  -3,
		kTimeZoneMidAtlantic =  -2,
		kTimeZoneAzores      =  -1,
		kTimeZoneGreenwich   =   0,
		kTimeZoneRome        =  +1,
		kTimeZoneIsrael      =  +2,
		kTimeZoneMoscow      =  +3,
		kTimeZoneBaku        =  +4,
		kTimeZoneNewDelhi    =  +5,
		kTimeZoneDhakar      =  +6,
		kTimeZoneBangkok     =  +7,
		kTimeZoneHongKong    =  +8,
		kTimeZoneTokyo       =  +9,
		kTimeZoneSydney      = +10,
		kTimeZoneMagadan     = +11,
		kTimeZoneWellington  = +12
	};

	/// Epoch
	/// The use of an epoch is to provide a timeframe with which to work.
	/// Most of the time you don't need to know about this.
	/// https://en.wikipedia.org/wiki/Epoch_%28reference_date%29
	enum Epoch
	{
		kEpochUnknown         =  0,
		kEpochJulian          =  1,      /// Began at -4712/01/01/12:00:00 (Year 1858, January 1, noon).
		kEpochGregorian       =  2,      /// Began at  1752/09/14/00:00:00 (Year 1752, January 1, midnight). Beginning of the Gregorian calendar.
		kEpochModifiedJulian  =  3,      /// Began at  1858/11/17/00:00:00 (Year 1858, January 1, midnight). 2,400,000.5 days after Julian epoch began.
		kEpoch1900            =  4,      /// Began at  1900/01/01/00:00:00 (Year 1900, January 1, midnight). Same epoch used by the Network Time Protocol.
		kEpoch1950            =  5,      /// Began at  1950/01/01/00:00:00 (Year 1950, January 1, midnight). Used by some gaming systems.
		kEpoch1970            =  6,      /// Began at  1970/01/01/00:00:00 (Year 1970, January 1, midnight). Same epoch used by C runtime library and Unix OS.
		kEpoch2000            =  7,      /// Began at  2000/01/01/00:00:00 (Year 2000, January 1, midnight). Same epoch used by Apple file systems.
		kEpochJ2000           =  8,      /// Began at  2000/01/01/11:58:55 (Year 2000, January 1, ~noon).    Coordinated Universal Time, also includes 816 milliseconds.
		kEpochDateTime        =  9,      /// Began at  0000/01/01/00:00:00 (Year 0000, January 1, midnight).
		kEpochCount
	};

	/// Era
	enum Era
	{
		kEraUnknown = 0,
		kEraBC      = 1,
		kEraAD      = 2
	};

	/// Parameter
	/// Defines a date or time parameter.
	enum Parameter
	{
		kParameterUnknown     =  0,
		kParameterYear        =  1,     /// Refers to full year value, such as 1994, 2006, etc. but not a two digit value such as 94 or 04. The valid range is 0-INT_MAX.
		kParameterMonth       =  2,     /// Refers to month of year, starting with 1 for January. The valid range is 1-12.
		kParameterWeekOfYear  =  3,     /// Refers to the week of the year, starting with 1 for the week of January 1. The valid range is 1-52.
		kParameterWeekOfMonth =  4,     /// Refers to the week of the month, starting with 1 for the first week. The valid range is 1-5.
		kParameterDayOfYear   =  5,     /// Refers to a day of the year, starting with 1 for January 1st. The valid range is 1-366.
		kParameterDayOfMonth  =  6,     /// Refers to the day of the month, starting with 1 for the first of the month. The valid range is 1-31.
		kParameterDayOfWeek   =  7,     /// Refers to the day of the week, starting with 1 for Sunday. The valid range is 1-7.
		kParameterHour        =  8,     /// Refers to the hour of a day in 24 hour format, starting with 0 for midnight. The valid range is 0-23.
		kParameterMinute      =  9,     /// Refers to the minute of the hour, starting with 0 for the first minute. The valid range is 0-59.
		kParameterSecond      = 10,     /// Refers to the second of the minute, starting with 0 for the first second. The valid range is 0-60, with the range usually being 0-59, but the occasional leap second could cause it to be 60.
		kParameterNanosecond  = 11      /// Refers to the nanosecond of the second. The valid range is 0-999999999. 
	};

	/// Conversions
	/// Defines useful conversion multipliers between seconds, minutes, hours, and days.
	/// Conversions are not defined for some entities (e.g. days per year) because the 
	/// value changes based on the particular year.
	enum Conversions
	{
		kSecondsPerMinute =    60,  // We ignore than on rare occasions in real time there are 61 seconds in a minute when there is a leap second.
		kSecondsPerHour   =  3600,
		kSecondsPerDay    = 86400,
		kMinutesPerHour   =    60,
		kMinutesPerDay    =  1440,
		kHoursPerDay      =    24,
		kDaysPerWeek      =     7,
		kWeeksPerYear     =    52,
		kMonthsPerYear    =    12
	};


	/// DateTimeParameters
	/// Specifies a struct that holds one of each of the date/time Parameter type.
	/// This struct is a bit like the C tm struct, though with more flexibility and precision.
	/// Any value can be kDateTimeIgnored to indicate that it isn't used. 
	/// Some parameters are potentially mutually exclusive with others (e.g. mMonth and mWeekOfYear).
	/// The user is expected to avoid such conflicts, or else the code specifies how it handles
	/// such conflicts.
	/// All values default to kDateTimeIgnored.
	/// Values can be negative in order to identify date/time deltas as opposed to absolute date/time values.
	/// Values can be outside their normal range in order to indicate date/time deltas greater 
	/// than normal. For example, mMonth can be -16 to indicate subtraction of 16 months. Negative or 
	/// out-of-range values may have no meaning when DateTimeParameters is used to indicate absolute date/time.
	struct DateTimeParameters
	{
		DateTimeParameters();

		int32_t mYear;
		int32_t mMonth;
		int32_t mWeekOfYear;
		int32_t mWeekOfMonth;
		int32_t mDayOfYear;
		int32_t mDayOfMonth;
		int32_t mDayOfWeek;
		int32_t mHour;
		int32_t mMinute;
		int32_t mSecond;
		int32_t mNanosecond;
	};


	/// DateTime
	/// Represents date and time in a single class. Unlike other date/time systems, 
	/// this class doesn't have a strictly limited resolution, due to its use of
	/// 64 bits (instead of 32 bits) of storage for the date/time value.
	/// 
	/// DateTime is internally represented as nanoseconds since 0000/01/01/00:00:00 
	/// (Year 0000, January 1, midnight). Thus the beginning of time is considered
	/// to be this date. This is differnet from the old Unix concept of time starting
	/// at 1970 and is so because it provides more flexibility and portably allows 
	/// for date representation prior to 1970. You can convert between time_t and DateTime 
	/// seconds with the DateTimeSecondsToTimeTSeconds and TimeTSecondsSecondsToDateTime functions.
	///
	/// This class does not support the formatting of date and time strings for 
	/// display to a users. Such functionality is outside the scope of this class
	/// primarily due to the issue of multiple string encodings (e.g. ASCII and Unicode)
	/// and due to the issue of multiple localization (e.g. English and French).
	/// As separate module exists which implements string formatting.
	///
	/// For best practices regarding usage of DateTime, see the following discussion
	///     http://stackoverflow.com/questions/2532729/daylight-saving-time-and-timezone-best-practices
	/// 
	class EASTDC_API DateTime
	{
	public:
		static const uint32_t kValueDefault = kDateTimeDefault;
		static const uint32_t kValueIgnored = kDateTimeIgnored;

	public:
		DateTime(TimeFrame timeFrame = kTimeFrameLocal)
		  : mnSeconds(0), mnNanosecond(0){ Set(timeFrame); }

		/// nSeconds/nNanosecond refers to elapsed time since 0000/01/01/00:00:00 
		/// (Year 0000, January 1, midnight). This function is thus different from
		/// the old Unix concept of time starting at 1970 and is so because it provides
		/// more flexibility and portably allows for date representation prior to 1970.
		/// You can convert between time_t and DateTime seconds with the DateTimeSecondsToTimeTSeconds
		/// and TimeTSecondsSecondsToDateTime functions.
		/// Nanosecond refers to a fraction of seconds.
		DateTime(int64_t nSeconds, uint32_t nNanosecond = 0)
		  : mnSeconds(nSeconds), mnNanosecond(nNanosecond) { }

		// See the SetNanoseconds function for the specification of nanoseconds.
		DateTime(const int128_t& nanoseconds)
		  : mnSeconds(0), mnNanosecond(0) { SetNanoseconds(nanoseconds); }

		DateTime(const DateTime& dateTime) 
		  : mnSeconds(dateTime.mnSeconds), mnNanosecond(dateTime.mnNanosecond) { }

		/// DateTime
		/// Constructs a DateTime class object from some standard parameters.
		/// To construct a DateTime class with a different set of parameter types,
		/// you'll need to use the Set function or in odd cases do manual calculations. 
		DateTime(uint32_t nYear, uint32_t nMonth, uint32_t nDayOfMonth, uint32_t nHour = 0, 
				 uint32_t nMinute = 0, uint32_t nSecond = 0, uint32_t nNanosecond = 0)
		   : mnSeconds(0), mnNanosecond(0)
			{ Set(nYear, nMonth, nDayOfMonth, nHour, nMinute, nSecond, nNanosecond); }

		DateTime& operator=(const DateTime& dateTime)
			{ mnSeconds = dateTime.mnSeconds; mnNanosecond= dateTime.mnNanosecond; return *this; }

		/// Compare
		/// This function compares this object with another DateTime object and returns an integer result.
		/// The return value is the same as with the strcmp string comparison function. If this object is
		/// less than the argument dateTime, the return value is < 0.
		/// Both involved DateTime objects are assumed to be in the same time zone, and if you want to compare times
		/// in different time zones then you will have to translate one of them to the other's time zone, at least 
		/// until some day that explicit support for that is added to this package.
		///
		/// If bCompareDate is true  and bCompareTime is false, we compare the absolute day and nothing more precise.
		/// If bCompareDate is false and bCompareTime is true,  we compare the time (seconds+nanoseconds) within a day.
		/// If bCompareDate is true  and bCompareTime is true,  we compare absolute time (seconds+nanoseconds).
		/// If bCompareDate is false and bComareTime  is false, we compare absolute time (seconds+nanoseconds).
		/// Comparison operators are defined outside this class, though they use the Compare function to do their work.
		int Compare(const DateTime& dateTime, bool bCompareDate = true, bool bCompareTime = true) const;

		/// GetParameter
		/// Gets the given parameter. If you want to get the year, you would call Get(kParameterYear).
		uint32_t GetParameter(Parameter parameter) const;

		/// SetParameter
		/// Sets the given parameter. If you want to set the year to 1999, you would call Set(kParameterYear, 1999).
		/// You have to be careful with the following parameters, becuase they adjust time in a relative way as opposed
		/// to an absolute way, and thus the current date/time value may affect the resulting absolute date/time.
		/// As a result the effects of these parameters when called with other paramters can be order-dependent. 
		/// For example, setting kParameterDayOfWeek before kParameterDayOfMonth could result in a different month
		/// than if you called them in reverse order.
		///     kParameterDayOfYear, kParameterDayOfWeek, kParameterWeekOfYear, kParameterWeekOfMonth.
		void SetParameter(Parameter parameter, uint32_t nValue);

		/// Set
		/// Sets the time based on the current time. If the timeFrame is kTimeFrameUTC, the time is set
		/// to what the current UTC time is, which will be a fixed number of hours off of what the current
		/// local time is.
		void Set(TimeFrame timeFrame = kTimeFrameLocal, bool bSetNanoseconds = true);

		/// Set
		/// Sets the time based on various inputs. If any input is kValueIgnored (uint32_t)-1, then the 
		/// input is ignored and the current value is used. If any of the cyclic inputs is beyond its 
		/// valid range, the modulo of the value is used and the division of the value is added to the 
		/// next higher bracket. For example, if the input nMinute is 65, then the minute used is 5 and 
		/// 1 is added to the current hour value.
		void Set(uint32_t nYear, uint32_t nMonth, uint32_t nDayOfMonth, uint32_t nHour, 
				 uint32_t nMinute, uint32_t nSecond, uint32_t nNanosecond = kValueIgnored);

		/// AddTime
		/// Allows you to increment (or decrement) the given parameter by the given nValue amount.
		/// The amount can be less than zero and can be more than the conventional type limit. 
		/// For example, if parameter = kParameterMinute, nValue can be any int64_t value and not
		/// just in the range of 0 - 60.
		void AddTime(Parameter parameter, int64_t nValue);

		/// GetSeconds
		/// Returns the DateTime internal representation. The internal representation is such that 
		/// a value of zero refers to 0000/01/01/00:00:00 (Year 0000, January 1, midnight).
		/// This function thus gets the number of seconds "since the beginning of time".
		int64_t GetSeconds() const;

		/// SetSeconds
		/// Sets the DateTime internal representation. 
		/// This is not the same as SetParameter(kParameterSeconds), which sets the current day's 
		/// seconds and not the absolute seconds since the beginning of time like this function does.
		/// This function thus sets the number of seconds "since the beginning of time".
		void SetSeconds(int64_t nSeconds);

		/// GetMilliseconds
		/// Returns the DateTime internal representation. The internal representation is such that 
		/// a value of zero refers to 0000/01/01/00:00:00 (Year 0000, January 1, midnight).
		/// This function thus gets the number of milliseconds "since the beginning of time".
		uint64_t GetMilliseconds() const;

		/// SetMilliseconds
		/// Sets the DateTime internal representation. 
		/// This is not the same as SetParameter, which sets the current day's 
		/// seconds and not the absolute seconds since the beginning of time like this function does.
		/// This function thus sets the number of milliseconds "since the beginning of time".
		void SetMilliseconds(uint64_t milliseconds);

		/// GetNanoseconds
		/// Note that this is not the same thing as GetParameter(kParamaterNanosecond), as GetParameter retrieves
		/// the fraction of the current second in nanoseconds.
		EA::StdC::int128_t GetNanoseconds() const;

		/// SetNanoseconds
		/// Sets the time based in absolute nanoseconds since 0000/01/01/00:00:00 (Year 0000, January 1, midnight).
		/// Note that this is not the same thing as SetParameter(kParameterNanosecond), as SetParameter set the 
		/// number of nanoseconds that have elapsed since the current second, whereas this function sets the 
		/// number of nanoseconds that have elapsed since Jan 1, year 0000).
		void SetNanoseconds(const EA::StdC::int128_t& nanoseconds);

	protected:
		int64_t  mnSeconds;     /// Time as seconds. The internal representation is such that a value of zero refers to 0000/01/01/00:00:00 (Year 0000, January 1, midnight). This function thus gets the number of seconds "since the beginning of time".
		uint32_t mnNanosecond;  /// The nanosecond value within the current second. In the range of [0, 999999999]. This variable is not an analogue of the DateTime mnSeconds member variable, as this is a fraction whereas that is absolute.

	}; // class DateTime




	// Utility functions
	// The following utilities refer to the Gregorian calendar, which is the current 
	// standard calendar. These form the basis for the implementation of a basic calendar.

	/// GetTime
	/// Returns the number of nanoseconds elapsed since January 1, 1970 UTC.
	/// This is like the C time() function, except it returns nanoseconds.
	/// However, there is a key difference between this function and the C time
	/// function: this function returns actual real time progression in nanoseconds
	/// and will always return a value that's the actual time progressed since 
	/// it was first called. Thus time will never appear to go backwards, slow down,
	/// or speed up. Thus this function cannot be used for accurate calendaring
	/// like an desktop productivity app might want. For that you should use the 
	/// GetTimeOfDay function instead. This function's time consistency is what
	/// Posix refers to as CLOCK_MONOTONIC as opposed to CLOCK_REALTIME.
	/// This function is not guaranteed to give results that are precisely aligned
	/// with the results of GetTimeOfDay, as this function has different behaviour
	/// guarantees.
	/// Not all platforms have nanosecond-level precision, and in that case
	/// a nanosecond value will be returned but it may be with precision 
	/// that is lower.
	/// You can use the TimeTSecondsSecondsToDateTime function to help convert 
	/// GetTime values to seconds/nanoseconds used by the DateTime class.
	EASTDC_API uint64_t GetTime();

	/// GetTimeMilliseconds
	/// Returns the number of milliseconds elapsed since January 1, 1970 UTC.
	/// This is simply a convenient wrapper for GetTime() / 1000000 (ie.
	/// converting the nanosecond resolution of GetTime() to milliseconds).
	EASTDC_API uint64_t GetTimeMilliseconds();

	/// GetTimePrecision
	/// Returns the precision, in nanoseconds, of GetTime().
	/// A return value of 1 means GetTime indeed returns nanosecond values with nanosecond precision.
	/// A value of 1000 means that GetTime returns nanoseconds values with microsecond precision.
	EASTDC_API uint64_t GetTimePrecision();

	/// IsLeapYear
	/// Returns true if the given year is a leap year.
	EASTDC_API bool IsLeapYear(uint32_t nYear);

	/// GetDaysInYear
	/// Returns the number of days in the given year. The value will vary based on whether 
	/// the year is a leap year or not.
	EASTDC_API uint32_t GetDaysInYear(uint32_t nYear);

	/// GetDaysInMonth
	/// Returns the number of days in the given month. The value will vary based on the 
	/// month and based on whether the year is a leap year. The input nMonth takes one
	/// of enum Month or an integer equivalent.
	EASTDC_API uint32_t GetDaysInMonth(uint32_t nMonth, uint32_t nYear);

	/// GetDayOfYear
	/// The input nMonth takes one of enum Month or an integer equivalent.
	/// The input nDayOfMonth takes an integer consistent with enum DayOfMonth.
	EASTDC_API uint32_t GetDayOfYear(uint32_t nMonth, uint32_t nDayOfMonth, uint32_t nYear);

	/// Convert4DigitTo2DigitYear
	/// Note that two-digit years bring a number of problems; they are best used for text
	/// display only and not for any internal processing.
	inline int Convert4DigitTo2DigitYear(int nYear4)
		{ return (nYear4 % 100); } 

	/// Convert2DigitTo4DigitYear
	/// This code returns a year in the 1900s if the input year is > 30 but returns
	/// a year in the 2000s if the year is <= 68. This is merely a guess and in fact there
	/// really is no good way to reliably convert a two digit year to a four digit year.
	inline int Convert2DigitTo4DigitYear(int nYear2)
		{ return nYear2 > 68 ? (1900 + nYear2) : (2000 + nYear2); } 

	/// DateTimeSecondsToTimeTSeconds
	/// DateTime seconds are based on 0000/01/01/00:00:00 (Year 0000, January 1, midnight).
	/// time_t seconds are based on 1970/01/01/00:00:00.
	inline int64_t DateTimeSecondsToTimeTSeconds(int64_t dateTimeSeconds)
		{ return dateTimeSeconds - INT64_C(62135683200);}

	/// TimeTSecondsSecondsToDateTime
	/// time_t seconds are based on 1970/01/01/00:00:00.
	/// DateTime seconds are based on 0000/01/01/00:00:00 (Year 0000, January 1, midnight).
	inline int64_t TimeTSecondsSecondsToDateTime(int64_t TimeTSeconds)
		{ return TimeTSeconds + INT64_C(62135683200);}

	/// ConvertEpochSeconds
	/// Converts seconds in an epoch to seconds in another epoch. You don't need to convert
	/// the nanoseconds portion of a DateTime (e.g. dateTime.GetNanoseconds()), as that value
	/// is a fractional second which is unchanged regardless of epoch. Recall that an epoch is
	/// a starting timepoint, such as how the Unix Epoch (time_t == 0) starts at Jan 1, 1970.
	/// This function extends and generalizes the functionality in DateTimeSecondsToTimeTSeconds and
	/// TimeTSecondsSecondsToDateTime.
	/// Returns seconds in the destination Epoch. Returns 0 if srcEpoch or destEpoch are invalid.
	/// Example usage:
	///     // Convert myDateTime to Unix time.
	///     int64_t unixSeconds = ConvertEpochSeconds(kEpochDateTime, myDateTime.GetSeconds(), kEpoch1970);
	EASTDC_API int64_t ConvertEpochSeconds(Epoch srcEpoch, int64_t srcSeconds, Epoch destEpoch);

	/// GetCurrent
	/// Returns the current year, month, hour, etc.
	EASTDC_API uint32_t GetCurrent(Parameter parameter, TimeFrame timeFrame = kTimeFrameLocal);

	/// IsDST
	/// Returns true if the current time is daylight savings time. This function assumes 
	/// that DST is valid for the given current locale. Some locales within the 
	/// United States don't observe DST. Daylight savings time is changed every few 
	/// years by the government, so it's probably best not to rely on this function
	/// being accurate. The computer this software is running on may not know the 
	/// current rules for daylight savings time.
	EASTDC_API bool IsDST();

	/// IsDSTDateTime
	/// Returns IsDST for a DateTime seconds value and the current location.
	/// The dateTimeSeconds argument is expected to be in Universal time.
	/// DateTime seconds are based on 0000/01/01/00:00:00 (Year 0000, January 1, midnight) (not time_t).
	EASTDC_API bool IsDSTDateTime(int64_t dateTimeSeconds);

	/// GetDaylightSavingsBias
	/// Returns the number of seconds that the current time is daylight savings adjusted from 
	/// the conventional time. Adding this value to the conventional time yields the time when
	/// adjusted for daylight savings.
	EASTDC_API int64_t GetDaylightSavingsBias();

	/// GetTimeZoneBias
	/// Returns the number of seconds that the local time zone is off of UTC.
	/// Adding this value to the current UTC yields the current local time.
	/// The return value will be zero in the case that EASTDC_UTC_TIME_AVAILABLE == 0.
	EASTDC_API int64_t GetTimeZoneBias();

	/// GetTimeZoneName
	/// Retrieves the name of the time zone. This is not a localizable function.
	/// The supplied pName must have a capacity of at least kTimeZoneNameCapacity bytes.
	/// For example names, see http://www.worldtimezone.com/wtz-names/timezonenames.html.
	/// The returned string isn't guaranteed to match equivalent results from other platforms.
	/// The return value will be false in the case that EASTDC_UTC_TIME_AVAILABLE == 0.
	enum { kTimeZoneNameCapacity = 64 };
	EASTDC_API bool GetTimeZoneName(char* pName, bool bDaylightSavingsName);

	/// DateTimeToTm
	/// Converts a DateTime to a C tm struct.
	EASTDC_API void DateTimeToTm(const DateTime& dateTime, tm& time);

	/// TmToDateTime
	/// Converts a C tm struct to a DateTime.
	EASTDC_API void TmToDateTime(const tm& time, DateTime& dateTime);


	/// DateTimeToFileTime
	/// Converts a DateTime to a FILETIME struct.
	/// A FILETIME contains a 64-bit value representing the number of 100-nanosecond 
	/// intervals since January 1, 1601 (UTC).
	EASTDC_API void DateTimeToFileTime(const DateTime& dateTime, _FILETIME& time);

	/// FileTimeToDateTime
	/// Converts a FILETIME struct to a DateTime.
	EASTDC_API void FileTimeToDateTime(const _FILETIME& time, DateTime& dateTime);


	/// DateTimeToSystemTime
	/// Converts a DateTime to a SYSTEMTIME struct.
	EASTDC_API void DateTimeToSystemTime(const DateTime& dateTime, _SYSTEMTIME& time);

	/// SystemTimeToDateTime
	/// Converts a SYSTEMTIME struct to a DateTime.
	EASTDC_API void SystemTimeToDateTime(const _SYSTEMTIME& time, DateTime& dateTime);


	/// GetTimeOfDay
	/// This is nearly identical to the Posix gettimeofday function, except this function adds a bUTC parameter to allow returning as local time instead of UTC.
	/// The gettimeofday() function shall obtain the current time, expressed as seconds and microseconds 
	/// since the Epoch, and store it in the timeval structure pointed to by tp. The resolution of the 
	/// system clock is unspecified. If tzp is not a null pointer, the behavior is unspecified.
	/// In other words: obtains the current time, expressed as seconds and microseconds since 00:00 Coordinated Universal Time (UTC), January 1, 1970.
	/// Returns 0 upon success or -1 upon error.
	/// Note that a timeval has the same meaning as time_t except that it contains sub-second information.
	/// The bUTC parameter defines if the time is returned in UTC time (a.k.a. Greenwich Mean Time) or local time.
	/// Recall that UTC refers to the time zone in Greenwich, Switzerland, whereas local time refers to whatever 
	/// time zone the computer is currently in.
	/// This function is a calendar function as opposed to being a time progression measurement function. As such it's possible
	/// that the time reported by it may go backwards occasionally, as would be the case when the user's clock is changed.
	/// For a time function that's meant to measure real time progression in a non-adjusting, use the GetTime function.
	/// The time zone parameter may be NULL, in which case the time zone is not returned.
	/// Time zone information will not be available in the case that EASTDC_UTC_TIME_AVAILABLE == 0.
	EASTDC_API int GetTimeOfDay(timeval* pTV, timezone_* pTZ, bool bUTC = true);


	/// TimevalDifference
	/// Calculates the result of TVA - TVB.
	/// Returns 1 if TVA >= TVB, 0 if TVA == TVB, -1 if TVA < TVB. Much like strcmp().
	/// Note that a timeval has the same meaning as time_t except that it contains
	/// sub-second information.
	EASTDC_API int TimevalDifference(const timeval* pTVA, const timeval* pTVB, timeval* pTVResult);


	/// TimeLocale 
	/// Allows user to override strings used by time and date formatting.
	struct TimeLocale
	{
		const char* mAbbrevDay[7];      // e.g. "Sun", etc.
		const char* mDay[7];            // e.g. "Sunday", etc.
		const char* mAbbrevMonth[12];   // e.g. "Jan", etc.
		const char* mMonth[12];         // e.g. "January", etc.
		const char* mAmPm[2];           // e.g. "AM" and "PM".
		const char* mDateTimeFormat;    // e.g. "%a %b %d %H:%M:%S %Y"
		const char* mDateFormat;        // e.g. "%m/%d/%y"
		const char* mTimeFormat;        // e.g. "%H:%M:%S"
		const char* mTimeFormatAmPm;    // e.g. "%I:%M:%S %p"
	};

	/// Strftime
	/// Converts tm struct to time string.
	/// Has equivalent behaviour to the C strftime function as defined by the Posix Standard.
	/// If the total number of resulting bytes including the terminating null byte is not more 
	/// than timeStringCapacity, returns the number of bytes placed into the array pointed to 
	/// by pTimeString, not including the terminating null byte. Otherwise, 0 is returned and 
	/// the contents of the array are indeterminate. 
	/// See http://www.opengroup.org/onlinepubs/009695399/functions/strftime.html
	/// This function doesn't guarantee locale-correctness. For localized time and date 
	/// formatting, consider the EALocale package or Localizer package.11
	EASTDC_API size_t Strftime(char* EA_RESTRICT pTimeString, size_t timeStringCapacity, const char* EA_RESTRICT pFormat, 
								const tm* EA_RESTRICT pTime, const TimeLocale* EA_RESTRICT pTimeLocale = NULL);

	/// Strptime
	/// Converts time string to tm struct.
	/// Has equivalent behaviour to the C strftime function as defined by the Posix Standard.
	/// Returns a pointer to the character following the last character parsed. Otherwise, a null pointer is returned. 
	/// See http://www.opengroup.org/onlinepubs/009695399/functions/strptime.html
	/// This function doesn't guarantee locale-correctness. For localized time and date 
	/// formatting, consider the EALocale package or Localizer package.
	EASTDC_API char* Strptime(const char* EA_RESTRICT pTimeString, const char* EA_RESTRICT pFormat, 
								tm* EA_RESTRICT pTime, const TimeLocale* EA_RESTRICT pTimeLocale = NULL);


} // namespace StdC
} // namespace EA





///////////////////////////////////////////////////////////////////////////////
// DateTime inline operators
///////////////////////////////////////////////////////////////////////////////

namespace EA {
  namespace StdC {

	inline bool operator==(const EA::StdC::DateTime& dateTime1, const EA::StdC::DateTime& dateTime2)
		{ return dateTime1.Compare(dateTime2) == 0; }

	inline bool operator>(const EA::StdC::DateTime& dateTime1, const EA::StdC::DateTime& dateTime2)
		{ return dateTime1.Compare(dateTime2) > 0; }

	inline bool operator>=(const EA::StdC::DateTime& dateTime1, const EA::StdC::DateTime& dateTime2)
		{ return dateTime1.Compare(dateTime2) >= 0; }

	inline bool operator<(const EA::StdC::DateTime& dateTime1, const EA::StdC::DateTime& dateTime2)
		{ return dateTime1.Compare(dateTime2) < 0; }

	inline bool operator<=(const EA::StdC::DateTime& dateTime1, const EA::StdC::DateTime& dateTime2)
		{ return dateTime1.Compare(dateTime2) <= 0; }

	inline bool operator!=(const EA::StdC::DateTime& dateTime1, const EA::StdC::DateTime& dateTime2)
		{ return dateTime1.Compare(dateTime2) != 0; }

  } // namespace StdC
} // namespace EA



#endif // Header include guard











