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

#include <functional>
#include <memory>
#include <cstring>
#include <algorithm>

#include "../../Primitives/interface/Errors.hpp"
#include "../../Platforms/Basic/interface/DebugUtilities.hpp"
#include "../../Graphics/GraphicsEngine/interface/Sampler.h"
#include "../../Graphics/GraphicsEngine/interface/RasterizerState.h"
#include "../../Graphics/GraphicsEngine/interface/DepthStencilState.h"
#include "../../Graphics/GraphicsEngine/interface/BlendState.h"
#include "../../Graphics/GraphicsEngine/interface/TextureView.h"
#include "../../Graphics/GraphicsEngine/interface/PipelineResourceSignature.h"
#include "../../Graphics/GraphicsEngine/interface/PipelineState.h"
#include "../../Graphics/GraphicsTools/interface/VertexPool.h"
#include "../../Common/interface/RefCntAutoPtr.hpp"
#include "Align.hpp"

#define LOG_HASH_CONFLICTS 1

namespace Diligent
{

#if (defined(__clang__) || defined(__GNUC__))

// GCC's and Clang's implementation of std::hash for integral types is IDENTITY,
// which is an unbelievably poor design choice.

// https://github.com/facebook/folly/blob/main/folly/hash/Hash.h

// Robert Jenkins' reversible 32 bit mix hash function.
constexpr uint32_t jenkins_rev_mix32(uint32_t key) noexcept
{
    key += (key << 12); // key *= (1 + (1 << 12))
    key ^= (key >> 22);
    key += (key << 4); // key *= (1 + (1 << 4))
    key ^= (key >> 9);
    key += (key << 10); // key *= (1 + (1 << 10))
    key ^= (key >> 2);
    // key *= (1 + (1 << 7)) * (1 + (1 << 12))
    key += (key << 7);
    key += (key << 12);
    return key;
}

// Thomas Wang 64 bit mix hash function.
constexpr uint64_t twang_mix64(uint64_t key) noexcept
{
    key = (~key) + (key << 21); // key *= (1 << 21) - 1; key -= 1;
    key = key ^ (key >> 24);
    key = key + (key << 3) + (key << 8); // key *= 1 + (1 << 3) + (1 << 8)
    key = key ^ (key >> 14);
    key = key + (key << 2) + (key << 4); // key *= 1 + (1 << 2) + (1 << 4)
    key = key ^ (key >> 28);
    key = key + (key << 31); // key *= 1 + (1 << 31)
    return key;
}

template <typename T>
typename std::enable_if<(std::is_fundamental<T>::value || std::is_enum<T>::value) && sizeof(T) == 8, size_t>::type ComputeHash(const T& Val) noexcept
{
    uint64_t Val64 = 0;
    std::memcpy(&Val64, &Val, sizeof(Val));
    return twang_mix64(Val64);
}

template <typename T>
typename std::enable_if<(std::is_fundamental<T>::value || std::is_enum<T>::value) && sizeof(T) <= 4, size_t>::type ComputeHash(const T& Val) noexcept
{
    uint32_t Val32 = 0;
    std::memcpy(&Val32, &Val, sizeof(Val));
    return jenkins_rev_mix32(Val32);
}

template <typename T>
typename std::enable_if<!(std::is_fundamental<T>::value || std::is_enum<T>::value), size_t>::type ComputeHash(const T& Val) noexcept
{
    return std::hash<T>{}(Val);
}

#else

template <typename T>
size_t ComputeHash(const T& Val) noexcept
{
    return std::hash<T>{}(Val);
}

#endif

// http://www.boost.org/doc/libs/1_35_0/doc/html/hash/combine.html
template <typename T>
void HashCombine(std::size_t& Seed, const T& Val) noexcept
{
    Seed ^= ComputeHash(Val) + 0x9e3779b9 + (Seed << 6) + (Seed >> 2);
}

template <typename FirstArgType, typename... RestArgsType>
void HashCombine(std::size_t& Seed, const FirstArgType& FirstArg, const RestArgsType&... RestArgs) noexcept
{
    HashCombine(Seed, FirstArg);
    HashCombine(Seed, RestArgs...); // recursive call using pack expansion syntax
}

template <typename FirstArgType, typename... RestArgsType>
std::size_t ComputeHash(const FirstArgType& FirstArg, const RestArgsType&... RestArgs) noexcept
{
    std::size_t Seed = 0;
    HashCombine(Seed, FirstArg, RestArgs...);
    return Seed;
}

inline std::size_t ComputeHashRaw(const void* pData, size_t Size) noexcept
{
    size_t Hash = 0;

    const auto* BytePtr  = static_cast<const Uint8*>(pData);
    const auto* EndPtr   = BytePtr + Size;
    const auto* DwordPtr = static_cast<const Uint32*>(AlignUp(pData, alignof(Uint32)));

    // Process initial bytes before we get to the 32-bit aligned pointer
    Uint64 Buffer = 0;
    Uint64 Shift  = 0;
    while (BytePtr < EndPtr && BytePtr < reinterpret_cast<const Uint8*>(DwordPtr))
    {
        Buffer |= Uint64{*(BytePtr++)} << Shift;
        Shift += 8;
    }
    VERIFY_EXPR(Shift <= 24);

    // Process dwords
    while (DwordPtr + 1 <= reinterpret_cast<const Uint32*>(EndPtr))
    {
        Buffer |= Uint64{*(DwordPtr++)} << Shift;
        HashCombine(Hash, static_cast<Uint32>(Buffer & ~Uint32{0}));
        Buffer = Buffer >> Uint64{32};
    }

    // Process the remaining bytes
    BytePtr = reinterpret_cast<const Uint8*>(DwordPtr);
    while (BytePtr < EndPtr)
    {
        Buffer |= Uint64{*(BytePtr++)} << Shift;
        Shift += 8;
    }
    VERIFY_EXPR(Shift <= (3 + 3) * 8);

    while (Shift != 0)
    {
        HashCombine(Hash, static_cast<Uint32>(Buffer & ~Uint32{0}));
        Buffer = Buffer >> Uint64{32};
        Shift -= (std::min)(Shift, Uint64{32});
    }

    return Hash;
}

template <typename CharType>
struct CStringHash
{
    size_t operator()(const CharType* str) const noexcept
    {
        if (str == nullptr)
            return 0;

        // http://www.cse.yorku.ca/~oz/hash.html
        std::size_t Seed = 0;
        while (std::size_t Ch = *(str++))
            Seed = Seed * 65599 + Ch;
        return Seed;
    }
};

template <typename CharType>
struct CStringCompare
{
    bool operator()(const CharType* str1, const CharType* str2) const
    {
        UNSUPPORTED("Template specialization is not implemented");
        return false;
    }
};

template <>
struct CStringCompare<Char>
{
    bool operator()(const Char* str1, const Char* str2) const noexcept
    {
        return strcmp(str1, str2) == 0;
    }
};

/// This helper structure is intended to facilitate using strings as a
/// hash table key. It provides constructors that can make a copy of the
/// source string or just keep a pointer to it, which enables searching in
/// the hash using raw const Char* pointers.
struct HashMapStringKey
{
public:
    HashMapStringKey() noexcept {}

