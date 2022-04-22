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

#include "DynamicAtlasManager.hpp"

#include <array>
#include <algorithm>

#include "gtest/gtest.h"

#include "FastRand.hpp"

using namespace Diligent;

namespace Diligent
{

static std::ostream& operator<<(std::ostream& os, const DynamicAtlasManager::Region& R)
{
    return os << "[" << R.x << ", " << R.x + R.width << ") x [" << R.y << ", " << R.y + R.height << ")";
}

} // namespace Diligent

namespace
{

using Region = DynamicAtlasManager::Region;

TEST(GraphicsAccessories_DynamicAtlasManager, Region_Ctor)
{
    Region Empty;
    EXPECT_EQ(Empty.x, 0u);
    EXPECT_EQ(Empty.y, 0u);
    EXPECT_EQ(Empty.width, 0u);
    EXPECT_EQ(Empty.height, 0u);

    Region R{1, 2, 15, 35};
    EXPECT_EQ(R.x, 1u);
    EXPECT_EQ(R.y, 2u);
    EXPECT_EQ(R.width, 15u);
    EXPECT_EQ(R.height, 35u);
}

TEST(GraphicsAccessories_DynamicAtlasManager, Region_OpEqual)
{
    EXPECT_EQ(Region(1, 2, 3, 4), Region(1, 2, 3, 4));
    EXPECT_NE(Region(0, 2, 3, 4), Region(1, 2, 3, 4));
    EXPECT_NE(Region(1, 0, 3, 4), Region(1, 2, 3, 4));
    EXPECT_NE(Region(1, 2, 0, 4), Region(1, 2, 3, 4));
    EXPECT_NE(Region(1, 2, 3, 0), Region(1, 2, 3, 4));
}

TEST(GraphicsAccessories_DynamicAtlasManager, Region_WidthFirstCompare)
{
    DynamicAtlasManager::WidthFirstCompare Less;
    // clang-format off
    EXPECT_FALSE(Less(Region{ 0,  0, 11,  0},  Region{ 0, 0, 10,  0}));
    EXPECT_FALSE(Less(Region{ 0,  0, 10,  0},  Region{ 0, 0, 10,  0}));
    EXPECT_TRUE (Less(Region{ 0,  0, 10,  0},  Region{ 0, 0, 11,  0}));
    EXPECT_TRUE (Less(Region{ 0,  0, 10, 15},  Region{ 0, 0, 11,  0}));
    EXPECT_TRUE (Less(Region{23,  0, 10, 15},  Region{ 0, 0, 11,  0}));
    EXPECT_TRUE (Less(Region{23, 37, 10, 15},  Region{ 0, 0, 11,  0}));

    EXPECT_FALSE(Less(Region{ 0,  0, 20, 16},  Region{ 0, 0, 20, 15}));
    EXPECT_FALSE(Less(Region{ 0,  0, 20, 15},  Region{ 0, 0, 20, 15}));
    EXPECT_TRUE (Less(Region{ 0,  0, 20, 15},  Region{ 0, 0, 20, 16}));
    EXPECT_TRUE (Less(Region{24,  0, 20, 15},  Region{ 0, 0, 20, 16}));
    EXPECT_TRUE (Less(Region{24, 48, 20, 15},  Region{ 0, 0, 20, 16}));

    EXPECT_FALSE(Less(Region{26,  0, 20, 16},  Region{25, 0, 20, 16}));
    EXPECT_FALSE(Less(Region{25,  0, 20, 16},  Region{25, 0, 20, 16}));
    EXPECT_TRUE (Less(Region{25,  0, 20, 16},  Region{26, 0, 20, 16}));
    EXPECT_TRUE (Less(Region{25, 99, 20, 16},  Region{26, 0, 20, 16}));

    EXPECT_FALSE(Less(Region{26, 61, 20, 16},  Region{26, 60, 20, 16}));
    EXPECT_FALSE(Less(Region{26, 60, 20, 16},  Region{26, 60, 20, 16}));
    EXPECT_TRUE (Less(Region{26, 60, 20, 16},  Region{26, 61, 20, 16}));
    // clang-format on

    EXPECT_FALSE(Less(Region{1, 2, 10, 20}, Region{1, 2, 10, 20}));
}

TEST(GraphicsAccessories_DynamicAtlasManager, Region_HeightFirstCompare)
{
    DynamicAtlasManager::HeightFirstCompare Less;

    // clang-format off
    EXPECT_FALSE(Less(Region{ 0,  0,  0, 11},  Region{ 0, 0,  0, 10}));
    EXPECT_FALSE(Less(Region{ 0,  0,  0, 10},  Region{ 0, 0,  0, 10}));
    EXPECT_TRUE (Less(Region{ 0,  0,  0, 10},  Region{ 0, 0,  0, 11}));
    EXPECT_TRUE (Less(Region{ 0,  0, 15, 10},  Region{ 0, 0,  0, 11}));
    EXPECT_TRUE (Less(Region{ 0, 23, 15, 10},  Region{ 0, 0,  0, 11}));
    EXPECT_TRUE (Less(Region{37, 23, 15, 10},  Region{ 0, 0,  0, 11}));

    EXPECT_FALSE(Less(Region{ 0,  0, 16, 20},  Region{ 0, 0, 15, 20}));
    EXPECT_FALSE(Less(Region{ 0,  0, 15, 20},  Region{ 0, 0, 15, 20}));
    EXPECT_TRUE (Less(Region{ 0,  0, 15, 20},  Region{ 0, 0, 16, 20}));
    EXPECT_TRUE (Less(Region{ 0, 24, 15, 20},  Region{ 0, 0, 16, 20}));
    EXPECT_TRUE (Less(Region{48, 24, 15, 20},  Region{ 0, 0, 16, 20}));

    EXPECT_FALSE(Less(Region{ 0, 26, 16, 20},  Region{0, 25, 16, 20}));
    EXPECT_FALSE(Less(Region{ 0, 25, 16, 20},  Region{0, 25, 16, 20}));
    EXPECT_TRUE (Less(Region{ 0, 25, 16, 20},  Region{0, 26, 16, 20}));
    EXPECT_TRUE (Less(Region{99, 25, 16, 20},  Region{0, 26, 16, 20}));

    EXPECT_FALSE(Less(Region{61, 26, 16, 20},  Region{60, 26, 16, 20}));
    EXPECT_FALSE(Less(Region{60, 26, 16, 20},  Region{60, 26, 16, 20}));
    EXPECT_TRUE (Less(Region{60, 26, 16, 20},  Region{61, 26, 16, 20}));
    // clang-format on

    EXPECT_FALSE(Less(Region{1, 2, 10, 20}, Region{1, 2, 10, 20}));
}

TEST(GraphicsAccessories_DynamicAtlasManager, Region_Hasher)
{
    auto H = Region::Hasher{};

    EXPECT_NE(H(Region{0, 2, 3, 4}), H(Region{1, 2, 3, 4}));
    EXPECT_NE(H(Region{1, 0, 3, 4}), H(Region{1, 2, 3, 4}));
    EXPECT_NE(H(Region{1, 2, 0, 4}), H(Region{1, 2, 3, 4}));
    EXPECT_NE(H(Region{1, 2, 3, 0}), H(Region{1, 2, 3, 4}));
}

TEST(GraphicsAccessories_DynamicAtlasManager, Empty)
{
    DynamicAtlasManager Mgr{16, 8};
    EXPECT_TRUE(Mgr.IsEmpty());
}

TEST(GraphicsAccessories_DynamicAtlasManager, Move)
{
    DynamicAtlasManager Mgr0{16, 8};

    auto R = Mgr0.Allocate(16, 8);

    DynamicAtlasManager Mgr1{std::move(Mgr0)};
    Mgr1.Free(std::move(R));
}

TEST(GraphicsAccessories_DynamicAtlasManager, Allocate)
{
    {
        DynamicAtlasManager Mgr{16, 8};
        EXPECT_TRUE(Mgr.IsEmpty());

        auto R = Mgr.Allocate(16, 8);
        EXPECT_FALSE(Mgr.IsEmpty());
        Mgr.Free(std::move(R));
        EXPECT_TRUE(Mgr.IsEmpty());
    }

    {
        DynamicAtlasManager Mgr{16, 16};

        auto R = Mgr.Allocate(8, 16);
        Mgr.Free(std::move(R));
    }

    {
        DynamicAtlasManager Mgr{16, 16};

        auto R = Mgr.Allocate(16, 8);
        Mgr.Free(std::move(R));
    }

    {
        DynamicAtlasManager Mgr{20, 16};

        auto R = Mgr.Allocate(16, 8);
        Mgr.Free(std::move(R));
    }

    {
        DynamicAtlasManager Mgr{16, 20};

        auto R = Mgr.Allocate(12, 8);
        Mgr.Free(std::move(R));
    }

    for (Uint32 i = 0; i < 2; ++i)
    {
        static constexpr size_t N = 5;

        std::array<Uint32, N> ids;
        for (Uint32 id = 0; id < ids.size(); ++id)
            ids[id] = id;

        const std::array<std::pair<Uint32, Uint32>, N> RegionSizes = //
            {
                std::make_pair(Uint32{4}, Uint32{8}),
                std::make_pair(Uint32{12}, Uint32{6}),
                std::make_pair(Uint32{10}, Uint32{10}),
                std::make_pair(Uint32{2}, Uint32{12}),
                std::make_pair(Uint32{5}, Uint32{1}) //
            };

        do
        {
            Uint32 AtlasWidth  = 16;
            Uint32 AtlasHeight = 20;
            if (i == 1)
                std::swap(AtlasWidth, AtlasHeight);

            DynamicAtlasManager Mgr{AtlasWidth, AtlasHeight};

            std::array<Region, N> Rs;
            for (Uint32 r = 0; r < N; ++r)
            {
                auto w = RegionSizes[r].first;
                auto h = RegionSizes[r].second;
                if (i == 1)
                    std::swap(w, h);
                Rs[r] = Mgr.Allocate(w, h);
            }

            for (auto id : ids)
                Mgr.Free(std::move(Rs[id]));

        } while (std::next_permutation(ids.begin(), ids.end()));
    }

#if 0
    // Test merging regions while splitting free space
    {
        DynamicAtlasManager Mgr{64, 64};

        auto R0 = Mgr.Allocate(64, 16);
        auto R1 = Mgr.Allocate(16, 48);
        EXPECT_EQ(R0, Region(0, 0, 64, 16));
        EXPECT_EQ(R1, Region(0, 16, 16, 48));
        Mgr.Free(std::move(R0));
        //  ____________________
        // |    |               |
        // |    |               |
        // | R1 |     Free      |
        // |    |               |
        // |    |               |
        // |____|_______________|
        // |        Free        |
        // |____________________|
        EXPECT_EQ(Mgr.GetFreeRegionCount(), 2U);


        auto R2 = Mgr.Allocate(16, 16);
        //  ____________________
        // |    |               |
        // |    |               |
        // | R1 |               |
        // |    |     Free      |
        // |    |               |
        // |____|               |
        // | R2 |               |
        // |____|_______________|
        EXPECT_EQ(Mgr.GetFreeRegionCount(), 1U);
        EXPECT_EQ(R2, Region(0, 0, 16, 16));

        auto R3 = Mgr.Allocate(48, 64);
        EXPECT_FALSE(R3.IsEmpty());

        Mgr.Free(std::move(R3));
        Mgr.Free(std::move(R1));
        Mgr.Free(std::move(R2));
        EXPECT_EQ(Mgr.GetFreeRegionCount(), 1U);
    }

    {
        DynamicAtlasManager Mgr{64, 64};

        auto R0 = Mgr.Allocate(16, 64);
        auto R1 = Mgr.Allocate(48, 16);
        EXPECT_EQ(R0, Region(0, 0, 16, 64));
        EXPECT_EQ(R1, Region(16, 0, 48, 16));
        Mgr.Free(std::move(R0));
        //  ____________________
        // |    |               |
        // |    |               |
        // | F  |     Free      |
        // | r  |               |
        // | e  |               |
        // | e  |_______________|
        // |    |      R1       |
        // |____|_______________|
        EXPECT_EQ(Mgr.GetFreeRegionCount(), 2U);

        auto R2 = Mgr.Allocate(16, 16);
        //  ____________________
        // |                    |
        // |                    |
        // |       Free         |
        // |                    |
        // |                    |
        // |____________________|
        // | R2 |      R1       |
        // |____|_______________|
        EXPECT_EQ(Mgr.GetFreeRegionCount(), 1U);
        EXPECT_EQ(R2, Region(0, 0, 16, 16));

        auto R3 = Mgr.Allocate(64, 48);
        EXPECT_FALSE(R3.IsEmpty());

        Mgr.Free(std::move(R3));
        Mgr.Free(std::move(R1));
        Mgr.Free(std::move(R2));
        EXPECT_EQ(Mgr.GetFreeRegionCount(), 1U);
    }

    {
        DynamicAtlasManager Mgr{64, 32};

        auto R0 = Mgr.Allocate(64, 24);
        auto R1 = Mgr.Allocate(8, 4);
        auto R2 = Mgr.Allocate(8, 4);
        EXPECT_EQ(R0, Region(0, 0, 64, 24));
        EXPECT_EQ(R1, Region(0, 24, 8, 4));
        EXPECT_EQ(R2, Region(0, 28, 8, 4));
        Mgr.Free(std::move(R0));
        Mgr.Free(std::move(R1));
        //  _________________________________
        // |  R2  |                          |
        // |______|        Free              |
        // | Free |                          |
        // |______|__________________________|
        // |                                 |
        // |                                 |
        // |            Free                 |
        // |                                 |
        // |                                 |
        // |_________________________________|

        EXPECT_EQ(Mgr.GetFreeRegionCount(), 3U);
        auto R3 = Mgr.Allocate(8, 16);
        //  _________________________________
        // |  R2  |                          |
        // |______|                          |
        // |      |                          |
        // | Free |                          |
        // |      |         Free             |
        // |______|                          |
        // |      |                          |
        // |  R3  |                          |
        // |      |                          |
        // |______|__________________________|
        EXPECT_EQ(Mgr.GetFreeRegionCount(), 2U);
        EXPECT_EQ(R3, Region(0, 0, 8, 16));

        auto R4 = Mgr.Allocate(56, 32);
        EXPECT_FALSE(R4.IsEmpty());

        auto R5 = Mgr.Allocate(8, 12);
        EXPECT_FALSE(R5.IsEmpty());

        Mgr.Free(std::move(R4));
        Mgr.Free(std::move(R2));
        Mgr.Free(std::move(R3));
        Mgr.Free(std::move(R5));
        EXPECT_EQ(Mgr.GetFreeRegionCount(), 1U);
    }

    {
        DynamicAtlasManager Mgr{32, 64};

        auto R0 = Mgr.Allocate(24, 64);
        auto R1 = Mgr.Allocate(4, 8);
        auto R2 = Mgr.Allocate(4, 8);
        EXPECT_EQ(R0, Region(0, 0, 24, 64));
        EXPECT_EQ(R1, Region(24, 0, 4, 8));
        EXPECT_EQ(R2, Region(28, 0, 4, 8));
        Mgr.Free(std::move(R0));
        Mgr.Free(std::move(R1));
        //  __________________________
        // |              |           |
        // |              |           |
        // |              |           |
        // |              |           |
        // |              |    F      |
        // |      F       |    R      |
        // |      R       |    E      |
        // |      E       |    E      |
        // |      E       |           |
        // |              |           |
        // |              |_____ _____|
        // |              |  F  |     |
        // |              |  R  | R2  |
        // |              |  E  |     |
        // |______________|__E__|_____|

        EXPECT_EQ(Mgr.GetFreeRegionCount(), 3U);
        auto R3 = Mgr.Allocate(16, 8);
        //  __________________________
        // |                          |
        // |                          |
        // |                          |
        // |                          |
        // |                          |
        // |         Free             |
        // |                          |
        // |                          |
        // |                          |
        // |                          |
        // |_______ ____________ _____|
        // |       |            |     |
        // |  R3   |    Free    | R2  |
        // |       |            |     |
        // |_______|____________|_____|
        EXPECT_EQ(Mgr.GetFreeRegionCount(), 2U);
        EXPECT_EQ(R3, Region(0, 0, 16, 8));

        auto R4 = Mgr.Allocate(32, 56);
        EXPECT_FALSE(R4.IsEmpty());

        auto R5 = Mgr.Allocate(12, 8);
        EXPECT_FALSE(R5.IsEmpty());

        Mgr.Free(std::move(R4));
        Mgr.Free(std::move(R2));
        Mgr.Free(std::move(R3));
        Mgr.Free(std::move(R5));
        EXPECT_EQ(Mgr.GetFreeRegionCount(), 1U);
    }
#endif
}

TEST(GraphicsAccessories_DynamicAtlasManager, AllocateRandom)
{
    DynamicAtlasManager Mgr{256, 256};
    const Uint32        NumIterations = 10;
    for (Uint32 i = 0; i < NumIterations; ++i)
    {
        FastRandInt         rnd{static_cast<unsigned int>(i), 1, 16};
        std::vector<Region> Regions(i * 8);
        for (auto& R : Regions)
        {
            R = Mgr.Allocate(rnd(), rnd());
        }
        for (auto& R : Regions)
        {
            if (!R.IsEmpty())
                Mgr.Free(std::move(R));
        }
    }
}

} // namespace
