///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Defines the following:
//    uint32_t FNV1         (const void*     pData, size_t nLength, uint32_t nInitialValue = kFNV1InitialValue);
//    uint32_t FNV1_String8 (const char*  pData, uint32_t nInitialValue = kFNV1InitialValue, CharCase charCase = kCharCaseAny);
//    uint32_t FNV1_String16(const char16_t* pData, uint32_t nInitialValue = kFNV1InitialValue, CharCase charCase = kCharCaseAny);
//    
//    uint64_t FNV64         (const void*     pData, size_t nLength, uint64_t nInitialValue = kFNV1InitialValue);
//    uint64_t FNV64_String8 (const char*  pData, uint64_t nInitialValue = kFNV1InitialValue, CharCase charCase = kCharCaseAny);
//    uint64_t FNV64_String16(const char16_t* pData, uint64_t nInitialValue = kFNV1InitialValue, CharCase charCase = kCharCaseAny);
//    
//    template<> class CTStringHash;
/////////////////////////////////////////////////////////////////////////////


#ifndef EASTDC_EAHASHSTRING_H
#define EASTDC_EAHASHSTRING_H


#include <EAStdC/internal/Config.h>
#include <EABase/eabase.h>
EA_DISABLE_ALL_VC_WARNINGS()
#include <stddef.h>
EA_RESTORE_ALL_VC_WARNINGS()


namespace EA
{
namespace StdC
{

	/// CharCase
	/// 
	/// Defines an enumeration that refers to character case, such as 
	/// upper case and lower case. Hashing functions that work on text
	/// (not necessarily just string hashes) use this enumeration.
	///
	enum CharCase
	{
		kCharCaseAny,   /// Treat the text to be hashed as it is.
		kCharCaseLower, /// Treat the text to be hashed as lower-case.
		kCharCaseUpper  /// Treat the text to be hashed as upper-case.
	};


	/// FNV1 (Fowler / Noll / Vo)
	///
	/// Reference:
	///   http://www.isthe.com/chongo/tech/comp/fnv/
	///
	/// Classification:
	///   String hash. 
	///   FNV1 is not a cryptographic-level hash.
	///
	/// Description:
	///   FNV hashes are designed primarily to hash strings quickly.
	///   They are very fast, as the algorithm uses one multiply and one 
	///   xor per charcter. These hashes are just a little bit slower than
	///   DJB2 hashes on most hardware but yield significantly better 
	///   dispersion and thus lower collision rates.
	///   Note that we implement the FNV1 algorithm and not the FNV1a 
	///   variation. For most data sets there is not a benefit with 
	///   the FNV1a variation.
	///
	/// Notes:
	///   This string hash hashes the characters, not the binary bytes. 
	///   As a result, the hash of "hello" and EA_CHAR16("hello") will yield the 
	///   same result. 
	///
	/// Algorithm:
	///   This hash is basically a loop like this:
	///      while(char c = *pData++)
	///         hash = (hash * 16777619) ^ c;
	///
	const uint32_t kFNV1InitialValue = 2166136261U;

	EASTDC_API uint32_t FNV1         (const void*     pData, size_t nLength, uint32_t nInitialValue = kFNV1InitialValue);
	EASTDC_API uint32_t FNV1_String8 (const char*  pData, uint32_t nInitialValue = kFNV1InitialValue, CharCase charCase = kCharCaseAny);
	EASTDC_API uint32_t FNV1_String16(const char16_t* pData, uint32_t nInitialValue = kFNV1InitialValue, CharCase charCase = kCharCaseAny);
	EASTDC_API uint32_t FNV1_String32(const char32_t* pData, uint32_t nInitialValue = kFNV1InitialValue, CharCase charCase = kCharCaseAny);


	/// FNV64
	/// 64 bit version of the FNV1 algorithm.
	const uint64_t kFNV64InitialValue = UINT64_C(14695981039346656037);

	EASTDC_API uint64_t FNV64         (const void*     pData, size_t nLength, uint64_t nInitialValue = kFNV64InitialValue);
	EASTDC_API uint64_t FNV64_String8 (const char*  pData, uint64_t nInitialValue = kFNV64InitialValue, CharCase charCase = kCharCaseAny);
	EASTDC_API uint64_t FNV64_String16(const char16_t* pData, uint64_t nInitialValue = kFNV64InitialValue, CharCase charCase = kCharCaseAny);
	EASTDC_API uint64_t FNV64_String32(const char32_t* pData, uint64_t nInitialValue = kFNV64InitialValue, CharCase charCase = kCharCaseAny);



	// DJB2 function is deprecated, as FNV1 has been shown to be superior.
	const uint32_t kDJB2InitialValue = 5381;

	EASTDC_API uint32_t DJB2         (const void*     pData, size_t nLength, uint32_t nInitialValue = kDJB2InitialValue);
	EASTDC_API uint32_t DJB2_String8 (const char*  pData, uint32_t nInitialValue = kDJB2InitialValue, CharCase charCase = kCharCaseAny);
	EASTDC_API uint32_t DJB2_String16(const char16_t* pData, uint32_t nInitialValue = kDJB2InitialValue, CharCase charCase = kCharCaseAny);


} // namespace StdC
} // namespace EA



#ifdef EA_COMPILER_MSVC
	#pragma warning(push)
	#pragma warning(disable: 4307) // Integer constant overflow.
	#pragma warning(disable: 4309) // Integer constant overflow.
	#pragma warning(disable: 4310) // Integer constant overflow.
