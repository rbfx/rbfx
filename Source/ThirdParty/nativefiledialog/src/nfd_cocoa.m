/*
  Native File Dialog Extended
  Repository: https://github.com/btzy/nativefiledialog-extended
  License: Zlib
  Authors: Bernard Teo, Michael Labbe
 */

#include <AppKit/AppKit.h>
#include <Availability.h>
#include "nfd.h"

// MacOS is deprecating the allowedFileTypes property in favour of allowedContentTypes, so we have
// to introduce this breaking change.  Define NFD_MACOS_ALLOWEDCONTENTTYPES to 1 to have it set the
// allowedContentTypes property of the SavePanel or OpenPanel. Define
// NFD_MACOS_ALLOWEDCONTENTTYPES to 0 to have it set the allowedFileTypes property of the SavePanel
// or OpenPanel.  If NFD_MACOS_ALLOWEDCONTENTTYPES is undefined, then it will set it to 1 if
// __MAC_OS_X_VERSION_MIN_REQUIRED >= 11.0, and 0 otherwise.
#if !defined(NFD_MACOS_ALLOWEDCONTENTTYPES)
#if !defined(__MAC_OS_X_VERSION_MIN_REQUIRED) || !defined(__MAC_11_0) || \
    __MAC_OS_X_VERSION_MIN_REQUIRED < __MAC_11_0
#define NFD_MACOS_ALLOWEDCONTENTTYPES 0
#else
#define NFD_MACOS_ALLOWEDCONTENTTYPES 1
#endif
#endif

#if NFD_MACOS_ALLOWEDCONTENTTYPES == 1
#include <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#endif

static const char* g_errorstr = NULL;

static void NFDi_SetError(const char* msg) {
    g_errorstr = msg;
}

static void* NFDi_Malloc(size_t bytes) {
    void* ptr = malloc(bytes);
    if (!ptr) NFDi_SetError("NFDi_Malloc failed.");

    return ptr;
}

static void NFDi_Free(void* ptr) {
    assert(ptr);
    free(ptr);
}

#if NFD_MACOS_ALLOWEDCONTENTTYPES == 1
// Returns an NSArray of UTType representing the content types.
static NSArray* BuildAllowedContentTypes(const nfdnfilteritem_t* filterList,
                                         nfdfiltersize_t filterCount) {
    NSMutableArray* buildFilterList = [[NSMutableArray alloc] init];

    for (nfdfiltersize_t filterIndex = 0; filterIndex != filterCount; ++filterIndex) {
        // this is the spec to parse (we don't use the friendly name on OS X)
        const nfdnchar_t* filterSpec = filterList[filterIndex].spec;

        const nfdnchar_t* p_currentFilterBegin = filterSpec;
        for (const nfdnchar_t* p_filterSpec = filterSpec; *p_filterSpec; ++p_filterSpec) {
            if (*p_filterSpec == ',') {
                // add the extension to the array
                NSString* filterStr = [[NSString alloc]
                    initWithBytes:(const void*)p_currentFilterBegin
                           length:(sizeof(nfdnchar_t) * (p_filterSpec - p_currentFilterBegin))
                         encoding:NSUTF8StringEncoding];
                UTType* filterType = [UTType typeWithFilenameExtension:filterStr
                                                      conformingToType:UTTypeData];
                [filterStr release];
                if (filterType) [buildFilterList addObject:filterType];
                p_currentFilterBegin = p_filterSpec + 1;
            }
        }
        // add the extension to the array
        NSString* filterStr = [[NSString alloc] initWithUTF8String:p_currentFilterBegin];
        UTType* filterType = [UTType typeWithFilenameExtension:filterStr
                                              conformingToType:UTTypeData];
        [filterStr release];
        if (filterType) [buildFilterList addObject:filterType];
    }

    NSArray* returnArray = [NSArray arrayWithArray:buildFilterList];

    [buildFilterList release];

    assert([returnArray count] != 0);

    return returnArray;
}
#else
// Returns an NSArray of NSString representing the file types.
static NSArray* BuildAllowedFileTypes(const nfdnfilteritem_t* filterList,
                                      nfdfiltersize_t filterCount) {
    NSMutableArray* buildFilterList = [[NSMutableArray alloc] init];

    for (nfdfiltersize_t filterIndex = 0; filterIndex != filterCount; ++filterIndex) {
        // this is the spec to parse (we don't use the friendly name on OS X)
        const nfdnchar_t* filterSpec = filterList[filterIndex].spec;

        const nfdnchar_t* p_currentFilterBegin = filterSpec;
        for (const nfdnchar_t* p_filterSpec = filterSpec; *p_filterSpec; ++p_filterSpec) {
            if (*p_filterSpec == ',') {
                // add the extension to the array
                NSString* filterStr = [[[NSString alloc]
                    initWithBytes:(const void*)p_currentFilterBegin
                           length:(sizeof(nfdnchar_t) * (p_filterSpec - p_currentFilterBegin))
                         encoding:NSUTF8StringEncoding] autorelease];
                [buildFilterList addObject:filterStr];
                p_currentFilterBegin = p_filterSpec + 1;
            }
        }
        // add the extension to the array
        NSString* filterStr = [NSString stringWithUTF8String:p_currentFilterBegin];
        [buildFilterList addObject:filterStr];
    }

    NSArray* returnArray = [NSArray arrayWithArray:buildFilterList];

    [buildFilterList release];

    assert([returnArray count] != 0);

    return returnArray;
}
#endif

