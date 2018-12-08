/* Copyright (c) <2018-2018> <Newton Game Dynamics>
*
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
*
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely
*/

#ifndef __DTIMETRACKER__
#define __DTIMETRACKER__


// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the DTIMETRACKER_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// DTIMETRACKER_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef DTIMETRACKER_EXPORTS
#define DTIMETRACKER_API __declspec(dllexport)
#else
#define DTIMETRACKER_API __declspec(dllimport)
#endif

DTIMETRACKER_API void ttStartRecording(const char* const fileName);
DTIMETRACKER_API void ttStopRecording();

DTIMETRACKER_API int ttOpenRecord(const char* const name);
DTIMETRACKER_API void ttCloseRecord(int record);

DTIMETRACKER_API void ttDeleteTrack();
DTIMETRACKER_API void ttSetTrackName(const char* const threadName);

#endif