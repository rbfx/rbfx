///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

/**
\file
Define the major, minor and patch versions.
*/

#ifndef EAASSERT_VERSION_H
#define EAASSERT_VERSION_H

// Define the major, minor and patch versions.
// This information is updated with each release.

//! This define indicates the major version number for the filesys package.
//! \sa EAASSERT_VERSION_MAJOR
#define EAASSERT_VERSION_MAJOR       1
//! This define indicates the minor version number for the filesys package.
//! \sa EAASSERT_VERSION_MINOR
#define EAASSERT_VERSION_MINOR       5
//! This define indicates the patch version number for the filesys package.
//! \sa EAASSERT_VERSION_PATCH
#define EAASSERT_VERSION_PATCH       8

/*!
 * This is a utility macro that users may use to create a single version number
 * that can be compared against EAASSERT_VERSION.
 *
 * For example:
 *
 * \code
 *
 * #if EAASSERT_VERSION > EAASSERT_CREATE_VERSION_NUMBER( 1, 1, 0 )
 * printf("Filesys version is greater than 1.1.0.\n");
 * #endif
 *
 * \endcode
 */
#define EAASSERT_CREATE_VERSION_NUMBER( major_ver, minor_ver, patch_ver ) \
	((major_ver) * 1000000 + (minor_ver) * 1000 + (patch_ver))

/*!
 * This macro is an aggregate of the major, minor and patch version numbers.
 * \sa EAASSERT_CREATE_VERSION_NUMBER
 */
#define EAASSERT_VERSION \
	EAASSERT_CREATE_VERSION_NUMBER( EAASSERT_VERSION_MAJOR, EAASSERT_VERSION_MINOR, EAASSERT_VERSION_PATCH )

#define EAASSERT_VERSION_MAJOR_STR EAASSERT_VERSION_STRINGIFY(EAASSERT_VERSION_MAJOR)
#if EAASSERT_VERSION_MINOR >= 10
	#define EAASSERT_VERSION_MINOR_STR EAASSERT_VERSION_STRINGIFY(EAASSERT_VERSION_MINOR)
#else
	#define EAASSERT_VERSION_MINOR_STR "0" EAASSERT_VERSION_STRINGIFY(EAASSERT_VERSION_MINOR)
#endif

#if EAASSERT_VERSION_PATCH >= 10
	#define EAASSERT_VERSION_PATCH_STR EAASSERT_VERSION_STRINGIFY(EAASSERT_VERSION_PATCH)
#else
	#define EAASSERT_VERSION_PATCH_STR "0" EAASSERT_VERSION_STRINGIFY(EAASSERT_VERSION_PATCH)
#endif

/*!
 * This macro returns a string version of the macro
 * \sa EAASSERT_VERSION_STRING
 */
#define EAASSERT_VERSION_STRING EAASSERT_VERSION_MAJOR_STR "." EAASSERT_VERSION_MINOR_STR "." EAASSERT_VERSION_PATCH_STR
	
#endif // EAASSERT_VERSION_H