    // This constructor can perform implicit const Char* -> HashMapStringKey
    // conversion without copying the string.
    HashMapStringKey(const Char* _Str, bool bMakeCopy = false) :
        Str{_Str}
    {
        VERIFY(Str, "String pointer must not be null");

        Ownership_Hash = CStringHash<Char>{}.operator()(Str) & HashMask;
        if (bMakeCopy)
        {
            auto  LenWithZeroTerm = strlen(Str) + 1;
            auto* StrCopy         = new char[LenWithZeroTerm];
            std::memcpy(StrCopy, Str, LenWithZeroTerm);
            Str = StrCopy;
            Ownership_Hash |= StrOwnershipMask;
        }
    }

    // Make this constructor explicit to avoid unintentional string copies
    explicit HashMapStringKey(const String& Str, bool bMakeCopy = true) :
        HashMapStringKey{Str.c_str(), bMakeCopy}
    {
    }

    HashMapStringKey(HashMapStringKey&& Key) noexcept :
        // clang-format off
        Str {Key.Str},
        Ownership_Hash{Key.Ownership_Hash}
    // clang-format on
    {
        Key.Str            = nullptr;
        Key.Ownership_Hash = 0;
    }

    HashMapStringKey& operator=(HashMapStringKey&& rhs) noexcept
    {
        if (this == &rhs)
            return *this;

        Clear();

        Str            = rhs.Str;
        Ownership_Hash = rhs.Ownership_Hash;

        rhs.Str            = nullptr;
        rhs.Ownership_Hash = 0;

        return *this;
    }

    ~HashMapStringKey()
    {
        Clear();
    }

    // Disable copy constructor and assignments. The struct is designed
    // to be initialized at creation time only.
    // clang-format off
    HashMapStringKey           (const HashMapStringKey&) = delete;
    HashMapStringKey& operator=(const HashMapStringKey&) = delete;
    // clang-format on

    HashMapStringKey Clone() const
    {
        return HashMapStringKey{GetStr(), (Ownership_Hash & StrOwnershipMask) != 0};
    }

    bool operator==(const HashMapStringKey& RHS) const noexcept
    {
        if (Str == RHS.Str)
            return true;

        if (Str == nullptr)
        {
            VERIFY_EXPR(RHS.Str != nullptr);
            return false;
        }
        else if (RHS.Str == nullptr)
        {
            VERIFY_EXPR(Str != nullptr);
            return false;
        }

        auto Hash    = GetHash();
        auto RHSHash = RHS.GetHash();
        if (Hash != RHSHash)
        {
            VERIFY_EXPR(strcmp(Str, RHS.Str) != 0);
            return false;
        }

        bool IsEqual = strcmp(Str, RHS.Str) == 0;

#if LOG_HASH_CONFLICTS
        if (!IsEqual && Hash == RHSHash)
        {
            LOG_WARNING_MESSAGE("Unequal strings \"", Str, "\" and \"", RHS.Str,
                                "\" have the same hash. You may want to use a better hash function. "
                                "You may disable this warning by defining LOG_HASH_CONFLICTS to 0");
        }
#endif
        return IsEqual;
    }

    bool operator!=(const HashMapStringKey& RHS) const noexcept
    {
        return !(*this == RHS);
    }

    explicit operator bool() const noexcept
    {
        return GetStr() != nullptr;
    }

    size_t GetHash() const noexcept
    {
        return Ownership_Hash & HashMask;
    }

    const Char* GetStr() const noexcept
    {
        return Str;
    }

    struct Hasher
    {
        size_t operator()(const HashMapStringKey& Key) const noexcept
        {
            return Key.GetHash();
        }
    };

    void Clear()
    {
        if (Str != nullptr && (Ownership_Hash & StrOwnershipMask) != 0)
            delete[] Str;

        Str            = nullptr;
        Ownership_Hash = 0;
    }

protected:
    static constexpr size_t StrOwnershipBit  = sizeof(size_t) * 8 - 1;
    static constexpr size_t StrOwnershipMask = size_t{1} << StrOwnershipBit;
    static constexpr size_t HashMask         = ~StrOwnershipMask;

    const Char* Str = nullptr;
    // We will use top bit of the hash to indicate if we own the pointer
    size_t Ownership_Hash = 0;
};


template <typename HasherType>
struct HashCombinerBase
{
    HashCombinerBase(HasherType& Hasher) :
        m_Hasher{Hasher}
    {}

protected:
    HasherType& m_Hasher;
};

template <typename HasherType, typename Type>
struct HashCombiner;

template <typename HasherType>
struct HashCombiner<HasherType, SamplerDesc> : HashCombinerBase<HasherType>
{
    HashCombiner(HasherType& Hasher) :
        HashCombinerBase<HasherType>{Hasher}
    {}

    void operator()(const SamplerDesc& SamDesc) const
    {
        ASSERT_SIZEOF(SamDesc.MinFilter, 1, "Hash logic below may be incorrect.");
        ASSERT_SIZEOF(SamDesc.MagFilter, 1, "Hash logic below may be incorrect.");
        ASSERT_SIZEOF(SamDesc.MipFilter, 1, "Hash logic below may be incorrect.");
        ASSERT_SIZEOF(SamDesc.AddressU, 1, "Hash logic below may be incorrect.");
        ASSERT_SIZEOF(SamDesc.AddressV, 1, "Hash logic below may be incorrect.");
        ASSERT_SIZEOF(SamDesc.AddressW, 1, "Hash logic below may be incorrect.");
        ASSERT_SIZEOF(SamDesc.Flags, 1, "Hash logic below may be incorrect.");
        ASSERT_SIZEOF(SamDesc.BorderColor, 16, "Hash logic below may be incorrect.");

        // Ignore Name. This is consistent with the operator==
        this->m_Hasher( // SamDesc.Name,
            ((static_cast<uint32_t>(SamDesc.MinFilter) << 0u) |
             (static_cast<uint32_t>(SamDesc.MagFilter) << 8u) |
             (static_cast<uint32_t>(SamDesc.MipFilter) << 24u)),
            ((static_cast<uint32_t>(SamDesc.AddressU) << 0u) |
             (static_cast<uint32_t>(SamDesc.AddressV) << 8u) |
             (static_cast<uint32_t>(SamDesc.AddressW) << 24u)),
            ((static_cast<uint32_t>(SamDesc.Flags) << 0u) |
             ((SamDesc.UnnormalizedCoords ? 1u : 0u) << 8u)),
            SamDesc.MipLODBias,
            SamDesc.MaxAnisotropy,
            static_cast<uint32_t>(SamDesc.ComparisonFunc),
            SamDesc.BorderColor[0],
            SamDesc.BorderColor[1],
            SamDesc.BorderColor[2],
            SamDesc.BorderColor[3],
            SamDesc.MinLOD,
            SamDesc.MaxLOD);
        ASSERT_SIZEOF64(SamDesc, 56, "Did you add new members to SamplerDesc? Please handle them here.");
    }
};


template <typename HasherType>
struct HashCombiner<HasherType, StencilOpDesc> : HashCombinerBase<HasherType>
{
    HashCombiner(HasherType& Hasher) :
        HashCombinerBase<HasherType>{Hasher}
    {}

