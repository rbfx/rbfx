///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////


#ifndef EASTDC_EATEXTUTIL_H
#define EASTDC_EATEXTUTIL_H


#include <EABase/eabase.h>
#include <EAStdC/internal/Config.h>
#include <EAStdC/EAString.h>


namespace EA
{
namespace StdC
{

	/// kLengthNull
	/// Defines a value to be used for string conversion functions which means
	/// that the string length is specified by a wherever the terminating null
	/// character is. For the copying or converting of strings, the terminating
	/// null character is also copied to the destination.
	const size_t kLengthNull = (size_t)-1;


	///////////////////////////////////////////////////////////////////////////////
	/// UTF8 utilities
	/// 
	EASTDC_API bool      UTF8Validate(const char* p, size_t nLength);
	EASTDC_API char*     UTF8Increment(const char* p, size_t n);
	EASTDC_API char*     UTF8Decrement(const char* p, size_t n);
	EASTDC_API size_t    UTF8Length(const char* p);
	EASTDC_API size_t    UTF8Length(const char16_t* p);
	EASTDC_API size_t    UTF8Length(const char32_t* p);
	EASTDC_API size_t    UTF8CharSize(const char* p);
	EASTDC_API size_t    UTF8CharSize(char16_t c);
	EASTDC_API size_t    UTF8CharSize(char32_t c);
	EASTDC_API char16_t  UTF8ReadChar(const char* p, const char** ppEnd = NULL);
	EASTDC_API char*     UTF8WriteChar(char* p, char16_t c);
	EASTDC_API char*     UTF8WriteChar(char* p, char32_t c);
	EASTDC_API size_t    UTF8TrimPartialChar(char* p, size_t nLength);
	EASTDC_API char*     UTF8ReplaceInvalidChar(const char* pIn, size_t nLength, char* pOut, char replaceWith);
	inline     bool      UTF8IsSoloByte(char c)   { return ((uint8_t)c < 0x80); }
	inline     bool      UTF8IsLeadByte(char c)   { return (0xc0 <= (uint8_t)c) && ((uint8_t)c <= 0xef); } // This tests for lead bytes for 2 and 3 byte UTF8 sequences, which map to char16_t code points. If we were to support 4, 5, 6 byte code sequences (char32_t code points), we'd test for <= 0xfd.
	inline     bool      UTF8IsFollowByte(char c) { return (0x80 <= (uint8_t)c) && ((uint8_t)c <= 0xbf); } // This assumes that the char is part of a valid UTF8 sequence.

 #if EA_CHAR8_UNIQUE 
	// We simply forward the UTF8 overload to maintain backwards compatbility.
	// Eventually, we should leverage the type system instead of UTF8 specific functions.
	//
	inline size_t UTF8Length(const char8_t* p) { return UTF8Length((const char*)p); }
	inline char8_t* UTF8ReplaceInvalidChar(const char8_t* pIn, size_t nLength, char8_t* pOut, char8_t replaceWith) 
		{ return (char8_t*)UTF8ReplaceInvalidChar((const char*)pIn, nLength, (char*)pOut, (char)replaceWith); }
#endif

	///////////////////////////////////////////////////////////////////////////////
	/// WildcardMatch
	/// 
	/// These functions match source strings to wildcard patterns like those used
	/// in file specifications. '*' in the pattern means match zero or more 
	/// consecutive source characters. '?' in the pattern means match exactly one
	/// source character. Here are some examples:
	///
	///      Source            Pattern          Result
	///      -------------------------------------------
	///      abcde             *e                true
	///      abcde             *f                false
	///      abcde             ???de             true
	///      abcde             ????g             false
	///      abcde             *c??              true
	///      abcde             *e??              false
	///      abcde             *????             true
	///      abcde             bcdef             false
	///      abcde             *?????            true
	///
	/// Multiple * and ? characters may be used. Two consecutive * characters are 
	/// treated as if they were one.
	///
	EASTDC_API bool WildcardMatch(const char*  pString, const char*  pPattern, bool bCaseSensitive);
	EASTDC_API bool WildcardMatch(const char16_t* pString, const char16_t* pPattern, bool bCaseSensitive);
	EASTDC_API bool WildcardMatch(const char32_t* pString, const char32_t* pPattern, bool bCaseSensitive);

