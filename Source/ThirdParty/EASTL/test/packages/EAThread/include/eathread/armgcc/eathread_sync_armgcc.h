///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#if defined(EA_PRAGMA_ONCE_SUPPORTED)
	#pragma once // Some compilers (e.g. VC++) benefit significantly from using this. We've measured 3-4% build speed improvements in apps as a result.
#endif

/////////////////////////////////////////////////////////////////////////////
// Functionality related to memory and code generation synchronization.
//
// Created by Rob Parolin 
/////////////////////////////////////////////////////////////////////////////


#ifndef EATHREAD_ARMGCC_EATHREAD_SYNC_ARMGCC_H
#define EATHREAD_ARMGCC_EATHREAD_SYNC_ARMGCC_H

// Header file should not be included directly.  Provided here for backwards compatibility.
// Please use eathread_sync.h

#if defined(EA_PROCESSOR_ARM) 
	#include <eathread/arm/eathread_sync_arm.h>
#endif

#endif