    void operator()(const StencilOpDesc& StOpDesc) const
    {
        // clang-format off
        ASSERT_SIZEOF(StOpDesc.StencilFailOp,      1, "Hash logic below may be incorrect.");
        ASSERT_SIZEOF(StOpDesc.StencilDepthFailOp, 1, "Hash logic below may be incorrect.");
        ASSERT_SIZEOF(StOpDesc.StencilPassOp,      1, "Hash logic below may be incorrect.");
        ASSERT_SIZEOF(StOpDesc.StencilFunc,        1, "Hash logic below may be incorrect.");

        this->m_Hasher(
            ((static_cast<uint32_t>(StOpDesc.StencilFailOp)      <<  0u) |
             (static_cast<uint32_t>(StOpDesc.StencilDepthFailOp) <<  8u) |
             (static_cast<uint32_t>(StOpDesc.StencilPassOp)      << 16u) |
             (static_cast<uint32_t>(StOpDesc.StencilFunc)        << 24u)));
        // clang-format on
        ASSERT_SIZEOF(StOpDesc, 4, "Did you add new members to StencilOpDesc? Please handle them here.");
    }
};


template <typename HasherType>
struct HashCombiner<HasherType, DepthStencilStateDesc> : HashCombinerBase<HasherType>
{
    HashCombiner(HasherType& Hasher) :
        HashCombinerBase<HasherType>{Hasher}
    {}

    void operator()(const DepthStencilStateDesc& DSSDesc) const
    {
        ASSERT_SIZEOF(DSSDesc.DepthFunc, 1, "Hash logic below may be incorrect.");
        ASSERT_SIZEOF(DSSDesc.StencilReadMask, 1, "Hash logic below may be incorrect.");
        ASSERT_SIZEOF(DSSDesc.StencilWriteMask, 1, "Hash logic below may be incorrect.");
        // clang-format off
        this->m_Hasher(
            (((DSSDesc.DepthEnable      ? 1u : 0u) << 0u) |
             ((DSSDesc.DepthWriteEnable ? 1u : 0u) << 1u) |
             ((DSSDesc.StencilEnable    ? 1u : 0u) << 2u) |
             (static_cast<uint32_t>(DSSDesc.DepthFunc)        << 8u)  |
             (static_cast<uint32_t>(DSSDesc.StencilReadMask)  << 16u) |
             (static_cast<uint32_t>(DSSDesc.StencilWriteMask) << 24u)),
            DSSDesc.FrontFace,
            DSSDesc.BackFace);
        // clang-format on
        ASSERT_SIZEOF(DSSDesc, 14, "Did you add new members to DepthStencilStateDesc? Please handle them here.");
    }
};


template <typename HasherType>
struct HashCombiner<HasherType, RasterizerStateDesc> : HashCombinerBase<HasherType>
{
    HashCombiner(HasherType& Hasher) :
        HashCombinerBase<HasherType>{Hasher}
    {}

    void operator()(const RasterizerStateDesc& RSDesc) const
    {
        ASSERT_SIZEOF(RSDesc.FillMode, 1, "Hash logic below may be incorrect.");
        ASSERT_SIZEOF(RSDesc.CullMode, 1, "Hash logic below may be incorrect.");

        // clang-format off
        this->m_Hasher(
            ((static_cast<uint32_t>(RSDesc.FillMode) << 0u) |
             (static_cast<uint32_t>(RSDesc.CullMode) << 8u) |
             ((RSDesc.FrontCounterClockwise ? 1u : 0u) << 16u) |
             ((RSDesc.DepthClipEnable       ? 1u : 0u) << 17u) |
             ((RSDesc.ScissorEnable         ? 1u : 0u) << 18u) |
             ((RSDesc.AntialiasedLineEnable ? 1u : 0u) << 19u)),
            RSDesc.DepthBias,
            RSDesc.DepthBiasClamp,
            RSDesc.SlopeScaledDepthBias);
        // clang-format on
        ASSERT_SIZEOF(RSDesc, 20, "Did you add new members to RasterizerStateDesc? Please handle them here.");
    }
};


template <typename HasherType>
struct HashCombiner<HasherType, BlendStateDesc> : HashCombinerBase<HasherType>
{
    HashCombiner(HasherType& Hasher) :
        HashCombinerBase<HasherType>{Hasher}
    {}

    void operator()(const BlendStateDesc& BSDesc) const
    {
        for (size_t i = 0; i < MAX_RENDER_TARGETS; ++i)
        {
            const auto& rt = BSDesc.RenderTargets[i];

            ASSERT_SIZEOF(rt.SrcBlend, 1, "Hash logic below may be incorrect.");
            ASSERT_SIZEOF(rt.DestBlend, 1, "Hash logic below may be incorrect.");
            ASSERT_SIZEOF(rt.BlendOp, 1, "Hash logic below may be incorrect.");
            ASSERT_SIZEOF(rt.SrcBlendAlpha, 1, "Hash logic below may be incorrect.");
            ASSERT_SIZEOF(rt.DestBlendAlpha, 1, "Hash logic below may be incorrect.");
            ASSERT_SIZEOF(rt.BlendOpAlpha, 1, "Hash logic below may be incorrect.");
            ASSERT_SIZEOF(rt.LogicOp, 1, "Hash logic below may be incorrect.");
            ASSERT_SIZEOF(rt.RenderTargetWriteMask, 1, "Hash logic below may be incorrect.");

            // clang-format off
            this->m_Hasher(
                (((rt.BlendEnable          ? 1u : 0u) <<  0u) |
                 ((rt.LogicOperationEnable ? 1u : 0u) <<  1u) |
                 (static_cast<uint32_t>(rt.SrcBlend)  <<  8u) |
                 (static_cast<uint32_t>(rt.DestBlend) << 16u) |
                 (static_cast<uint32_t>(rt.BlendOp)   << 24u)),
                ((static_cast<uint32_t>(rt.SrcBlendAlpha)  <<  0u) |
                 (static_cast<uint32_t>(rt.DestBlendAlpha) <<  8u) |
                 (static_cast<uint32_t>(rt.BlendOpAlpha)   << 16u) |
                 (static_cast<uint32_t>(rt.LogicOp)        << 24u)),
                rt.RenderTargetWriteMask);
            // clang-format on
        }
        this->m_Hasher(
            (((BSDesc.AlphaToCoverageEnable ? 1u : 0u) << 0u) |
             ((BSDesc.IndependentBlendEnable ? 1u : 0u) << 1u)));

        ASSERT_SIZEOF(BSDesc, 82, "Did you add new members to BlendStateDesc? Please handle them here.");
    }
};


template <typename HasherType>
struct HashCombiner<HasherType, TextureViewDesc> : HashCombinerBase<HasherType>
{
    HashCombiner(HasherType& Hasher) :
        HashCombinerBase<HasherType>{Hasher}
    {}