	#if defined(EA_WCHAR_UNIQUE) && EA_WCHAR_UNIQUE
		inline bool WildcardMatch(const wchar_t* pString, const wchar_t* pPattern, bool bCaseSensitive)
			{ return WildcardMatch(EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pString), EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pPattern), bCaseSensitive); }
	#endif


	////////////////////////////////////////////////////////////////////////////////
	/// ParseDelimitedText
	///
	/// Given a line of text (e.g. like this:)
	///     342.5, "This is a string", test, "This is a string, with a comma"
	/// This function parses it into separate fields (e.g. like this:)
	///     342.5
	///     This is a string
	///     test
	///     This is a string, with a comma
	/// This function lets you dynamically specify the delimiter. The delimiter
	/// can be any char (e.g. space, tab, semicolon) except the quote char
	/// itself, which is reserved for the purpose of grouping. See the source
	/// code comments for more details. However, in the case of text that is
	/// UTF8-encoded, you need to make sure the delimiter char is a value
	/// that is less than 127, so as not to collide with UTF8 encoded chars.
	///
	/// The input is a pointer to text and the text length. For ASCII, MBCS, and
	/// UTF8, this is the number of bytes or chars. For UTF16 (Unicode) it is
	/// the number of characters. There are two bytes (two chars) per character
	/// in UTF16. 
	///
	/// The input nTextLength can be -1 (kLengthNull) to indicate that the string 
	/// is null-terminated. 
	///
	/// See the other version of ParseDelimitedText below for some additional documentation.
	///
	EASTDC_API bool ParseDelimitedText(const char* pText, const char* pTextEnd, char cDelimiter, 
									   const char*& pToken, const char*& pTokenEnd, const char** ppNewText);

	EASTDC_API bool ParseDelimitedText(const char16_t* pText, const char16_t* pTextEnd, char16_t cDelimiter, 
									   const char16_t*& pToken, const char16_t*& pTokenEnd, const char16_t** ppNewText);

	EASTDC_API bool ParseDelimitedText(const char32_t* pText, const char32_t* pTextEnd, char32_t cDelimiter, 
									   const char32_t*& pToken, const char32_t*& pTokenEnd, const char32_t** ppNewText);

	#if defined(EA_WCHAR_UNIQUE) && EA_WCHAR_UNIQUE
		inline bool ParseDelimitedText(const wchar_t* pText, const wchar_t* pTextEnd, wchar_t cDelimiter, 
									   const wchar_t*& pToken, const wchar_t*& pTokenEnd, const wchar_t** ppNewText)
			{ return ParseDelimitedText(EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pText), EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pTextEnd), EASTDC_UNICODE_CHAR_CAST(cDelimiter), 
										EASTDC_UNICODE_CONST_CHAR_PTR_REF_CAST(pToken), EASTDC_UNICODE_CONST_CHAR_PTR_REF_CAST(pTokenEnd), EASTDC_UNICODE_CONST_CHAR_PTR_PTR_CAST(ppNewText)); }
	#endif




	/// ConvertBinaryDataToASCIIArray
	///
	/// These functions convert an array of binary characters into an encoded ASCII
	/// format that can be later converted back to binary. You might want to do this
	/// if you are trying to embed binary data into a text file (e.g. .ini file)
	/// and need a way to encode the binary data as text.
	/// 
	/// Example input:
	///    { 0x12, 0x34, 0x56, 0x78 }
	/// Resulting output:
	///    "12345678"
	///
	EASTDC_API void ConvertBinaryDataToASCIIArray(const void* pBinaryData, size_t nBinaryDataLength, char*  pASCIIArray);
	EASTDC_API void ConvertBinaryDataToASCIIArray(const void* pBinaryData, size_t nBinaryDataLength, char16_t* pASCIIArray);
	EASTDC_API void ConvertBinaryDataToASCIIArray(const void* pBinaryData, size_t nBinaryDataLength, char32_t* pASCIIArray);

	#if defined(EA_WCHAR_UNIQUE) && EA_WCHAR_UNIQUE
		inline void ConvertBinaryDataToASCIIArray(const void* pBinaryData, size_t nBinaryDataLength, wchar_t* pASCIIArray)
			{ ConvertBinaryDataToASCIIArray(pBinaryData, nBinaryDataLength, EASTDC_UNICODE_CHAR_PTR_CAST(pASCIIArray)); }
	#endif


	/// ConvertASCIIArrayToBinaryData
	///
	/// Takes an ASCII string of text and converts it to binary data. This is the reverse
	/// of the ConvertBinaryDataToASCIIArray function. If an invalid hexidecimal character
	/// is encountered, it is replaced with a '0' character.
	/// Returns true if the input was entirely valid hexadecimal data.
	/// 
	/// Example input:
	///    "12345678"
	/// Resulting output:
	///    { 0x12, 0x34, 0x56, 0x78 }
	///
	EASTDC_API bool ConvertASCIIArrayToBinaryData(const char*  pASCIIArray, size_t nASCIIArrayLength, void* pBinaryData);
	EASTDC_API bool ConvertASCIIArrayToBinaryData(const char16_t* pASCIIArray, size_t nASCIIArrayLength, void* pBinaryData);
	EASTDC_API bool ConvertASCIIArrayToBinaryData(const char32_t* pASCIIArray, size_t nASCIIArrayLength, void* pBinaryData);

	#if defined(EA_WCHAR_UNIQUE) && EA_WCHAR_UNIQUE
		inline bool ConvertASCIIArrayToBinaryData(const wchar_t*  pASCIIArray, size_t nASCIIArrayLength, void* pBinaryData)
			{ return ConvertASCIIArrayToBinaryData(EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pASCIIArray), nASCIIArrayLength, pBinaryData); }
	#endif




	///////////////////////////////////////////////////////////////////////////////
	/// GetTextLine
	///
	/// Given a block of text, this function reads a line of text and moves to the 
	/// beginning of the next line. The return value is the end of the current line,
	/// previous to the newline characters. 
	/// Lines are defined as ending in \n, \r, \r\n, or \n\r sequences.
	/// If ppNewText is supplied, it holds the start of the new line, which will often
	/// be different from the return value, as the start of the new line is after any
	/// newline characters. The length of the current line is pTextEnd - pText.
	///
	/// Example usage (assumes that :
	///     cosnt char  buffer[30] = "line1\nLine2\r\nLine3\rLine4\nLine5"; // strlen(buffer) == 30.
	///     const char* pLineNext(buffer);
	///     const char* pLine;
	///
	///     do{
	///         pLine = pLineNext;
	///         const char* pLineEnd = GetTextLine(pLine, buffer + EAArrayCount(buffer), &pLineNext);
	///         // Use pLine - pLineEnd
	///     }while(pLineNext != (buffer + 256));
	///
	EASTDC_API const char*  GetTextLine(const char* pText, const char* pTextEnd, const char** ppNewText);
	EASTDC_API const char16_t* GetTextLine(const char16_t* pText, const char16_t* pTextEnd, const char16_t** ppNewText);
	EASTDC_API const char32_t* GetTextLine(const char32_t* pText, const char32_t* pTextEnd, const char32_t** ppNewText);

	#if defined(EA_WCHAR_UNIQUE) && EA_WCHAR_UNIQUE
		inline const wchar_t* GetTextLine(const wchar_t* pText, const wchar_t* pTextEnd, const wchar_t** ppNewText)
			{ return reinterpret_cast<const wchar_t *>(GetTextLine(EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pText), EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pTextEnd), EASTDC_UNICODE_CONST_CHAR_PTR_PTR_CAST(ppNewText))); }
	#endif

	
	/// GetTextLine
	///
	/// Retrieves the line in the pLine output argument. Lines are defined as ending in 
	/// \n, \r, \r\n, or \n\r sequences.
	/// Returns true if there is any line to retrieve. Returns false only if sSource is empty.
	/// The retrieved pLine does not have the final newline character(s).
	/// sSource is modified to have the line removed. This gets increasingly less efficient
	/// as the length of sSource increases for the case that sSource is a typical STL-like
	/// linear string. 
	/// If pLine is NULL then the behavior is the same except no copy is done to pLine.
	///
	/// Requires that String support the following STL-like data functions: 
	///     data(), length(), erase(pos, count), assign(begin, end).
	///
	/// Example usage:
	///     eastl::stringW s(L"line1\nLine2\r\nLine3\rLine4\nLine5");
	///     eastl::stringW line;
	/// 
	///     while(GetTextLine(s, &line))
	///          printf("%ls\n", line.c_str());
	///
	template<typename String>
	bool GetTextLine(String& sSource, String* pLine);


	///////////////////////////////////////////////////////////////////////////////
	/// SplitTokenDelimited
	///
	/// This function returns tokens that are delimited by a single character --
	/// repetitions of that character will result in empty tokens returned.
	/// This is most commonly useful when you want to parse a string of text
	/// delimited by commas or spaces.
	/// 
	/// Returns true whenever it extracts a token. Note however that the extracted
	/// token may be empty. Note that the return value will be true if the source
	/// has length and will be false if the source is empty.
	/// If the input pToken is non-null, the text before the delimiter is copied to it. 
	///
	/// Examples (delimiter is comma):
	///    source      token    new source   return
	///    -----------------------------------------
	///    "a,b"       "a"      "b"          true
	///    " a , b "   " a "    " b "        true
	///    "ab,b"      "ab"     "b"          true
	///    ",a,b"      ""       "a,b"        true
	///    ",b"        ""       "b"          true
	///    ",,b"       ""       ",b"         true
	///    ",a,"       ""       "a,"         true
	///    "a,"        "a"      ""           true
	///    ","         ""       ""           true
	///    ", "        ""       " "          true
	///    "a"         "a"      ""           true
	///    " "         " "      ""           true
	///    ""          ""       ""           false
	///    NULL        ""       NULL         false
	///
	///  Example usage:
	///    const char16_t* pString = EA_CHAR16("a, b, c, d");
	///    char16_t pToken[16];
	///    
	///    while(SplitTokenDelimited(pString, kLengthNull, ',', pToken, 16, &pString))
	///         printf("%s\n", pToken);
	///    
	EASTDC_API bool SplitTokenDelimited(const char*  pSource, size_t nSourceLength, char  cDelimiter, char*  pToken, size_t nTokenLength, const char**  ppNewSource = NULL);
	EASTDC_API bool SplitTokenDelimited(const char16_t* pSource, size_t nSourceLength, char16_t cDelimiter, char16_t* pToken, size_t nTokenLength, const char16_t** ppNewSource = NULL);
	EASTDC_API bool SplitTokenDelimited(const char32_t* pSource, size_t nSourceLength, char32_t cDelimiter, char32_t* pToken, size_t nTokenLength, const char32_t** ppNewSource = NULL);

	#if defined(EA_WCHAR_UNIQUE) && EA_WCHAR_UNIQUE
		inline const wchar_t* SplitTokenDelimited(const wchar_t* pSource, size_t nSourceLength, wchar_t cDelimiter, wchar_t* pToken, size_t nTokenLength, const wchar_t** ppNewSource = NULL)
			{ return reinterpret_cast<const wchar_t*>(SplitTokenDelimited(EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pSource), nSourceLength, EASTDC_UNICODE_CHAR_CAST(cDelimiter), EASTDC_UNICODE_CHAR_PTR_CAST(pToken), nTokenLength, EASTDC_UNICODE_CONST_CHAR_PTR_PTR_CAST(ppNewSource))); }
	#endif

	template<typename String, typename Char>
	bool SplitTokenDelimited(String& sSource, Char cDelimiter, String* pToken);


	///////////////////////////////////////////////////////////////////////////////
	/// SplitTokenSeparated
	///
	/// This function returns tokens that are separated by one or more instances
	/// of a character. Returns true whenever it extracts a token. 
	///
	/// Examples (delimiter is space):
	///   source    token    new source   return
	///   ---------------------------------------
	///    "a"       "a"      ""           true
	///    "a b"     "a"      "b"          true
	///    "a  b"    "a"      "b"          true
	///    " a b"    "a"      "b"          true
	///    " a b "   "a"      "b "         true
	///    " a "     "a"      ""           true
	///    " a  "    "a"      ""           true
	///    ""        ""       ""           false
	///    " "       ""       ""           false
	///    NULL      ""       NULL         false
	///
	EASTDC_API bool SplitTokenSeparated(const char*  pSource, size_t nSourceLength, char  cDelimiter, char*  pToken, size_t nTokenLength, const char**  ppNewSource = NULL);
	EASTDC_API bool SplitTokenSeparated(const char16_t* pSource, size_t nSourceLength, char16_t cDelimiter, char16_t* pToken, size_t nTokenLength, const char16_t** ppNewSource = NULL);
	EASTDC_API bool SplitTokenSeparated(const char32_t* pSource, size_t nSourceLength, char32_t cDelimiter, char32_t* pToken, size_t nTokenLength, const char32_t** ppNewSource = NULL);

	#if defined(EA_WCHAR_UNIQUE) && EA_WCHAR_UNIQUE
		inline const wchar_t* SplitTokenSeparated(const wchar_t* pSource, size_t nSourceLength, wchar_t cDelimiter, wchar_t* pToken, size_t nTokenLength, const wchar_t** ppNewSource = NULL)
			{ return reinterpret_cast<const wchar_t*>(SplitTokenSeparated(EASTDC_UNICODE_CONST_CHAR_PTR_CAST(pSource), nSourceLength, EASTDC_UNICODE_CHAR_CAST(cDelimiter), EASTDC_UNICODE_CHAR_PTR_CAST(pToken), nTokenLength, EASTDC_UNICODE_CONST_CHAR_PTR_PTR_CAST(ppNewSource))); }
	#endif

	template<typename String, typename Char>
	bool SplitTokenSeparated(String& sSource, Char c, String* pToken);


	////////////////////////////////////////////////////////////////////////////////
	/// Boyer-Moore string search
	///
	/// This is the "turbo" implementation defined at http://www-igm.univ-mlv.fr/~lecroq/string/node14.html#SECTION00140.
	/// Boyer-Moore is a very fast string search compared to most others, including
	/// those in the STL. However, you need to be searching a string of at least 100
	/// chars and have a search pattern of at least 3 characters for the speed to show,
	/// as Boyer-Moore has a startup precalculation that costs some cycles. 
	/// This startup precalculation is proportional to the size of your search pattern
	/// and the size of the alphabet in use. Thus, doing Boyer-Moore searches on the 
	/// entire Unicode alphabet is going to incur a fairly expensive precalculation cost.
	///
	/// patternBuffer1 is a user-supplied buffer and must be at least as long as the search pattern.
	/// patternBuffer2 is a user-supplied buffer and must be at least as long as the search pattern.
	/// alphabetBuffer is a user-supplied buffer and must be at least as long as the highest character value used in the searched string and search pattern.
	///
	EASTDC_API int BoyerMooreSearch(const char* pPattern, int nPatternLength, const char* pSearchString, int nSearchStringLength, 
									int* pPatternBuffer1, int* pPatternBuffer2, int* pAlphabetBuffer, int nAlphabetBufferSize);


} // namespace StdC
} // namespace EA