#endif

#define M(x)   (x) // This is a placeholder for modification of x.
#define H(x,h) ((uint32_t)(h * (uint64_t)16777619) ^ x)


namespace EA
{
namespace StdC
{

	/////////////////////////////////////////////////////////////////////////////////
	/// CTStringHash
	///
	/// Compile-time string hash. Generates FNV1 string hashes via compile-time 
	/// template expansion. Both single byte and Unicode characters are accepted.
	///
	/// Example usage:
	///    const uint32_t h = CTStringHash<'T', 'e', 's', 't'>::value;
	///
	/// Example usage:
	///    void DoSomething(const char* pString) {
	///        switch(FNV1_String8(pString)) {
	///           case CTStringHash<'T', 'e', 's', 't'>::value:
	///               break;
	///           case CTStringHash<'H', 'e', 'l', 'l', 'o'>::value:
	///               break;
	///        }
	///    }
	///
	/// If many string constants need to be CTStringHash'd, the following Python script may be useful:
	///    def s(str):
	///    """
	///        Splits a string into a string of its constituent characters: "abc" --> "'a', 'b', 'c'"
	///    """
	///        splitStr = ""
	///        for c in str:
	///            splitStr += "'%s', " % c
	///        print splitStr[:-2]
	///
	///    Example usage:
	///        s("string") // yields 's', 't', 'r', 'i', 'n', 'g'
	///
	template<int c00=0, int c01=0, int c02=0, int c03=0, int c04=0, int c05=0, int c06=0, int c07=0, 
			 int c08=0, int c09=0, int c10=0, int c11=0, int c12=0, int c13=0, int c14=0, int c15=0, 
			 int c16=0, int c17=0, int c18=0, int c19=0, int c20=0, int c21=0, int c22=0, int c23=0, 
			 int c24=0, int c25=0, int c26=0, int c27=0, int c28=0, int c29=0, int c30=0, int c31=0>
	class CTStringHash
	{
	private:
		enum {
			V00=0x811c9dc5,
			V01=H(M(c00),V00), V02=H(M(c01),V01), V03=H(M(c02),V02), V04=H(M(c03),V03), V05=H(M(c04),V04),
			V06=H(M(c05),V05), V07=H(M(c06),V06), V08=H(M(c07),V07), V09=H(M(c08),V08), V10=H(M(c09),V09),
			V11=H(M(c10),V10), V12=H(M(c11),V11), V13=H(M(c12),V12), V14=H(M(c13),V13), V15=H(M(c14),V14),
			V16=H(M(c15),V15), V17=H(M(c16),V16), V18=H(M(c17),V17), V19=H(M(c18),V18), V20=H(M(c19),V19),
			V21=H(M(c20),V20), V22=H(M(c21),V21), V23=H(M(c22),V22), V24=H(M(c23),V23), V25=H(M(c24),V24),
			V26=H(M(c25),V25), V27=H(M(c26),V26), V28=H(M(c27),V27), V29=H(M(c28),V28), V30=H(M(c29),V29),
			V31=H(M(c30),V30), V32=H(M(c31),V31)
		};

	public:
		static const uint32_t value = (uint32_t)
			((c00==0)?V00:((c01==0)?V01:((c02==0)?V02:((c03==0)?V03:((c04==0)?V04:((c05==0)?V05:
			((c06==0)?V06:((c07==0)?V07:((c08==0)?V08:((c09==0)?V09:((c10==0)?V10:((c11==0)?V11:
			((c12==0)?V12:((c13==0)?V13:((c14==0)?V14:((c15==0)?V15:((c16==0)?V16:((c17==0)?V17:
			((c18==0)?V18:((c19==0)?V19:((c20==0)?V20:((c21==0)?V21:((c22==0)?V22:((c23==0)?V23:
			((c24==0)?V24:((c25==0)?V25:((c26==0)?V26:((c27==0)?V27:((c28==0)?V28:((c29==0)?V29:
			((c30==0)?V30:((c31==0)?V31:V32))))))))))))))))))))))))))))))));
	};


} // namespace StdC
} // namespace EA


#undef H
#undef M

#ifdef EA_COMPILER_MSVC
	#pragma warning(pop)
#endif



/////////////////////////////////////////////////////////////////////////////////
// Edit-time string hash Visual Studio macro
// 
// This macro replaces the currently selected text with an FNV1 hash of it.
// It does not understand the concept of escaped characters such as \t, as it 
// merely computes the hash of the \ and t characters indepdendently here.
//
/////////////////////////////////////////////////////////////////////////////////
//   Sub FNV1_32Bit()
//       Dim i As Integer
//       Dim iEnd As Integer
//       Dim FNV1String As String
//       Dim FNV1Value As Long
//       Dim CharAscii As Integer
//       Dim sResult As String
//
//       FNV1String = ActiveDocument().Selection.text()
//       FNV1Value = 2166136261
//
//       For i = 1 To Len(FNV1String)
//           CharAscii = Asc(Mid(FNV1String, i, 1))
//           FNV1Value = (FNV1Value * 16777619) Xor CharAscii
//           FNV1Value = FNV1Value And 4294967295
//       Next
//
//       sResult = "0x{0,8:x8}"
//       sResult = sResult.Format(sResult, FNV1Value)
//       ActiveDocument.Selection.Insert(sResult)
//   End Sub
/////////////////////////////////////////////////////////////////////////////////



#endif // header sentry