    void operator()(const TextureViewDesc& TexViewDesc) const
    {
        ASSERT_SIZEOF(TexViewDesc.ViewType, 1, "Hash logic below may be incorrect.");
        ASSERT_SIZEOF(TexViewDesc.TextureDim, 1, "Hash logic below may be incorrect.");
        ASSERT_SIZEOF(TexViewDesc.Format, 2, "Hash logic below may be incorrect.");
        ASSERT_SIZEOF(TexViewDesc.AccessFlags, 1, "Hash logic below may be incorrect.");
        ASSERT_SIZEOF(TexViewDesc.Flags, 1, "Hash logic below may be incorrect.");
        ASSERT_SIZEOF(TexViewDesc.Swizzle, 4, "Hash logic below may be incorrect.");

        // Ignore Name. This is consistent with the operator==
        this->m_Hasher(
            ((static_cast<uint32_t>(TexViewDesc.ViewType) << 0u) |
             (static_cast<uint32_t>(TexViewDesc.TextureDim) << 8u) |
             (static_cast<uint32_t>(TexViewDesc.Format) << 16u)),
            TexViewDesc.MostDetailedMip,
            TexViewDesc.NumMipLevels,
            TexViewDesc.FirstArraySlice,
            TexViewDesc.NumArraySlices,
            ((static_cast<uint32_t>(TexViewDesc.AccessFlags) << 0u) |
             (static_cast<uint32_t>(TexViewDesc.Flags) << 8u)),
            TexViewDesc.Swizzle.AsUint32());
        ASSERT_SIZEOF64(TexViewDesc, 40, "Did you add new members to TextureViewDesc? Please handle them here.");
    }
};


template <typename HasherType>
struct HashCombiner<HasherType, SampleDesc> : HashCombinerBase<HasherType>
{
    HashCombiner(HasherType& Hasher) :
        HashCombinerBase<HasherType>{Hasher}
    {}

    void operator()(const SampleDesc& SmplDesc) const
    {
        ASSERT_SIZEOF(SmplDesc.Count, 1, "Hash logic below may be incorrect.");
        ASSERT_SIZEOF(SmplDesc.Quality, 1, "Hash logic below may be incorrect.");

        this->m_Hasher(
            ((static_cast<uint32_t>(SmplDesc.Count) << 0u) |
             (static_cast<uint32_t>(SmplDesc.Quality) << 8u)));
        ASSERT_SIZEOF(SmplDesc, 2, "Did you add new members to SampleDesc? Please handle them here.");
    }
};


template <typename HasherType>
struct HashCombiner<HasherType, ShaderResourceVariableDesc> : HashCombinerBase<HasherType>
{
    HashCombiner(HasherType& Hasher) :
        HashCombinerBase<HasherType>{Hasher}
    {}

    void operator()(const ShaderResourceVariableDesc& VarDesc) const
    {
        ASSERT_SIZEOF(VarDesc.Type, 1, "Hash logic below may be incorrect.");
        ASSERT_SIZEOF(VarDesc.Flags, 1, "Hash logic below may be incorrect.");

        this->m_Hasher(
            VarDesc.Name,
            VarDesc.ShaderStages,
            ((static_cast<uint32_t>(VarDesc.Type) << 0u) |
             (static_cast<uint32_t>(VarDesc.Flags) << 8u)));
        ASSERT_SIZEOF64(VarDesc, 16, "Did you add new members to ShaderResourceVariableDesc? Please handle them here.");
    }
};


template <typename HasherType>
struct HashCombiner<HasherType, ImmutableSamplerDesc> : HashCombinerBase<HasherType>
{
    HashCombiner(HasherType& Hasher) :
        HashCombinerBase<HasherType>{Hasher}
    {}

    void operator()(const ImmutableSamplerDesc& SamDesc) const
    {
        this->m_Hasher(SamDesc.ShaderStages, SamDesc.SamplerOrTextureName, SamDesc.Desc);
        ASSERT_SIZEOF64(SamDesc, 16 + sizeof(SamplerDesc), "Did you add new members to ImmutableSamplerDesc? Please handle them here.");
    }
};


template <typename HasherType>
struct HashCombiner<HasherType, PipelineResourceDesc> : HashCombinerBase<HasherType>
{
    HashCombiner(HasherType& Hasher) :
        HashCombinerBase<HasherType>{Hasher}
    {}

    void operator()(const PipelineResourceDesc& ResDesc) const
    {
        ASSERT_SIZEOF(ResDesc.ResourceType, 1, "Hash logic below may be incorrect.");
        ASSERT_SIZEOF(ResDesc.VarType, 1, "Hash logic below may be incorrect.");
        ASSERT_SIZEOF(ResDesc.Flags, 1, "Hash logic below may be incorrect.");

        this->m_Hasher(
            ResDesc.Name,
            ResDesc.ShaderStages,
            ResDesc.ArraySize,
            ((static_cast<uint32_t>(ResDesc.ResourceType) << 0u) |
             (static_cast<uint32_t>(ResDesc.VarType) << 8u) |
             (static_cast<uint32_t>(ResDesc.Flags) << 16u)));
        ASSERT_SIZEOF64(ResDesc, 24, "Did you add new members to PipelineResourceDesc? Please handle them here.");
    }
};


template <typename HasherType>
struct HashCombiner<HasherType, PipelineResourceLayoutDesc> : HashCombinerBase<HasherType>
{
    HashCombiner(HasherType& Hasher) :
        HashCombinerBase<HasherType>{Hasher}
    {}

