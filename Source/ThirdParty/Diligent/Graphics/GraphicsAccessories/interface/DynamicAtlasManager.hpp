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
/// Declaration of DynamicAtlasManager class

#include <map>
#include <unordered_map>

#include "../../../Primitives/interface/BasicTypes.h"
#include "../../../Common/interface/HashUtils.hpp"

namespace Diligent
{

/// Dynamic 2D atlas manager
class DynamicAtlasManager
{
public:
    struct Region
    {
        Uint32 x = 0;
        Uint32 y = 0;

        Uint32 width  = 0;
        Uint32 height = 0;

        Region() = default;

        // clang-format off
        Region           (const Region&)  = default;
        Region           (      Region&&) = default;
        Region& operator=(const Region&)  = default;
        Region& operator=(      Region&&) = default;
        // clang-format on

        Region(Uint32 _x, Uint32 _y, Uint32 _width, Uint32 _height) :
            // clang-format off
            x     {_x},
            y     {_y},
            width {_width},
            height{_height}
        // clang-format on
        {}

        bool IsEmpty() const
        {
            return width == 0 || height == 0;
        }

        constexpr bool operator==(const Region& rhs) const
        {
            // clang-format off
            return x      == rhs.x     &&
                   y      == rhs.y     &&
                   width  == rhs.width &&
                   height == rhs.height;
            // clang-format on
        }
        constexpr bool operator!=(const Region& rhs) const
        {
            return !(*this == rhs);
        }

        struct Hasher
        {
            size_t operator()(const Region& R) const
            {
                return ComputeHash(R.width, R.height, R.x, R.y);
            }
        };
    };

    DynamicAtlasManager(Uint32 Width, Uint32 Height);
    ~DynamicAtlasManager();

    // clang-format off
    DynamicAtlasManager             (const DynamicAtlasManager&)  = delete;
    DynamicAtlasManager& operator = (const DynamicAtlasManager&)  = delete;
    DynamicAtlasManager             (      DynamicAtlasManager&&) = default;
    DynamicAtlasManager& operator = (      DynamicAtlasManager&&) = delete;
    // clang-format on

    Region Allocate(Uint32 Width, Uint32 Height);
    void   Free(Region&& R);

    Uint32 GetFreeRegionCount() const
    {
        VERIFY_EXPR(m_FreeRegionsByWidth.size() == m_FreeRegionsByHeight.size());
        return static_cast<Uint32>(m_FreeRegionsByWidth.size());
    }

    Uint32 GetWidth() const { return m_Width; }
    Uint32 GetHeight() const { return m_Height; }
    Uint64 GetTotalFreeArea() const { return m_TotalFreeArea; }

    bool IsEmpty() const
    {
        VERIFY_EXPR(m_AllocatedRegions.empty() && (m_TotalFreeArea == Uint64{m_Width} * Uint64{m_Height}) ||
                    !m_AllocatedRegions.empty() && (m_TotalFreeArea < Uint64{m_Width} * Uint64{m_Height}));
        return m_AllocatedRegions.empty();
    }

#define CMP(Member)                 \
    if (R0.Member < R1.Member)      \
        return true;                \
    else if (R0.Member > R1.Member) \
        return false;

    struct WidthFirstCompare
    {
        bool operator()(const Region& R0, const Region& R1) const
        {
            CMP(width)
            CMP(height)
            CMP(x)
            CMP(y)
            return false;
        }
    };
    struct HeightFirstCompare
    {
        bool operator()(const Region& R0, const Region& R1) const
        {
            CMP(height)
            CMP(width)
            CMP(y)
            CMP(x)
            return false;
        }
    };
#undef CMP

private:
#if DILIGENT_DEBUG
    void DbgVerifyRegion(const Region& R) const;
    void DbgVerifyConsistency() const;
    struct Node;
    void DbgRecursiveVerifyConsistency(const Node& N, Uint32& Area) const;
#endif

    const Uint32 m_Width;
    const Uint32 m_Height;

    Uint64 m_TotalFreeArea = 0;

    struct Node
    {
        Region R;
        bool   IsAllocated = false;
        Node*  Parent      = nullptr;

        void Split(const std::initializer_list<Region>& Regions);
        bool CanMergeChildren() const;
        void MergeChildren();
        bool HasChildren() const
        {
            VERIFY_EXPR(NumChildren == 0 && !Children || NumChildren != 0 && Children);
            VERIFY(!IsAllocated || NumChildren == 0, "Allocated nodes can't have children");
            return NumChildren != 0;
        }

        const Node& Child(Uint32 i) const
        {
            VERIFY_EXPR(i < NumChildren);
            return Children[i];
        }
        Node& Child(Uint32 i)
        {
            VERIFY_EXPR(i < NumChildren);
            return Children[i];
        }

        template <typename ProcessChildType>
        void ProcessChildren(ProcessChildType ProcessChild) const
        {
            for (Uint32 i = 0; i < NumChildren; ++i)
                ProcessChild(Child(i));
        }
        template <typename ProcessChildType>
        void ProcessChildren(ProcessChildType ProcessChild)
        {
            for (Uint32 i = 0; i < NumChildren; ++i)
                ProcessChild(Child(i));
        }

#if DILIGENT_DEBUG
        void Validate() const;
#endif
    private:
        Uint32                  NumChildren = 0;
        std::unique_ptr<Node[]> Children;
    };
    std::unique_ptr<Node> m_Root{new Node};

    void RegisterNode(Node& N);
    void UnregisterNode(const Node& N);

    // Free regions ordered by width->height->x->y
    std::map<Region, Node*, WidthFirstCompare> m_FreeRegionsByWidth;
    // Free regions ordered by height->width->y->x
    std::map<Region, Node*, HeightFirstCompare> m_FreeRegionsByHeight;
    // Allocated regions
    std::unordered_map<Region, Node*, Region::Hasher> m_AllocatedRegions;
};

} // namespace Diligent
