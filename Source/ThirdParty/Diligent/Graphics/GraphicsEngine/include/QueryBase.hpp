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
/// Implementation of Diligent::QueryBase template class

#include "Query.h"
#include "DeviceObjectBase.hpp"
#include "GraphicsTypes.h"
#include "RefCntAutoPtr.hpp"

namespace Diligent
{

/// Template class implementing base functionality of the query object

/// \tparam EngineImplTraits - Engine implementation type traits.
template <typename EngineImplTraits>
class QueryBase : public DeviceObjectBase<typename EngineImplTraits::QueryInterface, typename EngineImplTraits::RenderDeviceImplType, QueryDesc>
{
public:
    // Base interface this class inherits (IQueryD3D12, IQueryVk, etc.)
    using BaseInterface = typename EngineImplTraits::QueryInterface;

    // Render device implementation type (RenderDeviceD3D12Impl, RenderDeviceVkImpl, etc.).
    using RenderDeviceImplType = typename EngineImplTraits::RenderDeviceImplType;

    // Device context implementation type (DeviceContextD3D12Impl, DeviceContextVkImpl, etc.).
    using DeviceContextImplType = typename EngineImplTraits::DeviceContextImplType;

    enum class QueryState
    {
        Inactive,
        Querying,
        Ended
    };

    using TDeviceObjectBase = DeviceObjectBase<BaseInterface, RenderDeviceImplType, QueryDesc>;

    /// \param pRefCounters      - Reference counters object that controls the lifetime of this command list.
    /// \param pDevice           - Pointer to the device.
    /// \param Desc              - Query description
    /// \param bIsDeviceInternal - Flag indicating if the Query is an internal device object and
    ///							   must not keep a strong reference to the device.
    QueryBase(IReferenceCounters*   pRefCounters,
              RenderDeviceImplType* pDevice,
              const QueryDesc&      Desc,
              bool                  bIsDeviceInternal = false) :
        TDeviceObjectBase{pRefCounters, pDevice, Desc, bIsDeviceInternal}
    {
        const auto& deviceFeatures = this->GetDevice()->GetFeatures();
        static_assert(QUERY_TYPE_NUM_TYPES == 6, "Not all QUERY_TYPE enum values are handled below");
        switch (Desc.Type)
        {
            case QUERY_TYPE_OCCLUSION:
                if (!deviceFeatures.OcclusionQueries)
                    LOG_ERROR_AND_THROW("Occlusion queries are not supported by this device");
                break;

            case QUERY_TYPE_BINARY_OCCLUSION:
                if (!deviceFeatures.BinaryOcclusionQueries)
                    LOG_ERROR_AND_THROW("Binary occlusion queries are not supported by this device");
                break;

            case QUERY_TYPE_TIMESTAMP:
                if (!deviceFeatures.TimestampQueries)
                    LOG_ERROR_AND_THROW("Timestamp queries are not supported by this device");
                break;

            case QUERY_TYPE_PIPELINE_STATISTICS:
                if (!deviceFeatures.PipelineStatisticsQueries)
                    LOG_ERROR_AND_THROW("Pipeline statistics queries are not supported by this device");
                break;

            case QUERY_TYPE_DURATION:
                if (!deviceFeatures.DurationQueries)
                    LOG_ERROR_AND_THROW("Duration queries are not supported by this device");
                break;

            default:
                UNEXPECTED("Unexpected query type");
        }
    }

    ~QueryBase()
    {
        if (m_State == QueryState::Querying)
        {
            LOG_ERROR_MESSAGE("Destroying query '", this->m_Desc.Name,
                              "' that is in querying state. End the query before releasing it.");
        }
    }

    IMPLEMENT_QUERY_INTERFACE_IN_PLACE(IID_Query, TDeviceObjectBase)

    virtual void DILIGENT_CALL_TYPE Invalidate() override
    {
        m_State = QueryState::Inactive;
    }

