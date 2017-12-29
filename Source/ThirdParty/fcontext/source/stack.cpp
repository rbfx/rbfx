#include <assert.h>
#include <math.h>
#include <string.h>

#include "fcontext.h"

// Detect posix
#if !defined(_WIN32) && (defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__)))
/* UNIX-style OS. ------------------------------------------- */
#   include <unistd.h>
#   define _HAVE_POSIX 1
#endif

#ifdef _WIN32
#   define WIN32_LEAN_AND_LEAN
#   include <Windows.h>
/* x86_64
 * test x86_64 before i386 because icc might
 * define __i686__ for x86_64 too */
#if defined(__x86_64__) || defined(__x86_64) \
    || defined(__amd64__) || defined(__amd64) \
    || defined(_M_X64) || defined(_M_AMD64)
/* Windows seams not to provide a constant or function
 * telling the minimal stacksize */
#   define MIN_STACKSIZE  8 * 1024
#else
#   define MIN_STACKSIZE  4 * 1024
#endif

static size_t getPageSize()
{
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return (size_t)si.dwPageSize;
}

static size_t getMinSize()
{
    return MIN_STACKSIZE;
}

static size_t getMaxSize()
{
    return  1 * 1024 * 1024 * 1024; /* 1GB */
}

static size_t getDefaultSize()
{
    return 64 * 1024;   /* 64Kb */
}

#elif defined(_HAVE_POSIX)
#include <signal.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#if !defined (SIGSTKSZ)
# define SIGSTKSZ (8 * 1024)
# define UDEF_SIGSTKSZ
#endif

static size_t getPageSize()
{
    /* conform to POSIX.1-2001 */
    return (size_t)sysconf(_SC_PAGESIZE);
}

static size_t getMinSize()
{
    return SIGSTKSZ;
}

static size_t getMaxSize()
{
    rlimit limit;
    getrlimit(RLIMIT_STACK, &limit);

    return (size_t)limit.rlim_max;
}

static size_t getDefaultSize()
{
    size_t size;
    size_t maxSize;
    rlimit limit;

    getrlimit(RLIMIT_STACK, &limit);

    size = 8 * getMinSize();
    if (RLIM_INFINITY == limit.rlim_max)
        return size;
    maxSize = getMaxSize();
    return maxSize < size ? maxSize : size;
}
#endif

/* Stack allocation and protection*/
fcontext_stack_t create_fcontext_stack(size_t size)
{
    size_t pages;
    size_t size_;
    void* vp;
    fcontext_stack_t s;
    s.sptr = NULL;
    s.ssize = 0;

    if (size == 0)
        size = getDefaultSize();
    if (size <= getMinSize())
        size = getMinSize();
    assert(size <= getMaxSize());

    pages = (size_t)floorf(float(size) / float(getPageSize()));
    assert(pages >= 2);     /* at least two pages must fit into stack (one page is guard-page) */

    size_ = pages * getPageSize();
    assert(size_ != 0 && size != 0);
    assert(size_ <= size);

#ifdef _WIN32
    vp = VirtualAlloc(0, size_, MEM_COMMIT, PAGE_READWRITE);
    if (!vp)
        return s;

    DWORD old_options;
    VirtualProtect(vp, getPageSize(), PAGE_READWRITE | PAGE_GUARD, &old_options);
#elif defined(_HAVE_POSIX)
# if defined(MAP_ANON)
    vp = mmap(0, size_, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
# else
    vp = mmap(0, size_, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
# endif
    if (vp == MAP_FAILED)
        return s;
    mprotect(vp, getPageSize(), PROT_NONE);
#else
    vp = malloc(size_);
    if (!vp)
        return s;
#endif

    s.sptr = (char*)vp + size_;
    s.ssize = size_;
    return s;
}

void destroy_fcontext_stack(fcontext_stack_t* s)
{
    void* vp;

    assert(s->ssize >= getMinSize());
    assert(s->ssize <= getMaxSize());

    vp = (char*)s->sptr - s->ssize;

#ifdef _WIN32
    VirtualFree(vp, 0, MEM_RELEASE);
#elif defined(_HAVE_POSIX)
    munmap(vp, s->ssize);
#else
    free(vp);
#endif

    memset(s, 0x00, sizeof(fcontext_stack_t));
}