static void AddFilterListToDialog(NSSavePanel* dialog,
                                  const nfdnfilteritem_t* filterList,
                                  nfdfiltersize_t filterCount) {
    // note: NSOpenPanel inherits from NSSavePanel.

    if (!filterCount) return;

    assert(filterList);

// Make NSArray of file types and set it on the dialog
// We use setAllowedFileTypes or setAllowedContentTypes depending on the deployment target
#if NFD_MACOS_ALLOWEDCONTENTTYPES == 1
    NSArray* allowedContentTypes = BuildAllowedContentTypes(filterList, filterCount);
    [dialog setAllowedContentTypes:allowedContentTypes];
#else
    NSArray* allowedFileTypes = BuildAllowedFileTypes(filterList, filterCount);
    [dialog setAllowedFileTypes:allowedFileTypes];
#endif
}

static void SetDefaultPath(NSSavePanel* dialog, const nfdnchar_t* defaultPath) {
    if (!defaultPath || !*defaultPath) return;

    NSString* defaultPathString = [NSString stringWithUTF8String:defaultPath];
    NSURL* url = [NSURL fileURLWithPath:defaultPathString isDirectory:YES];
    [dialog setDirectoryURL:url];
}

static void SetDefaultName(NSSavePanel* dialog, const nfdnchar_t* defaultName) {
    if (!defaultName || !*defaultName) return;

    NSString* defaultNameString = [NSString stringWithUTF8String:defaultName];
    [dialog setNameFieldStringValue:defaultNameString];
}

static nfdresult_t CopyUtf8String(const char* utf8Str, nfdnchar_t** out) {
    // byte count, not char count
    size_t len = strlen(utf8Str);

    // Too bad we have to use additional memory for all the result paths,
    // because we cannot reconstitute an NSString from a char* to release it properly.
    *out = (nfdnchar_t*)NFDi_Malloc(len + 1);
    if (*out) {
        strcpy(*out, utf8Str);
        return NFD_OKAY;
    }

    return NFD_ERROR;
}

static NSWindow* GetNativeWindowHandle(const nfdwindowhandle_t* parentWindow) {
    if (parentWindow->type != NFD_WINDOW_HANDLE_TYPE_COCOA) {
        return NULL;
    }
    return (NSWindow*)parentWindow->handle;
}

/* public */

const char* NFD_GetError(void) {
    return g_errorstr;
}

void NFD_ClearError(void) {
    NFDi_SetError(NULL);
}

void NFD_FreePathN(nfdnchar_t* filePath) {
    NFDi_Free((void*)filePath);
}