    void operator()(const PipelineResourceLayoutDesc& LayoutDesc) const
    {
        this->m_Hasher(
            LayoutDesc.DefaultVariableType,
            LayoutDesc.DefaultVariableMergeStages,
            LayoutDesc.NumVariables,
            LayoutDesc.NumImmutableSamplers);

        if (LayoutDesc.Variables != nullptr)
        {
            for (size_t i = 0; i < LayoutDesc.NumVariables; ++i)
                this->m_Hasher(LayoutDesc.Variables[i]);
        }
        else
        {
            VERIFY_EXPR(LayoutDesc.NumVariables == 0);
        }

        if (LayoutDesc.ImmutableSamplers != nullptr)
        {
            for (size_t i = 0; i < LayoutDesc.NumImmutableSamplers; ++i)
                this->m_Hasher(LayoutDesc.ImmutableSamplers[i]);
        }
        else
        {
            VERIFY_EXPR(LayoutDesc.NumImmutableSamplers == 0);
        }

        ASSERT_SIZEOF64(LayoutDesc, 40, "Did you add new members to PipelineResourceDesc? Please handle them here.");
    }
};



template <typename HasherType>
struct HashCombiner<HasherType, RenderPassAttachmentDesc> : HashCombinerBase<HasherType>
{
    HashCombiner(HasherType& Hasher) :
        HashCombinerBase<HasherType>{Hasher}
    {}

    void operator()(const RenderPassAttachmentDesc& Desc) const
    {
        ASSERT_SIZEOF(Desc.Format, 2, "Hash logic below may be incorrect.");
        ASSERT_SIZEOF(Desc.SampleCount, 1, "Hash logic below may be incorrect.");
        ASSERT_SIZEOF(Desc.LoadOp, 1, "Hash logic below may be incorrect.");
        ASSERT_SIZEOF(Desc.StoreOp, 1, "Hash logic below may be incorrect.");
        ASSERT_SIZEOF(Desc.StencilLoadOp, 1, "Hash logic below may be incorrect.");
        ASSERT_SIZEOF(Desc.StencilStoreOp, 1, "Hash logic below may be incorrect.");

        this->m_Hasher(
            ((static_cast<uint32_t>(Desc.Format) << 0u) |
             (static_cast<uint32_t>(Desc.SampleCount) << 16u) |
             (static_cast<uint32_t>(Desc.LoadOp) << 24u)),
            ((static_cast<uint32_t>(Desc.StoreOp) << 0u) |
             (static_cast<uint32_t>(Desc.StencilLoadOp) << 8u) |
             (static_cast<uint32_t>(Desc.StencilStoreOp) << 16u)),
            Desc.InitialState,
            Desc.FinalState);
        ASSERT_SIZEOF(Desc, 16, "Did you add new members to RenderPassAttachmentDesc? Please handle them here.");
    }
};


template <typename HasherType>
struct HashCombiner<HasherType, AttachmentReference> : HashCombinerBase<HasherType>
{
    HashCombiner(HasherType& Hasher) :
        HashCombinerBase<HasherType>{Hasher}
    {}

    void operator()(const AttachmentReference& Ref) const
    {
        this->m_Hasher(Ref.AttachmentIndex, Ref.State);
        ASSERT_SIZEOF(Ref, 8, "Did you add new members to AttachmentReference? Please handle them here.");
    }
};


template <typename HasherType>
struct HashCombiner<HasherType, ShadingRateAttachment> : HashCombinerBase<HasherType>
{
    HashCombiner(HasherType& Hasher) :
        HashCombinerBase<HasherType>{Hasher}
    {}

    void operator()(const ShadingRateAttachment& SRA) const
    {
        ASSERT_SIZEOF(SRA.TileSize, 8, "Hash logic below may be incorrect.");
        this->m_Hasher(SRA.Attachment, SRA.TileSize[0], SRA.TileSize[1]);
        ASSERT_SIZEOF(SRA, 16, "Did you add new members to AttachmentReference? Please handle them here.");
    }
};


template <typename HasherType>
struct HashCombiner<HasherType, SubpassDesc> : HashCombinerBase<HasherType>
{
    HashCombiner(HasherType& Hasher) :
        HashCombinerBase<HasherType>{Hasher}
    {}

    void operator()(const SubpassDesc& Subpass) const
    {
        this->m_Hasher(
            Subpass.InputAttachmentCount,
            Subpass.RenderTargetAttachmentCount,
            Subpass.PreserveAttachmentCount);

        if (Subpass.pInputAttachments != nullptr)
        {
            for (size_t i = 0; i < Subpass.InputAttachmentCount; ++i)
                this->m_Hasher(Subpass.pInputAttachments[i]);
        }
        else
        {
            VERIFY_EXPR(Subpass.InputAttachmentCount == 0);
        }

        if (Subpass.pRenderTargetAttachments != nullptr)
        {
            for (size_t i = 0; i < Subpass.RenderTargetAttachmentCount; ++i)
                this->m_Hasher(Subpass.pRenderTargetAttachments[i]);
        }
        else
        {
            VERIFY_EXPR(Subpass.RenderTargetAttachmentCount == 0);
        }

        if (Subpass.pResolveAttachments != nullptr)
        {
            for (size_t i = 0; i < Subpass.RenderTargetAttachmentCount; ++i)
                this->m_Hasher(Subpass.pResolveAttachments[i]);
        }

        if (Subpass.pDepthStencilAttachment)
            this->m_Hasher(*Subpass.pDepthStencilAttachment);

        if (Subpass.pPreserveAttachments != nullptr)
        {
            for (size_t i = 0; i < Subpass.PreserveAttachmentCount; ++i)
                this->m_Hasher(Subpass.pPreserveAttachments[i]);
        }
        else
        {
            VERIFY_EXPR(Subpass.PreserveAttachmentCount == 0);
        }

        if (Subpass.pShadingRateAttachment)
            this->m_Hasher(*Subpass.pShadingRateAttachment);

        ASSERT_SIZEOF64(Subpass, 72, "Did you add new members to SubpassDesc? Please handle them here.");
    }
};


template <typename HasherType>
struct HashCombiner<HasherType, SubpassDependencyDesc> : HashCombinerBase<HasherType>
{
    HashCombiner(HasherType& Hasher) :
        HashCombinerBase<HasherType>{Hasher}
    {}

    void operator()(const SubpassDependencyDesc& Dep) const
    {
        this->m_Hasher(
            Dep.SrcSubpass,
            Dep.DstSubpass,
            Dep.SrcStageMask,
            Dep.DstStageMask,
            Dep.SrcAccessMask,
            Dep.DstAccessMask);
        ASSERT_SIZEOF(Dep, 24, "Did you add new members to SubpassDependencyDesc? Please handle them here.");
    }
};


template <typename HasherType>
struct HashCombiner<HasherType, RenderPassDesc> : HashCombinerBase<HasherType>
{
    HashCombiner(HasherType& Hasher) :
        HashCombinerBase<HasherType>{Hasher}
    {}

