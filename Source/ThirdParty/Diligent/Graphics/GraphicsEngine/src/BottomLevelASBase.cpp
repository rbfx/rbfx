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

#include "BottomLevelASBase.hpp"

namespace Diligent
{

void ValidateBottomLevelASDesc(const BottomLevelASDesc& Desc) noexcept(false)
{
#define LOG_BLAS_ERROR_AND_THROW(...) LOG_ERROR_AND_THROW("Description of a bottom-level AS '", (Desc.Name ? Desc.Name : ""), "' is invalid: ", ##__VA_ARGS__)

    if (Desc.CompactedSize > 0)
    {
        if (Desc.pTriangles != nullptr || Desc.pBoxes != nullptr)
            LOG_BLAS_ERROR_AND_THROW("If non-zero CompactedSize is specified, pTriangles and pBoxes must both be null.");

        if (Desc.Flags != RAYTRACING_BUILD_AS_NONE)
            LOG_BLAS_ERROR_AND_THROW("If non-zero CompactedSize is specified, Flags must be RAYTRACING_BUILD_AS_NONE.");
    }
    else
    {
        if (!((Desc.BoxCount != 0) ^ (Desc.TriangleCount != 0)))
            LOG_BLAS_ERROR_AND_THROW("Exactly one of BoxCount (", Desc.BoxCount, ") and TriangleCount (", Desc.TriangleCount, ") must be non-zero.");

        if (Desc.pBoxes == nullptr && Desc.BoxCount > 0)
            LOG_BLAS_ERROR_AND_THROW("BoxCount is ", Desc.BoxCount, ", but pBoxes is null.");

        if (Desc.pTriangles == nullptr && Desc.TriangleCount > 0)
            LOG_BLAS_ERROR_AND_THROW("TriangleCount is ", Desc.TriangleCount, ", but pTriangles is null.");

        if ((Desc.Flags & RAYTRACING_BUILD_AS_PREFER_FAST_TRACE) != 0 && (Desc.Flags & RAYTRACING_BUILD_AS_PREFER_FAST_BUILD) != 0)
            LOG_BLAS_ERROR_AND_THROW("RAYTRACING_BUILD_AS_PREFER_FAST_TRACE and RAYTRACING_BUILD_AS_PREFER_FAST_BUILD flags are mutually exclusive.");

        for (Uint32 i = 0; i < Desc.TriangleCount; ++i)
        {
            const auto& tri = Desc.pTriangles[i];

            if (tri.GeometryName == nullptr)
                LOG_BLAS_ERROR_AND_THROW("pTriangles[", i, "].GeometryName must not be null.");

            if (tri.VertexValueType != VT_FLOAT32 && tri.VertexValueType != VT_FLOAT16 && tri.VertexValueType != VT_INT16)
                LOG_BLAS_ERROR_AND_THROW("pTriangles[", i, "].VertexValueType (", GetValueTypeString(tri.VertexValueType),
                                         ") is invalid. Only the following values are allowed: VT_FLOAT32, VT_FLOAT16, VT_INT16.");

            if (tri.VertexComponentCount != 2 && tri.VertexComponentCount != 3)
                LOG_BLAS_ERROR_AND_THROW("pTriangles[", i, "].VertexComponentCount (", Uint32{tri.VertexComponentCount}, ") is invalid. Only 2 or 3 are allowed.");

            if (tri.MaxVertexCount == 0)
                LOG_BLAS_ERROR_AND_THROW("pTriangles[", i, "].MaxVertexCount must be greater than 0.");

            if (tri.MaxPrimitiveCount == 0)
                LOG_BLAS_ERROR_AND_THROW("pTriangles[", i, "].MaxPrimitiveCount must be greater than 0.");

            if (tri.IndexType == VT_UNDEFINED)
            {
                if (tri.MaxVertexCount != tri.MaxPrimitiveCount * 3)
                    LOG_BLAS_ERROR_AND_THROW("pTriangles[", i, "].MaxVertexCount (", tri.MaxVertexCount,
                                             ") must be equal to MaxPrimitiveCount * 3 (", tri.MaxPrimitiveCount * 3, ").");
            }
            else
            {
                if (tri.IndexType != VT_UINT32 && tri.IndexType != VT_UINT16)
                    LOG_BLAS_ERROR_AND_THROW("pTriangles[", i, "].IndexType (", GetValueTypeString(tri.VertexValueType),
                                             ") must be VT_UINT16 or VT_UINT32.");
            }
        }

        for (Uint32 i = 0; i < Desc.BoxCount; ++i)
        {
            const auto& box = Desc.pBoxes[i];

            if (box.GeometryName == nullptr)
                LOG_BLAS_ERROR_AND_THROW("pBoxes[", i, "].GeometryName must not be null.");

            if (box.MaxBoxCount == 0)
                LOG_BLAS_ERROR_AND_THROW("pBoxes[", i, "].MaxBoxCount must be greater than 0.");
        }
    }

#undef LOG_BLAS_ERROR_AND_THROW
}

void CopyBLASGeometryDesc(const BottomLevelASDesc& SrcDesc,
                          BottomLevelASDesc&       DstDesc,
                          FixedLinearAllocator&    MemPool,
                          const BLASNameToIndex*   pSrcNameToIndex,
                          BLASNameToIndex&         DstNameToIndex) noexcept(false)
{
    if (SrcDesc.pTriangles != nullptr)
    {
        MemPool.AddSpace<decltype(*SrcDesc.pTriangles)>(SrcDesc.TriangleCount);

        for (Uint32 i = 0; i < SrcDesc.TriangleCount; ++i)
            MemPool.AddSpaceForString(SrcDesc.pTriangles[i].GeometryName);

        MemPool.Reserve();

        auto* pTriangles = MemPool.CopyArray(SrcDesc.pTriangles, SrcDesc.TriangleCount);

        // Copy strings
        for (Uint32 i = 0; i < SrcDesc.TriangleCount; ++i)
        {
            const auto* GeoName        = MemPool.CopyString(SrcDesc.pTriangles[i].GeometryName);
            pTriangles[i].GeometryName = GeoName;

            Uint32 ActualIndex = INVALID_INDEX;
            if (pSrcNameToIndex)
            {
                auto iter = pSrcNameToIndex->find(GeoName);
                VERIFY_EXPR(iter != pSrcNameToIndex->end());
                ActualIndex = iter->second.ActualIndex;
            }

            bool IsUniqueName = DstNameToIndex.emplace(GeoName, BLASGeomIndex{i, ActualIndex}).second;
            if (!IsUniqueName)
                LOG_ERROR_AND_THROW("Geometry name '", GeoName, "' is not unique");
        }

        DstDesc.pTriangles    = pTriangles;
        DstDesc.TriangleCount = SrcDesc.TriangleCount;
        DstDesc.pBoxes        = nullptr;
        DstDesc.BoxCount      = 0;
    }
    else if (SrcDesc.pBoxes != nullptr)
    {
        MemPool.AddSpace<decltype(*SrcDesc.pBoxes)>(SrcDesc.BoxCount);

        for (Uint32 i = 0; i < SrcDesc.BoxCount; ++i)
            MemPool.AddSpaceForString(SrcDesc.pBoxes[i].GeometryName);

        MemPool.Reserve();

        auto* pBoxes = MemPool.CopyArray(SrcDesc.pBoxes, SrcDesc.BoxCount);

        // Copy strings
        for (Uint32 i = 0; i < SrcDesc.BoxCount; ++i)
        {
            const auto* GeoName    = MemPool.CopyString(SrcDesc.pBoxes[i].GeometryName);
            pBoxes[i].GeometryName = GeoName;

            Uint32 ActualIndex = INVALID_INDEX;
            if (pSrcNameToIndex)
            {
                auto iter = pSrcNameToIndex->find(GeoName);
                VERIFY_EXPR(iter != pSrcNameToIndex->end());
                ActualIndex = iter->second.ActualIndex;
            }

            bool IsUniqueName = DstNameToIndex.emplace(GeoName, BLASGeomIndex{i, ActualIndex}).second;
            if (!IsUniqueName)
                LOG_ERROR_AND_THROW("Geometry name '", GeoName, "' is not unique");
        }

        DstDesc.pBoxes        = pBoxes;
        DstDesc.BoxCount      = SrcDesc.BoxCount;
        DstDesc.pTriangles    = nullptr;
        DstDesc.TriangleCount = 0;
    }
    else
    {
        LOG_ERROR_AND_THROW("Either pTriangles or pBoxes must not be null");
    }
}

} // namespace Diligent
