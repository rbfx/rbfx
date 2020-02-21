///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// stdioEA.h
//
// Declares elements of stdio.h that are missing from various platforms.
// Some platform/compiler combinations don't support some or all of the 
// standard C stdio.h functionality, so we declare the functionality 
// ourselves here. This doesn't always mean that we implement the functionality.
///////////////////////////////////////////////////////////////////////////////


#ifndef EASTDC_INTERNAL_STDIOEA_H
#define EASTDC_INTERNAL_STDIOEA_H


#include <EAStdC/internal/Config.h>


#if !EASTDC_FILE_AVAILABLE

	#include <stdio.h>

	//struct FILE
	//{
	//    char8_t* mpMemory;
	//    int32_t  mPosition;
	//};

	size_t fread(void* ptr, size_t size, size_t count, FILE* stream);
	size_t fwrite(const void* ptr, size_t size, size_t count, FILE* stream);
	int    fwide(FILE* stream, int mode);
	char*  fgets(char* str, int num, FILE* stream);
	int    fputs(const char* str, FILE* stream);
	int    fgetc(FILE* stream);
	int    ungetc(int character, FILE* stream);
	int    feof(FILE* stream);
	int    ferror(FILE* stream);

#endif // EASTDC_FILE_AVAILABLE

#endif // Header include guard