void NFD_FreePathU8(nfdu8char_t* filePath) {
    NFD_FreePathN(filePath);
}

static NSApplicationActivationPolicy old_app_policy;

nfdresult_t NFD_Init(void) {
    NSApplication* app = [NSApplication sharedApplication];
    old_app_policy = [app activationPolicy];
    if (old_app_policy == NSApplicationActivationPolicyProhibited) {
        if (![app setActivationPolicy:NSApplicationActivationPolicyAccessory]) {
            NFDi_SetError("Failed to set activation policy.");
            return NFD_ERROR;
        }
    }
    return NFD_OKAY;
}

/* call this to de-initialize NFD, if NFD_Init returned NFD_OKAY */
void NFD_Quit(void) {
    [[NSApplication sharedApplication] setActivationPolicy:old_app_policy];
}

nfdresult_t NFD_OpenDialogN(nfdnchar_t** outPath,
                            const nfdnfilteritem_t* filterList,
                            nfdfiltersize_t filterCount,
                            const nfdnchar_t* defaultPath) {
    nfdopendialognargs_t args = {0};
    args.filterList = filterList;
    args.filterCount = filterCount;
    args.defaultPath = defaultPath;
    return NFD_OpenDialogN_With_Impl(NFD_INTERFACE_VERSION, outPath, &args);
}

nfdresult_t NFD_OpenDialogN_With_Impl(nfdversion_t version,
                                      nfdnchar_t** outPath,
                                      const nfdopendialognargs_t* args) {
    // We haven't needed to bump the interface version yet.
    (void)version;

    nfdresult_t result = NFD_CANCEL;
    @autoreleasepool {
        NSWindow* keyWindow = GetNativeWindowHandle(&args->parentWindow);
        if (keyWindow) {
            [keyWindow makeKeyAndOrderFront:nil];
        } else {
            keyWindow = [[NSApplication sharedApplication] keyWindow];
        }

        NSOpenPanel* dialog = [NSOpenPanel openPanel];
        [dialog setAllowsMultipleSelection:NO];

        // Build the filter list
        AddFilterListToDialog(dialog, args->filterList, args->filterCount);

        // Set the starting directory
        SetDefaultPath(dialog, args->defaultPath);

        if ([dialog runModal] == NSModalResponseOK) {
            const NSURL* url = [dialog URL];
            const char* utf8Path = [[url path] UTF8String];
            result = CopyUtf8String(utf8Path, outPath);
        }

        // return focus to the key window (i.e. main window)
        [keyWindow makeKeyAndOrderFront:nil];
    }
    return result;
}

nfdresult_t NFD_OpenDialogU8(nfdu8char_t** outPath,
                             const nfdu8filteritem_t* filterList,
                             nfdfiltersize_t filterCount,
                             const nfdu8char_t* defaultPath) {
    return NFD_OpenDialogN(outPath, filterList, filterCount, defaultPath);
}

nfdresult_t NFD_OpenDialogU8_With_Impl(nfdversion_t version,
                                       nfdu8char_t** outPath,
                                       const nfdopendialogu8args_t* args) {
    return NFD_OpenDialogN_With_Impl(version, outPath, args);
}

nfdresult_t NFD_OpenDialogMultipleN(const nfdpathset_t** outPaths,
                                    const nfdnfilteritem_t* filterList,
                                    nfdfiltersize_t filterCount,
                                    const nfdnchar_t* defaultPath) {
    nfdopendialognargs_t args = {0};
    args.filterList = filterList;
    args.filterCount = filterCount;
    args.defaultPath = defaultPath;
    return NFD_OpenDialogMultipleN_With_Impl(NFD_INTERFACE_VERSION, outPaths, &args);
}

