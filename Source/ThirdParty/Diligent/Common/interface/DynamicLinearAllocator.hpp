/*
 *  Copyright 2019-2022 Diligent Graphics LLC
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

#pragma once

/// \file
/// Defines Diligent::DynamicLinearAllocator class

#include <vector>
#include <cstring>

#include "../../Primitives/interface/BasicTypes.h"
#include "../../Primitives/interface/MemoryAllocator.h"
#include "../../Platforms/Basic/interface/DebugUtilities.hpp"
#include "CompilerDefinitions.h"
#include "Align.hpp"

namespace Diligent
{

/// Implementation of a linear allocator on fixed memory pages
class DynamicLinearAllocator
{
public:
    // clang-format off
    DynamicLinearAllocator           (const DynamicLinearAllocator&) = delete;
    DynamicLinearAllocator           (DynamicLinearAllocator&&)      = delete;
    DynamicLinearAllocator& operator=(const DynamicLinearAllocator&) = delete;
    DynamicLinearAllocator& operator=(DynamicLinearAllocator&&)      = delete;
    // clang-format on

    explicit DynamicLinearAllocator(IMemoryAllocator& Allocator, Uint32 BlockSize = 4 << 10) :
        m_BlockSize{BlockSize},
        m_pAllocator{&Allocator}
    {
        VERIFY(IsPowerOfTwo(BlockSize), "Block size (", BlockSize, ") is not power of two");
    }

    ~DynamicLinearAllocator()
    {
        Free();
    }

    void Free()
    {
        for (auto& block : m_Blocks)
        {
            m_pAllocator->Free(block.Data);
        }
        m_Blocks.clear();

        m_pAllocator = nullptr;
    }

    void Discard()
    {
        for (auto& block : m_Blocks)
        {
            block.CurrPtr = block.Data;
        }
    }

    NODISCARD void* Allocate(size_t size, size_t align)
    {
        if (size == 0)
            return nullptr;

        for (auto& block : m_Blocks)
        {
            auto* Ptr = AlignUp(block.CurrPtr, align);
            if (Ptr + size <= block.Data + block.Size)
            {
                block.CurrPtr = Ptr + size;
                return Ptr;
            }
        }

        // Create a new block
        size_t BlockSize = m_BlockSize;
        while (BlockSize < size + align - 1)
            BlockSize *= 2;
        m_Blocks.emplace_back(m_pAllocator->Allocate(BlockSize, "dynamic linear allocator page", __FILE__, __LINE__), BlockSize);

        auto& block = m_Blocks.back();
        auto* Ptr   = AlignUp(block.Data, align);
        VERIFY(Ptr + size <= block.Data + block.Size, "Not enough space in the new block - this is a bug");
        block.CurrPtr = Ptr + size;
        return Ptr;
    }

    template <typename T>
    NODISCARD T* Allocate(size_t count = 1)
    {
        return reinterpret_cast<T*>(Allocate(sizeof(T) * count, alignof(T)));
    }

    template <typename T, typename... Args>
    NODISCARD T* Construct(Args&&... args)
    {
        T* Ptr = Allocate<T>(1);
        new (Ptr) T{std::forward<Args>(args)...};
        return Ptr;
    }

    template <typename T, typename... Args>
    NODISCARD T* ConstructArray(size_t count, const Args&... args)
    {
        T* Ptr = Allocate<T>(count);
        for (size_t i = 0; i < count; ++i)
        {
            new (Ptr + i) T{args...};
        }
        return Ptr;
    }

    template <typename T>
    NODISCARD T* CopyArray(const T* Src, size_t count)
    {
        T* Dst = Allocate<T>(count);
        for (size_t i = 0; i < count; ++i)
        {
            new (Dst + i) T{Src[i]};
        }
        return Dst;
    }

    NODISCARD Char* CopyString(const Char* Str, size_t len = 0)
    {
        if (Str == nullptr)
            return nullptr;

        if (len == 0)
            len = strlen(Str);
        else
            VERIFY_EXPR(len <= strlen(Str));

        Char* Dst = Allocate<Char>(len + 1);
        std::memcpy(Dst, Str, sizeof(Char) * len);
        Dst[len] = 0;
        return Dst;
    }

    NODISCARD wchar_t* CopyWString(const char* Str, size_t len = 0)
    {
        if (Str == nullptr)
            return nullptr;

        if (len == 0)
            len = strlen(Str);
        else
            VERIFY_EXPR(len <= strlen(Str));

        auto* Dst = Allocate<wchar_t>(len + 1);
        for (size_t i = 0; i < len; ++i)
        {
            Dst[i] = static_cast<wchar_t>(Str[i]);
        }
        Dst[len] = 0;
        return Dst;
    }

    NODISCARD Char* CopyString(const String& Str)
    {
        return CopyString(Str.c_str(), Str.length());
    }

    NODISCARD wchar_t* CopyWString(const String& Str)
    {
        return CopyWString(Str.c_str(), Str.length());
    }

    size_t GetBlockCount() const
    {
        return m_Blocks.size();
    }

    template <typename HandlerType>
    void ProcessBlocks(HandlerType&& Handler) const
    {
        for (const auto& Block : m_Blocks)
            Handler(Block.Data, Block.Size);
    }

private:
    struct Block
    {
        uint8_t* const Data    = nullptr;
        size_t const   Size    = 0;
        uint8_t*       CurrPtr = nullptr;

        Block(void* _Data, size_t _Size) :
            Data{static_cast<uint8_t*>(_Data)}, Size{_Size}, CurrPtr{Data} {}
    };

    std::vector<Block> m_Blocks;
    const Uint32       m_BlockSize  = 4 << 10;
    IMemoryAllocator*  m_pAllocator = nullptr;
};

} // namespace Diligent
