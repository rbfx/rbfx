#include "ik/backtrace.h"
#if !__ANDROID__
#include <execinfo.h>
#endif
/* ------------------------------------------------------------------------- */
char**
get_backtrace(int* size)
{
    void* array[BACKTRACE_SIZE];
    char** strings;
#if __ANDROID__
    *size = 0;
    strings = 0;
#else
    *size = backtrace(array, BACKTRACE_SIZE);
    strings = backtrace_symbols(array, *size);
#endif
    return strings;
}