nfdresult_t NFD_OpenDialogMultipleN_With_Impl(nfdversion_t version,
                                              const nfdpathset_t** outPaths,
                                              const nfdopendialognargs_t* args) {
    // We haven't needed to bump the interface version yet.
    (void)version;

    nfdresult_t result = NFD_CANCEL;
    @autoreleasepool {
        NSWindow* keyWindow = GetNativeWindowHandle(&args->parentWindow);
        if (keyWindow) {
            [keyWindow makeKeyAndOrderFront:nil];
        } else {
            keyWindow = [[NSApplication sharedApplication] keyWindow];
        }

        NSOpenPanel* dialog = [NSOpenPanel openPanel];
        [dialog setAllowsMultipleSelection:YES];

        // Build the filter list
        AddFilterListToDialog(dialog, args->filterList, args->filterCount);

        // Set the starting directory
        SetDefaultPath(dialog, args->defaultPath);

        if ([dialog runModal] == NSModalResponseOK) {
            const NSArray* urls = [dialog URLs];

            if ([urls count] > 0) {
                // have at least one URL, we return this NSArray
                [urls retain];
                *outPaths = (const nfdpathset_t*)urls;
                result = NFD_OKAY;
            }
        }

        // return focus to the key window (i.e. main window)
        [keyWindow makeKeyAndOrderFront:nil];
    }
    return result;
}

nfdresult_t NFD_OpenDialogMultipleU8(const nfdpathset_t** outPaths,
                                     const nfdu8filteritem_t* filterList,
                                     nfdfiltersize_t filterCount,
                                     const nfdu8char_t* defaultPath) {
    return NFD_OpenDialogMultipleN(outPaths, filterList, filterCount, defaultPath);
}

nfdresult_t NFD_OpenDialogMultipleU8_With_Impl(nfdversion_t version,
                                               const nfdpathset_t** outPaths,
                                               const nfdopendialogu8args_t* args) {
    return NFD_OpenDialogMultipleN_With_Impl(version, outPaths, args);
}

nfdresult_t NFD_SaveDialogN(nfdnchar_t** outPath,
                            const nfdnfilteritem_t* filterList,
                            nfdfiltersize_t filterCount,
                            const nfdnchar_t* defaultPath,
                            const nfdnchar_t* defaultName) {
    nfdsavedialognargs_t args = {0};
    args.filterList = filterList;
    args.filterCount = filterCount;
    args.defaultPath = defaultPath;
    args.defaultName = defaultName;
    return NFD_SaveDialogN_With_Impl(NFD_INTERFACE_VERSION, outPath, &args);
}

nfdresult_t NFD_SaveDialogN_With_Impl(nfdversion_t version,
                                      nfdnchar_t** outPath,
                                      const nfdsavedialognargs_t* args) {
    // We haven't needed to bump the interface version yet.
    (void)version;

    nfdresult_t result = NFD_CANCEL;
    @autoreleasepool {
        NSWindow* keyWindow = GetNativeWindowHandle(&args->parentWindow);
        if (keyWindow) {
            [keyWindow makeKeyAndOrderFront:nil];
        } else {
            keyWindow = [[NSApplication sharedApplication] keyWindow];
        }

        NSSavePanel* dialog = [NSSavePanel savePanel];
        [dialog setExtensionHidden:NO];
        // allow other file types, to give the user an escape hatch since you can't select "*.*" on
        // Mac
        [dialog setAllowsOtherFileTypes:TRUE];

        // Build the filter list
        AddFilterListToDialog(dialog, args->filterList, args->filterCount);

        // Set the starting directory
        SetDefaultPath(dialog, args->defaultPath);

        // Set the default file name
        SetDefaultName(dialog, args->defaultName);

        if ([dialog runModal] == NSModalResponseOK) {
            const NSURL* url = [dialog URL];
            const char* utf8Path = [[url path] UTF8String];
            result = CopyUtf8String(utf8Path, outPath);
        }

        // return focus to the key window (i.e. main window)
        [keyWindow makeKeyAndOrderFront:nil];
    }
    return result;
}

nfdresult_t NFD_SaveDialogU8(nfdu8char_t** outPath,
                             const nfdu8filteritem_t* filterList,
                             nfdfiltersize_t filterCount,
                             const nfdu8char_t* defaultPath,
                             const nfdu8char_t* defaultName) {
    return NFD_SaveDialogN(outPath, filterList, filterCount, defaultPath, defaultName);
}

