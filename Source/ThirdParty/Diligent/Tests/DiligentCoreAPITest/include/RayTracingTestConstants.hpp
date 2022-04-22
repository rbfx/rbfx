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

#include "BasicMath.hpp"

namespace Diligent
{

namespace TestingConstants
{

// clang-format off
    namespace TriangleClosestHit
    {
        static const float4 Vertices[] =
        {
            float4{0.25f, 0.25f, 0.0f, 0.0f},
            float4{0.75f, 0.25f, 0.0f, 0.0f},
            float4{0.50f, 0.75f, 0.0f, 0.0f}
        };
        static const Uint32 Indices[] =
        {
            0, 1, 2
        };
    } // namespace TriangleClosestHit

    namespace TriangleAnyHit
    {
        static const float3 Vertices[] =
        {
            float3{0.25f, 0.25f, 0.0f}, float3{0.75f, 0.25f, 0.0f}, float3{0.50f, 0.75f, 0.0f},
            float3{0.50f, 0.10f, 0.1f}, float3{0.90f, 0.90f, 0.1f}, float3{0.10f, 0.90f, 0.1f},
            float3{0.40f, 1.00f, 0.2f}, float3{0.20f, 0.40f, 0.2f}, float3{1.00f, 0.70f, 0.2f}
        };
    } // namespace TriangleAnyHit

    namespace ProceduralIntersection
    {
        static const float3 Boxes[] =
        {
            float3{0.25f, 0.5f, 2.0f} - float3{1.0f, 1.0f, 1.0f},
            float3{0.25f, 0.5f, 2.0f} + float3{1.0f, 1.0f, 1.0f}
        };
    } // namespace ProceduralIntersection

    namespace MultiGeometry
    {
        struct VertexType
        {
            float4 Pos;
            float4 Color1;
            float4 Color2;

            VertexType(float2 _Pos, float3 _Color1, float3 _Color2) :
                Pos   {_Pos.x,    _Pos.y,    2.0f,      1.0f},
                Color1{_Color1.x, _Color1.y, _Color1.z, 1.0f},
                Color2{_Color2.x, _Color2.y, _Color2.z, 1.0f}
            {}
        };

        static const VertexType Vertices[] =
        {
            // geometry 1
            VertexType{{0.10f, 0.10f}, {0.7f, 0.3f, 0.1f}, {0.2f, 0.9f, 0.4f}}, //  0
            VertexType{{0.17f, 0.30f}, {0.6f, 0.0f, 0.4f}, {0.2f, 0.5f, 0.8f}}, //  1
            VertexType{{0.10f, 0.31f}, {0.3f, 0.7f, 0.4f}, {0.9f, 0.2f, 0.6f}}, //  2
            VertexType{{0.22f, 0.45f}, {0.2f, 0.9f, 0.7f}, {0.1f, 0.7f, 0.1f}}, //  3
            // geometry 2
            VertexType{{0.27f, 0.10f}, {0.5f, 0.1f, 0.6f}, {0.3f, 0.1f, 0.5f}}, //  4
            VertexType{{0.40f, 0.30f}, {1.0f, 1.0f, 1.0f}, {0.3f, 1.0f, 0.7f}}, //  5
            VertexType{{0.26f, 0.30f}, {0.3f, 0.3f, 0.9f}, {1.0f, 0.0f, 0.3f}}, //  6
            VertexType{{0.40f, 0.47f}, {0.8f, 1.0f, 0.2f}, {1.0f, 0.7f, 0.0f}}, //  7
            VertexType{{0.54f, 0.30f}, {0.1f, 1.0f, 0.9f}, {0.0f, 1.0f, 0.6f}}, //  8
            VertexType{{0.53f, 0.10f}, {1.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}}, //  9
            // geometry 3
            VertexType{{0.65f, 0.10f}, {0.3f, 0.6f, 0.8f}, {1.0f, 0.9f, 0.2f}}, // 10
            VertexType{{0.63f, 0.25f}, {0.9f, 1.0f, 0.2f}, {0.1f, 0.2f, 0.3f}}, // 11
            VertexType{{0.82f, 0.20f}, {0.4f, 0.5f, 0.0f}, {1.0f, 0.2f, 0.6f}}, // 12
            VertexType{{0.76f, 0.30f}, {1.0f, 0.0f, 0.0f}, {0.4f, 0.7f, 0.2f}}, // 13
            VertexType{{0.55f, 0.48f}, {0.5f, 0.1f, 0.2f}, {1.0f, 0.3f, 0.5f}}, // 14
            VertexType{{0.90f, 0.40f}, {0.8f, 0.2f, 1.0f}, {0.3f, 0.6f, 0.4f}}, // 15
        };
        static const uint Indices[] =
        {
             0,  1,  2,   2,  1,  3,                           // geometry 1
             4,  5,  6,   6,  7,  8,   8,  5,  9,              // geometry 2
            10, 12, 11,  11, 12, 13,  11, 13, 14,  13, 12, 15, // geometry 3
        };
        static const uint4 Primitives[] =
        {
            // geometry 1
            {Indices[ 0], Indices[ 1], Indices[ 2], 0}, // 0
            {Indices[ 3], Indices[ 4], Indices[ 5], 0}, // 1
            // geometry 2
            {Indices[ 6], Indices[ 7], Indices[ 8], 0}, // 2
            {Indices[ 9], Indices[10], Indices[11], 0}, // 3
            {Indices[12], Indices[13], Indices[14], 0}, // 4
            // geometry 3
            {Indices[15], Indices[16], Indices[17], 0}, // 5
            {Indices[18], Indices[19], Indices[20], 0}, // 6
            {Indices[21], Indices[22], Indices[23], 0}, // 7
            {Indices[24], Indices[25], Indices[26], 0}  // 8
        };
        static const uint PrimitiveOffsets[] =
        {
            0, 2, 5
        };

        struct ShaderRecord
        {
            const float4 Weight;
            const uint   GeometryID;

            uint   padding[3];

            ShaderRecord(const float4& w, uint id): Weight{w}, GeometryID{id} {}
        };
        static const ShaderRecord Weights[] =
        {
            ShaderRecord{{1.0f, 0.4f, 0.4f, 1.0f}, 0},
            ShaderRecord{{0.4f, 1.0f, 0.4f, 1.0f}, 1},
            ShaderRecord{{0.4f, 0.4f, 1.0f, 1.0f}, 2},
            ShaderRecord{{0.4f, 1.0f, 0.4f, 1.0f}, 0},
            ShaderRecord{{0.4f, 0.4f, 1.0f, 1.0f}, 1},
            ShaderRecord{{1.0f, 0.4f, 0.4f, 1.0f}, 2},
        };
        static constexpr Uint32 ShaderRecordSize = sizeof(Weights[0]);
        static constexpr Uint32 InstanceCount    = 2;

        static_assert(_countof(Vertices) == 16, "Update array size in shaders");
        static_assert(_countof(PrimitiveOffsets) == 3, "Update array size in shaders");
        static_assert(_countof(Primitives) == 9, "Update array size in shaders");
        static_assert(_countof(Indices) % 3 == 0, "Invalid index count");
        static_assert(_countof(Indices) / 3 == _countof(Primitives), "Primitive count mismatch");
        static_assert(ShaderRecordSize == 32, "must be 32 bytes");

    } // namespace MultiGeometry

// clang-format on

} // namespace TestingConstants

} // namespace Diligent