namespace EA
{
	namespace StdC
	{

		template<typename String>
		bool GetTextLine(String& sSource, String* pLine)
		{
			typedef typename String::value_type Char;

			bool        bReturnValue = false;
			const Char* pText    = sSource.data();
			const Char* pTextEnd = pText + sSource.length();

			if(pText < pTextEnd)
			{
				// Read until the first \r or \n. 
				while((pText < pTextEnd) && (*pText != '\r') && (*pText != '\n'))
					++pText;

				if(pLine)
					pLine->assign(sSource.data(), pText);

				// Find the end of the line, which will be 0, 1, or 2 chars past pText.
				const Char* pTextCurrent = pText;

				if(pTextCurrent < pTextEnd)
				{
					// Read past a second newline character, e.g. as part of a /r/n sequence.
					if((++pTextCurrent < pTextEnd) && (*pTextCurrent ^ *pText) == ('\r' ^ '\n'))
						++pTextCurrent;
				}

				sSource.erase(0, pTextCurrent - sSource.data());

				bReturnValue = true;
			}

			return bReturnValue;
		}


		template<typename String, typename Char>
		bool SplitTokenDelimited(String& sSource, Char cDelimiter, String* pToken)
		{
			if(pToken)
				pToken->clear();

			if(!sSource.empty())
			{
				// find the first occurence of the delimiter
				const size_t nIndex = sSource.find(cDelimiter);

				if(nIndex == String::npos) // If we didn't find the delimiter...
				{
					if(pToken) // If we have the token
						pToken->swap(sSource);  // swap it with the source string ( it will clear the source )
					else
						sSource.erase(); // simply clear the source
				}
				else
				{
					if(pToken) // If we found a delimiter - move the string before it to the token, if present
						pToken->assign(sSource, 0, (typename String::size_type)nIndex);

					sSource.erase(0, (typename String::size_type)nIndex + 1); // remove the token + delimiter from the source
				}

				return true; // there are tokens remaining
			}

			return false;
		}