    void operator()(const RenderPassDesc& RP) const
    {
        // Ignore Name. This is consistent with the operator==
        this->m_Hasher(
            RP.AttachmentCount,
            RP.SubpassCount,
            RP.DependencyCount);

        if (RP.pAttachments != nullptr)
        {
            for (size_t i = 0; i < RP.AttachmentCount; ++i)
                this->m_Hasher(RP.pAttachments[i]);
        }
        else
        {
            VERIFY_EXPR(RP.AttachmentCount == 0);
        }

        if (RP.pSubpasses != nullptr)
        {
            for (size_t i = 0; i < RP.SubpassCount; ++i)
                this->m_Hasher(RP.pSubpasses[i]);
        }
        else
        {
            VERIFY_EXPR(RP.SubpassCount == 0);
        }

        if (RP.pDependencies != nullptr)
        {
            for (size_t i = 0; i < RP.DependencyCount; ++i)
                this->m_Hasher(RP.pDependencies[i]);
        }
        else
        {
            VERIFY_EXPR(RP.DependencyCount == 0);
        }

        ASSERT_SIZEOF64(RP, 56, "Did you add new members to RenderPassDesc? Please handle them here.");
    }
};


template <typename HasherType>
struct HashCombiner<HasherType, LayoutElement> : HashCombinerBase<HasherType>
{
    HashCombiner(HasherType& Hasher) :
        HashCombinerBase<HasherType>{Hasher}
    {}

    void operator()(const LayoutElement& Elem) const
    {
        ASSERT_SIZEOF(Elem.ValueType, 1, "Hash logic below may be incorrect.");
        ASSERT_SIZEOF(Elem.IsNormalized, 1, "Hash logic below may be incorrect.");
        ASSERT_SIZEOF(Elem.Frequency, 1, "Hash logic below may be incorrect.");

        this->m_Hasher(
            Elem.HLSLSemantic,
            Elem.InputIndex,
            Elem.BufferSlot,
            Elem.NumComponents,
            ((static_cast<uint32_t>(Elem.ValueType) << 0u) |
             ((Elem.IsNormalized ? 1u : 0u) << 8u) |
             (static_cast<uint32_t>(Elem.Frequency) << 16u)),
            Elem.RelativeOffset,
            Elem.Stride,
            Elem.InstanceDataStepRate);
        ASSERT_SIZEOF64(Elem, 40, "Did you add new members to LayoutElement? Please handle them here.");
    }
};


template <typename HasherType>
struct HashCombiner<HasherType, InputLayoutDesc> : HashCombinerBase<HasherType>
{
    HashCombiner(HasherType& Hasher) :
        HashCombinerBase<HasherType>{Hasher}
    {}

    void operator()(const InputLayoutDesc& Layout) const
    {
        this->m_Hasher(Layout.NumElements);
        if (Layout.LayoutElements != nullptr)
        {
            for (size_t i = 0; i < Layout.NumElements; ++i)
                this->m_Hasher(Layout.LayoutElements[i]);
        }
        else
        {
            VERIFY_EXPR(Layout.NumElements == 0);
        }

        ASSERT_SIZEOF64(Layout, 16, "Did you add new members to InputLayoutDesc? Please handle them here.");
    }
};


template <typename HasherType>
struct HashCombiner<HasherType, GraphicsPipelineDesc> : HashCombinerBase<HasherType>
{
    HashCombiner(HasherType& Hasher) :
        HashCombinerBase<HasherType>{Hasher}
    {}

    void operator()(const GraphicsPipelineDesc& Desc) const
    {
        ASSERT_SIZEOF(Desc.NumViewports, 1, "Hash logic below may be incorrect.");
        ASSERT_SIZEOF(Desc.NumRenderTargets, 1, "Hash logic below may be incorrect.");
        ASSERT_SIZEOF(Desc.SubpassIndex, 1, "Hash logic below may be incorrect.");
        ASSERT_SIZEOF(Desc.ShadingRateFlags, 1, "Hash logic below may be incorrect.");

        this->m_Hasher(
            Desc.BlendDesc,
            Desc.SampleMask,
            Desc.RasterizerDesc,
            Desc.DepthStencilDesc,
            Desc.InputLayout,
            Desc.PrimitiveTopology,
            Desc.NumViewports,
            ((static_cast<uint32_t>(Desc.NumViewports) << 0u) |
             (static_cast<uint32_t>(Desc.NumRenderTargets) << 8u) |
             (static_cast<uint32_t>(Desc.SubpassIndex) << 16u) |
             (static_cast<uint32_t>(Desc.ShadingRateFlags) << 24u)));

        for (size_t i = 0; i < Desc.NumRenderTargets; ++i)
            this->m_Hasher(Desc.RTVFormats[i]);

        this->m_Hasher(Desc.DSVFormat,
                       Desc.SmplDesc,
                       Desc.NodeMask);

        if (Desc.pRenderPass != nullptr)
            this->m_Hasher(Desc.pRenderPass->GetDesc());
    }
};


template <typename HasherType>
struct HashCombiner<HasherType, RayTracingPipelineDesc> : HashCombinerBase<HasherType>
{
    HashCombiner(HasherType& Hasher) :
        HashCombinerBase<HasherType>{Hasher}
    {}

    void operator()(const RayTracingPipelineDesc& Desc) const
    {
        ASSERT_SIZEOF(Desc.ShaderRecordSize, 2, "Hash logic below may be incorrect.");
        ASSERT_SIZEOF(Desc.MaxRecursionDepth, 1, "Hash logic below may be incorrect.");
        this->m_Hasher(
            ((static_cast<uint32_t>(Desc.ShaderRecordSize) << 0u) |
             (static_cast<uint32_t>(Desc.MaxRecursionDepth) << 16u)));
        ASSERT_SIZEOF(Desc, 4, "Did you add new members to RayTracingPipelineDesc? Please handle them here.");
    }
};


template <typename HasherType>
struct HashCombiner<HasherType, PipelineStateDesc> : HashCombinerBase<HasherType>
{
    HashCombiner(HasherType& Hasher) :
        HashCombinerBase<HasherType>{Hasher}
    {}

    void operator()(const PipelineStateDesc& Desc) const
    {
        // Ignore Name. This is consistent with the operator==
        this->m_Hasher(
            Desc.PipelineType,
            Desc.SRBAllocationGranularity,
            Desc.ImmediateContextMask,
            Desc.ResourceLayout);
    }
};


template <typename HasherType>
struct HashCombiner<HasherType, PipelineResourceSignatureDesc> : HashCombinerBase<HasherType>
{
    HashCombiner(HasherType& Hasher) :
        HashCombinerBase<HasherType>{Hasher}
    {}

