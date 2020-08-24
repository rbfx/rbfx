/*
  Native File Dialog

  User API

  http://www.frogtoss.com/labs
 */


#ifndef _NFD_H
#define _NFD_H

#if _WIN32
#   if NFD_EXPORTS
#       define NFD_API __declspec(dllexport)
#   elif NDF_IMPORTS
#       define NFD_API __declspec(dllimport)
#	else
#		define NFD_API
#   endif
#elif NDF_EXPORTS || NDF_IMPORTS
#   define NFD_API __attribute__((visibility("default")))
#else
#   define NFD_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

/* denotes UTF-8 char */
typedef char nfdchar_t;

/* opaque data structure -- see NFD_PathSet_* */
typedef struct {
    nfdchar_t *buf;
    size_t *indices; /* byte offsets into buf */
    size_t count;    /* number of indices into buf */
}nfdpathset_t;

typedef enum {
    NFD_ERROR,       /* programmatic error */
    NFD_OKAY,        /* user pressed okay, or successful return */
    NFD_CANCEL       /* user pressed cancel */
}nfdresult_t;
    

/* nfd_<targetplatform>.c */

/* single file open dialog */    
NFD_API
nfdresult_t NFD_OpenDialog( const nfdchar_t *filterList,
                            const nfdchar_t *defaultPath,
                            nfdchar_t **outPath,
                            void* owner );

/* multiple file open dialog */    
NFD_API
nfdresult_t NFD_OpenDialogMultiple( const nfdchar_t *filterList,
                                    const nfdchar_t *defaultPath,
                                    nfdpathset_t *outPaths );

/* save dialog */
NFD_API
nfdresult_t NFD_SaveDialog( const nfdchar_t *filterList,
                            const nfdchar_t *defaultPath,
                            nfdchar_t **outPath,
                            void* owner );


/* select folder dialog */
NFD_API
nfdresult_t NFD_PickFolder( const nfdchar_t *defaultPath,
                            nfdchar_t **outPath);

/* nfd_common.c */

/* get last error -- set when nfdresult_t returns NFD_ERROR */
NFD_API
const char *NFD_GetError( void );
/* get the number of entries stored in pathSet */
NFD_API
size_t      NFD_PathSet_GetCount( const nfdpathset_t *pathSet );
/* Get the UTF-8 path at offset index */
NFD_API
nfdchar_t  *NFD_PathSet_GetPath( const nfdpathset_t *pathSet, size_t index );
/* Free the pathSet */    
NFD_API
void        NFD_PathSet_Free( nfdpathset_t *pathSet );


/* free the memory allocated for a path */
NFD_API
void        NFD_FreePath( nfdchar_t *outPath );

#ifdef __cplusplus
}
#endif

#endif