nfdresult_t NFD_SaveDialogU8_With_Impl(nfdversion_t version,
                                       nfdu8char_t** outPath,
                                       const nfdsavedialogu8args_t* args) {
    return NFD_SaveDialogN_With_Impl(version, outPath, args);
}

nfdresult_t NFD_PickFolderN(nfdnchar_t** outPath, const nfdnchar_t* defaultPath) {
    nfdpickfoldernargs_t args = {0};
    args.defaultPath = defaultPath;
    return NFD_PickFolderN_With_Impl(NFD_INTERFACE_VERSION, outPath, &args);
}

nfdresult_t NFD_PickFolderN_With_Impl(nfdversion_t version,
                                      nfdnchar_t** outPath,
                                      const nfdpickfoldernargs_t* args) {
    // We haven't needed to bump the interface version yet.
    (void)version;

    nfdresult_t result = NFD_CANCEL;
    @autoreleasepool {
        NSWindow* keyWindow = GetNativeWindowHandle(&args->parentWindow);
        if (keyWindow) {
            [keyWindow makeKeyAndOrderFront:nil];
        } else {
            keyWindow = [[NSApplication sharedApplication] keyWindow];
        }

        NSOpenPanel* dialog = [NSOpenPanel openPanel];
        [dialog setAllowsMultipleSelection:NO];
        [dialog setCanChooseDirectories:YES];
        [dialog setCanCreateDirectories:YES];
        [dialog setCanChooseFiles:NO];

        // Set the starting directory
        SetDefaultPath(dialog, args->defaultPath);

        if ([dialog runModal] == NSModalResponseOK) {
            const NSURL* url = [dialog URL];
            const char* utf8Path = [[url path] UTF8String];
            result = CopyUtf8String(utf8Path, outPath);
        }

        // return focus to the key window (i.e. main window)
        [keyWindow makeKeyAndOrderFront:nil];
    }
    return result;
}

nfdresult_t NFD_PickFolderU8(nfdu8char_t** outPath, const nfdu8char_t* defaultPath) {
    return NFD_PickFolderN(outPath, defaultPath);
}

nfdresult_t NFD_PickFolderU8_With_Impl(nfdversion_t version,
                                       nfdu8char_t** outPath,
                                       const nfdpickfolderu8args_t* args) {
    return NFD_PickFolderN_With_Impl(version, outPath, args);
}

nfdresult_t NFD_PickFolderMultipleN(const nfdpathset_t** outPaths, const nfdnchar_t* defaultPath) {
    nfdpickfoldernargs_t args = {0};
    args.defaultPath = defaultPath;
    return NFD_PickFolderMultipleN_With_Impl(NFD_INTERFACE_VERSION, outPaths, &args);
}

nfdresult_t NFD_PickFolderMultipleN_With_Impl(nfdversion_t version,
                                              const nfdpathset_t** outPaths,
                                              const nfdpickfoldernargs_t* args) {
    // We haven't needed to bump the interface version yet.
    (void)version;

    nfdresult_t result = NFD_CANCEL;
    @autoreleasepool {
        NSWindow* keyWindow = GetNativeWindowHandle(&args->parentWindow);
        if (keyWindow) {
            [keyWindow makeKeyAndOrderFront:nil];
        } else {
            keyWindow = [[NSApplication sharedApplication] keyWindow];
        }

        NSOpenPanel* dialog = [NSOpenPanel openPanel];
        [dialog setAllowsMultipleSelection:YES];
        [dialog setCanChooseDirectories:YES];
        [dialog setCanCreateDirectories:YES];
        [dialog setCanChooseFiles:NO];

        // Set the starting directory
        SetDefaultPath(dialog, args->defaultPath);

        if ([dialog runModal] == NSModalResponseOK) {
            const NSArray* urls = [dialog URLs];

            if ([urls count] > 0) {
                // have at least one URL, we return this NSArray
                [urls retain];
                *outPaths = (const nfdpathset_t*)urls;
                result = NFD_OKAY;
            }
        }

        // return focus to the key window (i.e. main window)
        [keyWindow makeKeyAndOrderFront:nil];
    }
    return result;
}