    void operator()(const PipelineResourceSignatureDesc& Desc) const
    {
        ASSERT_SIZEOF(Desc.BindingIndex, 1, "Hash logic below may be incorrect.");
        ASSERT_SIZEOF(Desc.UseCombinedTextureSamplers, 1, "Hash logic below may be incorrect.");
        // Ignore Name. This is consistent with the operator==
        this->m_Hasher(
            Desc.NumResources,
            Desc.NumImmutableSamplers,
            ((static_cast<uint32_t>(Desc.BindingIndex) << 0u) |
             (static_cast<uint32_t>(Desc.UseCombinedTextureSamplers) << 8u)),
            Desc.SRBAllocationGranularity);

        if (Desc.Resources != nullptr)
        {
            for (size_t i = 0; i < Desc.NumResources; ++i)
                this->m_Hasher(Desc.Resources[i]);
        }
        else
        {
            VERIFY_EXPR(Desc.NumResources == 0);
        }

        if (Desc.ImmutableSamplers != nullptr)
        {
            for (size_t i = 0; i < Desc.NumImmutableSamplers; ++i)
                this->m_Hasher(Desc.ImmutableSamplers[i]);
        }
        else
        {
            VERIFY_EXPR(Desc.NumImmutableSamplers == 0);
        }

        if (Desc.UseCombinedTextureSamplers)
            this->m_Hasher(Desc.CombinedSamplerSuffix);

        ASSERT_SIZEOF64(Desc, 56, "Did you add new members to PipelineResourceSignatureDesc? Please handle them here.");
    }
};


template <typename HasherType>
struct HashCombiner<HasherType, ShaderDesc> : HashCombinerBase<HasherType>
{
    HashCombiner(HasherType& Hasher) :
        HashCombinerBase<HasherType>{Hasher}
    {}

    void operator()(const ShaderDesc& Desc) const
    {
        // Ignore Name. This is consistent with the operator==
        this->m_Hasher(
            Desc.ShaderType,
            Desc.UseCombinedTextureSamplers,
            Desc.CombinedSamplerSuffix);

        ASSERT_SIZEOF64(Desc, 24, "Did you add new members to ShaderDesc? Please handle them here.");
    }
};


template <typename HasherType>
struct HashCombiner<HasherType, Version> : HashCombinerBase<HasherType>
{
    HashCombiner(HasherType& Hasher) :
        HashCombinerBase<HasherType>{Hasher}
    {}

    void operator()(const Version& Ver) const
    {
        this->m_Hasher(Ver.Minor, Ver.Major);
        ASSERT_SIZEOF64(Ver, 8, "Did you add new members to Version? Please handle them here.");
    }
};


template <typename HasherType>
struct HashCombiner<HasherType, PipelineStateCreateInfo> : HashCombinerBase<HasherType>
{
    HashCombiner(HasherType& Hasher) :
        HashCombinerBase<HasherType>{Hasher}
    {}

    void operator()(const PipelineStateCreateInfo& CI) const
    {
        this->m_Hasher(
            CI.PSODesc,
            CI.Flags,
            CI.ResourceSignaturesCount);
        if (CI.ppResourceSignatures != nullptr)
        {
            for (size_t i = 0; i < CI.ResourceSignaturesCount; ++i)
            {
                if (const auto* pSign = CI.ppResourceSignatures[i])
                {
                    this->m_Hasher(pSign->GetDesc());
                }
            }
        }
        else
        {
            VERIFY_EXPR(CI.ResourceSignaturesCount == 0);
        }
    }
};

template <typename HasherType>
void HashShaderBytecode(HasherType& Hasher, IShader* pShader)
{
    if (pShader == nullptr)
        return;

    const void* pBytecode = nullptr;
    Uint64      Size      = 0;
    pShader->GetBytecode(&pBytecode, Size);
    VERIFY_EXPR(pBytecode != nullptr && Size != 0);
    Hasher.UpdateRaw(pBytecode, static_cast<size_t>(Size));
}

template <typename HasherType>
struct HashCombiner<HasherType, GraphicsPipelineStateCreateInfo> : HashCombinerBase<HasherType>
{
    HashCombiner(HasherType& Hasher) :
        HashCombinerBase<HasherType>{Hasher}
    {}

    void operator()(const GraphicsPipelineStateCreateInfo& CI) const
    {
        this->m_Hasher(
            static_cast<const PipelineStateCreateInfo&>(CI),
            CI.GraphicsPipeline);
        HashShaderBytecode(this->m_Hasher, CI.pVS);
        HashShaderBytecode(this->m_Hasher, CI.pPS);
        HashShaderBytecode(this->m_Hasher, CI.pDS);
        HashShaderBytecode(this->m_Hasher, CI.pHS);
        HashShaderBytecode(this->m_Hasher, CI.pGS);
        HashShaderBytecode(this->m_Hasher, CI.pAS);
        HashShaderBytecode(this->m_Hasher, CI.pMS);
    }
};

template <typename HasherType>
struct HashCombiner<HasherType, ComputePipelineStateCreateInfo> : HashCombinerBase<HasherType>
{
    HashCombiner(HasherType& Hasher) :
        HashCombinerBase<HasherType>{Hasher}
    {}

    void operator()(const ComputePipelineStateCreateInfo& CI) const
    {
        this->m_Hasher(static_cast<const PipelineStateCreateInfo&>(CI));
        HashShaderBytecode(this->m_Hasher, CI.pCS);
    }
};

template <typename HasherType>
struct HashCombiner<HasherType, RayTracingPipelineStateCreateInfo> : HashCombinerBase<HasherType>
{
    HashCombiner(HasherType& Hasher) :
        HashCombinerBase<HasherType>{Hasher}
    {}

    void operator()(const RayTracingPipelineStateCreateInfo& CI) const
    {
        this->m_Hasher(
            static_cast<const PipelineStateCreateInfo&>(CI),
            CI.RayTracingPipeline,
            CI.GeneralShaderCount,
            CI.TriangleHitShaderCount,
            CI.ProceduralHitShaderCount,
            CI.pShaderRecordName,
            CI.MaxAttributeSize,
            CI.MaxPayloadSize);

        for (size_t i = 0; i < CI.GeneralShaderCount; ++i)
        {
            const auto& GeneralShader = CI.pGeneralShaders[i];
            this->m_Hasher(GeneralShader.Name);
            HashShaderBytecode(this->m_Hasher, GeneralShader.pShader);
        }

        for (size_t i = 0; i < CI.TriangleHitShaderCount; ++i)
        {
            const auto& TriHitShader = CI.pTriangleHitShaders[i];
            this->m_Hasher(TriHitShader.Name);
            HashShaderBytecode(this->m_Hasher, TriHitShader.pAnyHitShader);
            HashShaderBytecode(this->m_Hasher, TriHitShader.pClosestHitShader);
        }

        for (size_t i = 0; i < CI.ProceduralHitShaderCount; ++i)
        {
            const auto& ProcHitShader = CI.pProceduralHitShaders[i];
            this->m_Hasher(ProcHitShader.Name);
            HashShaderBytecode(this->m_Hasher, ProcHitShader.pAnyHitShader);
            HashShaderBytecode(this->m_Hasher, ProcHitShader.pClosestHitShader);
            HashShaderBytecode(this->m_Hasher, ProcHitShader.pIntersectionShader);
        }
    }
};

template <typename HasherType>
struct HashCombiner<HasherType, TilePipelineDesc> : HashCombinerBase<HasherType>
{
    HashCombiner(HasherType& Hasher) :
        HashCombinerBase<HasherType>{Hasher}
    {}