    void OnBeginQuery(DeviceContextImplType* pContext)
    {
        DEV_CHECK_ERR(this->m_Desc.Type != QUERY_TYPE_TIMESTAMP,
                      "BeginQuery cannot be called on timestamp query '", this->m_Desc.Name,
                      "'. Call EndQuery to set the timestamp.");

        DEV_CHECK_ERR(m_State != QueryState::Querying,
                      "Attempting to begin query '", this->m_Desc.Name,
                      "' twice. A query must be ended before it can be begun again.");

        if (m_pContext != nullptr && m_pContext != pContext)
            Invalidate();

        m_pContext = pContext;
        m_State    = QueryState::Querying;
    }

    void OnEndQuery(DeviceContextImplType* pContext)
    {
        if (this->m_Desc.Type != QUERY_TYPE_TIMESTAMP)
        {
            DEV_CHECK_ERR(m_State == QueryState::Querying && m_pContext != nullptr,
                          "Attempting to end query '", this->m_Desc.Name, "' that has not been begun.");
            DEV_CHECK_ERR(m_pContext == pContext, "Query '", this->m_Desc.Name, "' has been begun by another context.");
        }
        else
        {
            // Timestamp queries are never begun
            if (m_pContext != nullptr && m_pContext != pContext)
                Invalidate();

            m_pContext = pContext;
        }

        m_State = QueryState::Ended;
    }

    QueryState GetState() const
    {
        return m_State;
    }

    void CheckQueryDataPtr(void* pData, Uint32 DataSize)
    {
        DEV_CHECK_ERR(m_State == QueryState::Ended,
                      "Attempting to get data of query '", this->m_Desc.Name, "' that has not been ended.");

        if (pData != nullptr)
        {
            DEV_CHECK_ERR(*reinterpret_cast<QUERY_TYPE*>(pData) == this->m_Desc.Type, "Incorrect query data structure type.");

            static_assert(QUERY_TYPE_NUM_TYPES == 6, "Not all QUERY_TYPE enum values are handled below.");
            switch (this->m_Desc.Type)
            {
                case QUERY_TYPE_UNDEFINED:
                    UNEXPECTED("Undefined query type is unexpected.");
                    break;

                case QUERY_TYPE_OCCLUSION:
                    DEV_CHECK_ERR(DataSize == sizeof(QueryDataOcclusion),
                                  "The size of query data (", DataSize, ") is incorrect: ", sizeof(QueryDataOcclusion), " (aka sizeof(QueryDataOcclusion)) is expected.");
                    break;

                case QUERY_TYPE_BINARY_OCCLUSION:
                    DEV_CHECK_ERR(DataSize == sizeof(QueryDataBinaryOcclusion),
                                  "The size of query data (", DataSize, ") is incorrect: ", sizeof(QueryDataBinaryOcclusion), " (aka sizeof(QueryDataBinaryOcclusion)) is expected.");
                    break;

                case QUERY_TYPE_TIMESTAMP:
                    DEV_CHECK_ERR(DataSize == sizeof(QueryDataTimestamp),
                                  "The size of query data (", DataSize, ") is incorrect: ", sizeof(QueryDataTimestamp), " (aka sizeof(QueryDataTimestamp)) is expected.");
                    break;

                case QUERY_TYPE_PIPELINE_STATISTICS:
                    DEV_CHECK_ERR(DataSize == sizeof(QueryDataPipelineStatistics),
                                  "The size of query data (", DataSize, ") is incorrect: ", sizeof(QueryDataPipelineStatistics), " (aka sizeof(QueryDataPipelineStatistics)) is expected.");
                    break;

                case QUERY_TYPE_DURATION:
                    DEV_CHECK_ERR(DataSize == sizeof(QueryDataDuration),
                                  "The size of query data (", DataSize, ") is incorrect: ", sizeof(QueryDataDuration), " (aka sizeof(QueryDataDuration)) is expected.");
                    break;

                default:
                    UNEXPECTED("Unexpected query type.");
            }
        }
    }

protected:
    RefCntAutoPtr<DeviceContextImplType> m_pContext;

    QueryState m_State = QueryState::Inactive;
};

} // namespace Diligent
