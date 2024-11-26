/*
 *  Copyright 2019-2024 Diligent Graphics LLC
 *  Copyright 2015-2019 Egor Yusov
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  In no event and under no legal theory, whether in tort (including negligence),
 *  contract, or otherwise, unless required by applicable law (such as deliberate
 *  and grossly negligent acts) or agreed to in writing, shall any Contributor be
 *  liable for any damages, including any direct, indirect, special, incidental,
 *  or consequential damages of any character arising as a result of this License or
 *  out of the use or inability to use the software (including but not limited to damages
 *  for loss of goodwill, work stoppage, computer failure or malfunction, or any and
 *  all other commercial damages or losses), even if such Contributor has been advised
 *  of the possibility of such damages.
 */

#include "pch.h"
#include "DefaultRawMemoryAllocator.hpp"
#include "Align.hpp"

#include <stdlib.h>

#if defined(_DEBUG) && defined(_MSC_VER)
#    include <crtdbg.h>
#    define USE_CRT_MALLOC_DBG 1
#endif

#if PLATFORM_ANDROID && __ANDROID_API__ < 28
#    define USE_ALIGNED_MALLOC_FALLBACK 1
#endif

namespace Diligent
{

#ifdef USE_ALIGNED_MALLOC_FALLBACK
namespace
{
void* AllocateAlignedFallback(size_t Size, size_t Alignment)
{
    constexpr size_t PointerSize       = sizeof(void*);
    const size_t     AdjustedAlignment = (std::max)(Alignment, PointerSize);

    void* Pointer        = malloc(Size + AdjustedAlignment + PointerSize);
    void* AlignedPointer = AlignUp(reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(Pointer) + PointerSize), AdjustedAlignment);

    void** StoredPointer = reinterpret_cast<void**>(AlignedPointer) - 1;
    VERIFY_EXPR(StoredPointer >= Pointer);
    *StoredPointer = Pointer;

    return AlignedPointer;
}

void FreeAlignedFallback(void* Ptr)
{
    if (Ptr != nullptr)
    {
        void* OriginalPointer = *(reinterpret_cast<void**>(Ptr) - 1);
        free(OriginalPointer);
    }
}
} // namespace
#endif

DefaultRawMemoryAllocator::DefaultRawMemoryAllocator()
{
}

void* DefaultRawMemoryAllocator::Allocate(size_t Size, const Char* dbgDescription, const char* dbgFileName, const Int32 dbgLineNumber)
{
    VERIFY_EXPR(Size > 0);
#ifdef USE_CRT_MALLOC_DBG
    return _malloc_dbg(Size, _NORMAL_BLOCK, dbgFileName, dbgLineNumber);
#else
    return malloc(Size);
#endif
}

void DefaultRawMemoryAllocator::Free(void* Ptr)
{
    free(Ptr);
}

#ifdef ALIGNED_MALLOC
#    undef ALIGNED_MALLOC
#endif
#ifdef ALIGNED_FREE
#    undef ALIGNED_FREE
#endif

#ifdef USE_CRT_MALLOC_DBG
#    define ALIGNED_MALLOC(Size, Alignment, dbgFileName, dbgLineNumber) _aligned_malloc_dbg(Size, Alignment, dbgFileName, dbgLineNumber)
#    define ALIGNED_FREE(Ptr)                                           _aligned_free(Ptr)
#elif defined(_MSC_VER) || defined(__MINGW64__) || defined(__MINGW32__)
#    define ALIGNED_MALLOC(Size, Alignment, dbgFileName, dbgLineNumber) _aligned_malloc(Size, Alignment)
#    define ALIGNED_FREE(Ptr)                                           _aligned_free(Ptr)
#elif defined(USE_ALIGNED_MALLOC_FALLBACK)
#    define ALIGNED_MALLOC(Size, Alignment, dbgFileName, dbgLineNumber) AllocateAlignedFallback(Size, Alignment)
#    define ALIGNED_FREE(Ptr)                                           FreeAlignedFallback(Ptr)
#else
#    define ALIGNED_MALLOC(Size, Alignment, dbgFileName, dbgLineNumber) aligned_alloc(Alignment, Size)
#    define ALIGNED_FREE(Ptr)                                           free(Ptr)
#endif

void* DefaultRawMemoryAllocator::AllocateAligned(size_t Size, size_t Alignment, const Char* dbgDescription, const char* dbgFileName, const Int32 dbgLineNumber)
{
    VERIFY_EXPR(Size > 0 && Alignment > 0);
    Size = AlignUp(Size, Alignment);
    return ALIGNED_MALLOC(Size, Alignment, dbgFileName, dbgLineNumber);
}

void DefaultRawMemoryAllocator::FreeAligned(void* Ptr)
{
    ALIGNED_FREE(Ptr);
}

DefaultRawMemoryAllocator& DefaultRawMemoryAllocator::GetAllocator()
{
    static DefaultRawMemoryAllocator Allocator;
    return Allocator;
}

} // namespace Diligent