    void operator()(const TilePipelineDesc& Desc) const
    {
        ASSERT_SIZEOF(Desc.NumRenderTargets, 1, "Hash logic below may be incorrect.");
        ASSERT_SIZEOF(Desc.SampleCount, 1, "Hash logic below may be incorrect.");

        this->m_Hasher(
            ((static_cast<uint32_t>(Desc.NumRenderTargets) << 0u) |
             (static_cast<uint32_t>(Desc.SampleCount) << 8u)));

        for (size_t i = 0; i < Desc.NumRenderTargets; ++i)
            this->m_Hasher(Desc.RTVFormats[i]);
    }
};

template <typename HasherType>
struct HashCombiner<HasherType, TilePipelineStateCreateInfo> : HashCombinerBase<HasherType>
{
    HashCombiner(HasherType& Hasher) :
        HashCombinerBase<HasherType>{Hasher}
    {}

    void operator()(const TilePipelineStateCreateInfo& CI) const
    {
        this->m_Hasher(static_cast<const PipelineStateCreateInfo&>(CI), CI.TilePipeline);
        HashShaderBytecode(this->m_Hasher, CI.pTS);
    }
};

template <typename HasherType>
struct HashCombiner<HasherType, VertexPoolElementDesc> : HashCombinerBase<HasherType>
{
    HashCombiner(HasherType& Hasher) :
        HashCombinerBase<HasherType>{Hasher}
    {}

    void operator()(const VertexPoolElementDesc& Desc) const
    {
        ASSERT_SIZEOF(Desc.Usage, 1, "Hash logic below may be incorrect.");
        ASSERT_SIZEOF(Desc.CPUAccessFlags, 1, "Hash logic below may be incorrect.");
        ASSERT_SIZEOF(Desc.Mode, 1, "Hash logic below may be incorrect.");

        this->m_Hasher(
            Desc.Size,
            Desc.BindFlags,
            ((static_cast<uint32_t>(Desc.Usage) << 0u) |
             (static_cast<uint32_t>(Desc.CPUAccessFlags) << 8u) |
             (static_cast<uint32_t>(Desc.Mode) << 16u)));
    }
};

struct DefaultHasher
{
    template <typename... ArgsType>
    std::size_t operator()(const ArgsType&... Args) noexcept
    {
        HashCombine(m_Seed, Args...);
        return m_Seed;
    }

    void UpdateRaw(const void* pData, uint64_t Size) noexcept
    {
        HashCombine(m_Seed, ComputeHashRaw(pData, static_cast<size_t>(Size)));
    }

    size_t Get() const
    {
        return m_Seed;
    }

private:
    size_t m_Seed = 0;
};

template <typename Type>
struct StdHasher
{
    size_t operator()(const Type& Val) const
    {
        DefaultHasher                     Hasher;
        HashCombiner<DefaultHasher, Type> Combiner{Hasher};
        Combiner(Val);
        return Hasher.Get();
    }
};

} // namespace Diligent


namespace std
{

template <typename T>
struct hash<Diligent::RefCntAutoPtr<T>>
{
    size_t operator()(const Diligent::RefCntAutoPtr<T>& Key) const noexcept
    {
        return std::hash<const T*>{}(static_cast<const T*>(Key));
    }
};


template <>
struct hash<Diligent::HashMapStringKey>
{
    size_t operator()(const Diligent::HashMapStringKey& Key) const noexcept
    {
        return Key.GetHash();
    }
};


#define DEFINE_HASH(Type)                        \
    template <>                                  \
    struct hash<Type>                            \
    {                                            \
        size_t operator()(const Type& Val) const \
        {                                        \
            Diligent::StdHasher<Type> Hasher;    \
            return Hasher(Val);                  \
        }                                        \
    }

DEFINE_HASH(Diligent::SamplerDesc);
DEFINE_HASH(Diligent::StencilOpDesc);
DEFINE_HASH(Diligent::DepthStencilStateDesc);
DEFINE_HASH(Diligent::RasterizerStateDesc);
DEFINE_HASH(Diligent::BlendStateDesc);
DEFINE_HASH(Diligent::TextureViewDesc);
DEFINE_HASH(Diligent::SampleDesc);
DEFINE_HASH(Diligent::ShaderResourceVariableDesc);
DEFINE_HASH(Diligent::ImmutableSamplerDesc);
DEFINE_HASH(Diligent::PipelineResourceDesc);
DEFINE_HASH(Diligent::PipelineResourceLayoutDesc);
DEFINE_HASH(Diligent::RenderPassAttachmentDesc);
DEFINE_HASH(Diligent::AttachmentReference);
DEFINE_HASH(Diligent::ShadingRateAttachment);
DEFINE_HASH(Diligent::SubpassDesc);
DEFINE_HASH(Diligent::SubpassDependencyDesc);
DEFINE_HASH(Diligent::RenderPassDesc);
DEFINE_HASH(Diligent::LayoutElement);
DEFINE_HASH(Diligent::InputLayoutDesc);
DEFINE_HASH(Diligent::GraphicsPipelineDesc);
DEFINE_HASH(Diligent::RayTracingPipelineDesc);
DEFINE_HASH(Diligent::PipelineStateDesc);
DEFINE_HASH(Diligent::PipelineResourceSignatureDesc);
DEFINE_HASH(Diligent::ShaderDesc);
DEFINE_HASH(Diligent::Version);
DEFINE_HASH(Diligent::PipelineStateCreateInfo);
DEFINE_HASH(Diligent::GraphicsPipelineStateCreateInfo);
DEFINE_HASH(Diligent::ComputePipelineStateCreateInfo);
DEFINE_HASH(Diligent::RayTracingPipelineStateCreateInfo);
DEFINE_HASH(Diligent::TilePipelineDesc);
DEFINE_HASH(Diligent::TilePipelineStateCreateInfo);
DEFINE_HASH(Diligent::VertexPoolElementDesc);


#undef DEFINE_HASH

} // namespace std