		template<typename String, typename Char>
		bool SplitTokenSeparated(String& sSource, Char c, String* pToken)
		{
			// loop until we are done
			for(;;)
			{
				// look for first occurance of the separator
				const size_t nIndex1 = sSource.find(c);

				// didn't find any
				if(nIndex1 == String::npos)
				{
					// no text available
					if(sSource.empty())
					{
						// clear token
						if(pToken)
							pToken->erase();

						// no token found - return false
						return false;
					}
					else
					{
						// no separators, but we are not empty - fill the token and clear the source
						if(pToken)
						{
							// clear it
							pToken->erase();
							
							// swap it with the source string ( it will clear the source )
							pToken->swap(sSource);
						}
						else
						{
							// simply clear the source
							sSource.erase();
						}

						// token found - return true
						return true;
					}
				}

				// find first ( or second ) occurence of a non-separator character
				const size_t nIndex2 = sSource.find_first_not_of(c, (typename String::size_type)nIndex1);

				// if we found a non-empty string in front of separators, extract it and exit
				if(nIndex1 > 0)
				{
					// add it to the token
					if(pToken)
						pToken->assign(sSource, 0, (typename String::size_type)nIndex1);

					// remove all we found so far from the source
					sSource.erase(0, (typename String::size_type)nIndex2);

					// token found - return true
					return true;
				}

				// Remove initial separator and loop back to try again
				sSource.erase(0, (typename String::size_type)nIndex2);
			}

		   // return false;   // This should never get executed, but some compilers might not be smart enough to realize it.
		}
	}
}




#endif // Header include guard












