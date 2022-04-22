/*
 *  Copyright 2019-2022 Diligent Graphics LLC
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

#include <type_traits>
#include <array>
#include <cstring>
#include <atomic>

#include "../../Primitives/interface/BasicTypes.h"
#include "../../Primitives/interface/MemoryAllocator.h"
#include "../../Primitives/interface/CheckBaseStructAlignment.hpp"
#include "../../Platforms/Basic/interface/DebugUtilities.hpp"
#include "DynamicLinearAllocator.hpp"
#include "Align.hpp"

namespace Diligent
{

class SerializedData
{
public:
    SerializedData() {}

    SerializedData(void* pData, size_t Size) noexcept;

    SerializedData(size_t Size, IMemoryAllocator& Allocator) noexcept;

    SerializedData(SerializedData&& Other) noexcept :
        // clang-format off
        m_pAllocator{Other.m_pAllocator},
        m_Ptr       {Other.m_Ptr},
        m_Size      {Other.m_Size},
        m_Hash      {Other.m_Hash.load()}
    // clang-format on
    {
        Other.m_pAllocator = nullptr;
        Other.m_Ptr        = nullptr;
        Other.m_Size       = 0;
        Other.m_Hash.store(0);

        ASSERT_SIZEOF64(*this, 32, "Please handle new members here");
    }

    SerializedData& operator=(SerializedData&& Rhs) noexcept;

    SerializedData(const SerializedData&) = delete;
    SerializedData& operator=(const SerializedData&) = delete;

    ~SerializedData();

    explicit operator bool() const { return m_Ptr != nullptr; }

    bool operator==(const SerializedData& Rhs) const;
    bool operator!=(const SerializedData& Rhs) const
    {
        return !(*this == Rhs);
    }

    void*  Ptr() const { return m_Ptr; }
    size_t Size() const { return m_Size; }

    template <typename T>
    T* Ptr() const { return reinterpret_cast<T*>(m_Ptr); }

    size_t GetHash() const;

    void Free();

    struct Hasher
    {
        size_t operator()(const SerializedData& Mem) const
        {
            return Mem.GetHash();
        }
    };

private:
    IMemoryAllocator* m_pAllocator = nullptr;

    void*  m_Ptr  = nullptr;
    size_t m_Size = 0;

    mutable std::atomic<size_t> m_Hash{0};
};



template <typename T>
struct IsTriviallySerializable
{
    static constexpr bool value = std::is_floating_point<T>::value || std::is_integral<T>::value || std::is_enum<T>::value;
};

template <typename T, size_t Size>
struct IsTriviallySerializable<std::array<T, Size>>
{
    static constexpr bool value = IsTriviallySerializable<T>::value;
};

template <typename T, size_t Size>
struct IsTriviallySerializable<T[Size]>
{
    static constexpr bool value = IsTriviallySerializable<T>::value;
};

#define DECL_TRIVIALLY_SERIALIZABLE(Type)   \
    template <>                             \
    struct IsTriviallySerializable<Type>    \
    {                                       \
        static constexpr bool value = true; \
    }

enum class SerializerMode
{
    Read,
    Write,
    Measure
};


template <SerializerMode Mode>
class Serializer
{
public:
    template <typename T>
    using RawType = typename std::remove_const_t<std::remove_reference_t<T>>;

    template <typename T>
    using TEnable = typename std::enable_if_t<IsTriviallySerializable<T>::value, void>;

    template <typename T>
    using TEnableStr = typename std::enable_if_t<(std::is_same<const char* const, T>::value || std::is_same<const char*, T>::value), void>;
    using CharPtr    = typename std::conditional_t<Mode == SerializerMode::Read, const char*&, const char*>;
    using VoidPtr    = typename std::conditional_t<Mode == SerializerMode::Read, const void*&, const void*>;

    template <typename T>
    using TReadOnly = typename std::enable_if_t<Mode == SerializerMode::Read, const T*>;

    using TPointer = typename std::conditional_t<Mode == SerializerMode::Write, Uint8*, const Uint8*>;

    template <typename T>
    using ConstQual = typename std::conditional_t<Mode == SerializerMode::Read, T, const T>;


    Serializer() :
        // clang-format off
        m_Start{nullptr},
        m_End  {m_Start + ~0u},
        m_Ptr  {m_Start}
    // clang-format on
    {
        static_assert(Mode == SerializerMode::Measure, "Only Measure mode is supported");
    }

    explicit Serializer(const SerializedData& Data) :
        // clang-format off
        m_Start{static_cast<TPointer>(Data.Ptr())},
        m_End  {m_Start + Data.Size()},
        m_Ptr  {m_Start}
    // clang-format on
    {
        static_assert(Mode == SerializerMode::Read || Mode == SerializerMode::Write, "Only Read or Write mode is supported");
    }

    template <typename T>
    TEnable<T> Serialize(ConstQual<T>& Value)
    {
        Copy(&Value, sizeof(Value));
    }

    // Copies Size bytes to/from pData
    void CopyBytes(ConstQual<void>* pData, size_t Size)
    {
        Copy(pData, Size);
    }

    template <typename T>
    TEnableStr<T> Serialize(CharPtr Str);

    /// Serializes Size bytes to/from pBytes
    ///
    ///  * Measure
    ///      Writes Size as Uint32 (noop)
    ///      Aligns up current offset to Alignment bytes
    ///      Writes Size bytes (noop)
    ///
    ///  * Write
    ///      Writes Size as Uint32
    ///      Aligns up current offset to Alignment bytes
    ///      Writes Size bytes from pBytes
    ///
    ///  * Read
    ///      Reads Size as Uint32
    ///      Aligns up current offset to Alignment bytes
    ///      Sets pBytes to m_Ptr
    ///      Moves m_Ptr by Size bytes
    void SerializeBytes(VoidPtr pBytes, ConstQual<size_t>& Size, size_t Alignment = 8);

    template <typename ElemPtrType, typename CountType, typename ArrayElemSerializerType>
    void SerializeArray(DynamicLinearAllocator* Allocator,
                        ElemPtrType&            Elements,
                        CountType&              Count,
                        ArrayElemSerializerType ElemSerializer);

    template <typename ElemPtrType, typename CountType>
    void SerializeArrayRaw(DynamicLinearAllocator* Allocator,
                           ElemPtrType&            Elements,
                           CountType&              Count);

    template <typename T>
    TReadOnly<T> Cast()
    {
        static_assert(std::is_trivially_destructible<T>::value, "Can not cast to non triavial type");
        VERIFY(reinterpret_cast<size_t>(m_Ptr) % alignof(T) == 0, "Pointer must be properly aligned");
        VERIFY_EXPR(m_Ptr + sizeof(T) <= m_End);
        auto* Ptr = m_Ptr;
        m_Ptr += sizeof(T);
        return reinterpret_cast<const T*>(Ptr);
    }

    template <typename Arg0Type, typename... ArgTypes>
    void operator()(Arg0Type& Arg0, ArgTypes&... Args)
    {
        Serialize<RawType<Arg0Type>>(Arg0);
        operator()(Args...);
    }

    template <typename Arg0Type>
    void operator()(Arg0Type& Arg0)
    {
        Serialize<RawType<Arg0Type>>(Arg0);
    }

    size_t GetSize() const
    {
        VERIFY_EXPR(m_Ptr >= m_Start);
        return m_Ptr - m_Start;
    }

    size_t GetRemainingSize() const
    {
        VERIFY_EXPR(m_End >= m_Ptr);
        return m_End - m_Ptr;
    }

    const void* GetCurrentPtr() const
    {
        return m_Ptr;
    }

    bool IsEnded() const
    {
        return m_Ptr == m_End;
    }

    SerializedData AllocateData(IMemoryAllocator& Allocator) const
    {
        static_assert(Mode == SerializerMode::Measure, "This method is only allowed in Measure mode");
        return SerializedData{GetSize(), Allocator};
    }

    static constexpr SerializerMode GetMode() { return Mode; }

private:
    template <typename T>
    void Copy(T* pData, size_t Size);

    void AlignOffset(size_t Alignment)
    {
        const auto Size       = GetSize();
        const auto AlignShift = AlignUp(Size, size_t{8}) - Size;
        VERIFY_EXPR(m_Ptr + AlignShift <= m_End);
        m_Ptr += AlignShift;
    }

private:
    TPointer const m_Start = nullptr;
    TPointer const m_End   = nullptr;

    TPointer m_Ptr = nullptr;
};


template <>
template <typename T>
void Serializer<SerializerMode::Read>::Copy(T* pData, size_t Size)
{
    static_assert(IsAlignedBaseClass<T>::Value, "There is unused space at the end of the structure that may be filled with garbage. Use padding to zero-initialize this space and avoid nasty issues.");
    VERIFY(m_Ptr + Size <= m_End, "Note enough data to read ", Size, " bytes");
    std::memcpy(pData, m_Ptr, Size);
    m_Ptr += Size;
}

template <>
template <typename T>
void Serializer<SerializerMode::Write>::Copy(T* pData, size_t Size)
{
    static_assert(IsAlignedBaseClass<T>::Value, "There is unused space at the end of the structure that may be filled with garbage. Use padding to zero-initialize this space and avoid nasty issues.");
    VERIFY(m_Ptr + Size <= m_End, "Note enough data to write ", Size, " bytes");
    std::memcpy(m_Ptr, pData, Size);
    m_Ptr += Size;
}

template <>
template <typename T>
void Serializer<SerializerMode::Measure>::Copy(T* pData, size_t Size)
{
    VERIFY_EXPR(m_Ptr + Size <= m_End);
    m_Ptr += Size;
}

template <>
template <typename T>
typename Serializer<SerializerMode::Read>::TEnableStr<T> Serializer<SerializerMode::Read>::Serialize(CharPtr Str)
{
    Uint32 Length = 0;
    Serialize<Uint32>(Length);

    VERIFY(m_Ptr + Length <= m_End, "Note enough data to read ", Length, " characters.");
    Str = Length > 1 ? reinterpret_cast<const char*>(m_Ptr) : "";
    m_Ptr += Length;
}

template <SerializerMode Mode> // Write or Measure
template <typename T>
typename Serializer<Mode>::template TEnableStr<T> Serializer<Mode>::Serialize(CharPtr Str)
{
    static_assert(Mode == SerializerMode::Write || Mode == SerializerMode::Measure, "Unexpected mode");
    const Uint32 Length = static_cast<Uint32>((Str != nullptr && *Str != 0) ? strlen(Str) + 1 : 0);
    Serialize<Uint32>(Length);
    Copy(Str, Length);
}


template <>
inline void Serializer<SerializerMode::Read>::SerializeBytes(VoidPtr pBytes, ConstQual<size_t>& Size, size_t Alignment)
{
    Uint32 Size32 = 0;
    Serialize<Uint32>(Size32);
    Size = Size32;

    AlignOffset(Alignment);

    VERIFY(m_Ptr + Size <= m_End, "Note enough data to read ", Size, " bytes.");

    pBytes = m_Ptr;
    m_Ptr += Size;
}

template <SerializerMode Mode> // Write or Measure
inline void Serializer<Mode>::SerializeBytes(VoidPtr pBytes, ConstQual<size_t>& Size, size_t Alignment)
{
    static_assert(Mode == SerializerMode::Write || Mode == SerializerMode::Measure, "Unexpected mode");
    Serialize<Uint32>(static_cast<Uint32>(Size));
    AlignOffset(Alignment);
    Copy(pBytes, Size);
}


template <SerializerMode Mode>
template <typename ElemPtrType, typename CountType, typename ArrayElemSerializerType>
void Serializer<Mode>::SerializeArray(DynamicLinearAllocator* Allocator,
                                      ElemPtrType&            SrcArray,
                                      CountType&              Count,
                                      ArrayElemSerializerType ElemSerializer)
{
    VERIFY_EXPR((SrcArray != nullptr) == (Count != 0));

    (*this)(Count);
    for (size_t i = 0; i < static_cast<size_t>(Count); ++i)
    {
        ElemSerializer(*this, SrcArray[i]);
    }
}


template <>
template <typename ElemPtrType, typename CountType, typename ArrayElemSerializerType>
void Serializer<SerializerMode::Read>::SerializeArray(DynamicLinearAllocator* Allocator,
                                                      ElemPtrType&            DstArray,
                                                      CountType&              Count,
                                                      ArrayElemSerializerType ElemSerializer)
{
    VERIFY_EXPR(Allocator != nullptr);
    VERIFY_EXPR(DstArray == nullptr);

    (*this)(Count);
    auto* pDstElements = Allocator->ConstructArray<RawType<decltype(DstArray[0])>>(Count);
    for (size_t i = 0; i < static_cast<size_t>(Count); ++i)
    {
        ElemSerializer(*this, pDstElements[i]);
    }
    DstArray = pDstElements;
}


template <SerializerMode Mode>
template <typename ElemPtrType, typename CountType>
void Serializer<Mode>::SerializeArrayRaw(DynamicLinearAllocator* Allocator,
                                         ElemPtrType&            Elements,
                                         CountType&              Count)
{
    SerializeArray(Allocator, Elements, Count,
                   [](Serializer<Mode>& Ser, auto& Elem) //
                   {
                       Ser(Elem);
                   });
}

} // namespace Diligent


namespace std
{

template <>
struct hash<Diligent::SerializedData>
{
    size_t operator()(const Diligent::SerializedData& Data) const
    {
        return Data.GetHash();
    }
};

} // namespace std