nfdresult_t NFD_PickFolderMultipleU8(const nfdpathset_t** outPaths,
                                     const nfdu8char_t* defaultPath) {
    return NFD_PickFolderMultipleN(outPaths, defaultPath);
}

nfdresult_t NFD_PickFolderMultipleU8_With_Impl(nfdversion_t version,
                                               const nfdpathset_t** outPaths,
                                               const nfdpickfolderu8args_t* args) {
    return NFD_PickFolderMultipleN_With_Impl(version, outPaths, args);
}

nfdresult_t NFD_PathSet_GetCount(const nfdpathset_t* pathSet, nfdpathsetsize_t* count) {
    const NSArray* urls = (const NSArray*)pathSet;
    *count = [urls count];
    return NFD_OKAY;
}

nfdresult_t NFD_PathSet_GetPathN(const nfdpathset_t* pathSet,
                                 nfdpathsetsize_t index,
                                 nfdnchar_t** outPath) {
    const NSArray* urls = (const NSArray*)pathSet;

    @autoreleasepool {
        // autoreleasepool needed because UTF8String method might use the pool
        const NSURL* url = [urls objectAtIndex:index];
        const char* utf8Path = [[url path] UTF8String];
        return CopyUtf8String(utf8Path, outPath);
    }
}

nfdresult_t NFD_PathSet_GetPathU8(const nfdpathset_t* pathSet,
                                  nfdpathsetsize_t index,
                                  nfdu8char_t** outPath) {
    return NFD_PathSet_GetPathN(pathSet, index, outPath);
}

void NFD_PathSet_FreePathN(const nfdnchar_t* filePath) {
    // const_cast not supported on Mac
    union {
        const nfdnchar_t* constPath;
        nfdnchar_t* nonConstPath;
    } pathUnion;

    pathUnion.constPath = filePath;

    NFD_FreePathN(pathUnion.nonConstPath);
}

void NFD_PathSet_FreePathU8(const nfdu8char_t* filePath) {
    // const_cast not supported on Mac
    union {
        const nfdu8char_t* constPath;
        nfdu8char_t* nonConstPath;
    } pathUnion;

    pathUnion.constPath = filePath;

    NFD_FreePathU8(pathUnion.nonConstPath);
}

void NFD_PathSet_Free(const nfdpathset_t* pathSet) {
    const NSArray* urls = (const NSArray*)pathSet;
    [urls release];
}

nfdresult_t NFD_PathSet_GetEnum(const nfdpathset_t* pathSet, nfdpathsetenum_t* outEnumerator) {
    const NSArray* urls = (const NSArray*)pathSet;

    @autoreleasepool {
        // autoreleasepool needed because NSEnumerator uses it
        NSEnumerator* enumerator = [urls objectEnumerator];
        [enumerator retain];
        outEnumerator->ptr = (void*)enumerator;
    }

    return NFD_OKAY;
}

void NFD_PathSet_FreeEnum(nfdpathsetenum_t* enumerator) {
    NSEnumerator* real_enum = (NSEnumerator*)enumerator->ptr;
    [real_enum release];
}

nfdresult_t NFD_PathSet_EnumNextN(nfdpathsetenum_t* enumerator, nfdnchar_t** outPath) {
    NSEnumerator* real_enum = (NSEnumerator*)enumerator->ptr;

    @autoreleasepool {
        // autoreleasepool needed because NSURL uses it
        const NSURL* url = [real_enum nextObject];
        if (url) {
            const char* utf8Path = [[url path] UTF8String];
            return CopyUtf8String(utf8Path, outPath);
        } else {
            *outPath = NULL;
            return NFD_OKAY;
        }
    }
}

nfdresult_t NFD_PathSet_EnumNextU8(nfdpathsetenum_t* enumerator, nfdu8char_t** outPath) {
    return NFD_PathSet_EnumNextN(enumerator, outPath);
}
