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

#include <utility>

namespace Diligent
{

template <typename CallbackType, typename RetType, typename... ArgsType>
struct CallbackWrapper
{
    using RawFuncType = RetType(ArgsType..., void*);

    CallbackWrapper(CallbackType&& _Callback) :
        Callback{std::forward<CallbackType>(_Callback)},
        RawFunction //
        {
            [](ArgsType... Args, void* pData) //
            {
                auto* pCallback = reinterpret_cast<CallbackType*>(pData);
                return (*pCallback)(std::forward<ArgsType>(Args)...);
            } //
        }
    {
    }

    RawFuncType* GetRawFunc() const { return RawFunction; }
    void*        GetData() { return &Callback; }

    operator RawFuncType*() const { return GetRawFunc(); }
    operator void*() { return GetData(); }

private:
    CallbackType       Callback;
    RawFuncType* const RawFunction;
};

template <typename T>
struct FunctionTraits : public FunctionTraits<decltype(&T::operator())>
{};

template <typename ClassType, typename ReturnType, typename... ArgsType>
struct FunctionTraits<ReturnType (ClassType::*)(ArgsType...) const>
{
    static CallbackWrapper<ClassType, ReturnType, ArgsType...> MakeCallback(ClassType&& Callback)
    {
        return {std::forward<ClassType>(Callback)};
    }
};

/// Makes a callback wrapper for a function of type F(Args..., void*)
///
/// \remarks The last argument is expected to be void*, and it is used
///          to pass the function state.
///
/// Example:
///
///    auto ModifyPipelineCI = MakeCallback([&](PipelineStateCreateInfo* pPipelineCI) {
///        auto* pGraphicsPipelineCI                              = static_cast<GraphicsPipelineStateCreateInfo*>(pPipelineCI);
///        pGraphicsPipelineCI->GraphicsPipeline.RTVFormats[0]    = m_pSwapChain->GetDesc().ColorBufferFormat;
///        pGraphicsPipelineCI->GraphicsPipeline.DSVFormat        = m_pSwapChain->GetDesc().DepthBufferFormat;
///        pGraphicsPipelineCI->GraphicsPipeline.NumRenderTargets = 1;
///    });
///
///    LoadPipelineStateInfo PipelineLI{};
///    PipelineLI.Modify       = ModifyPipelineCI;
///    PipelineLI.pUserData    = ModifyPipelineCI;
template <typename CallbackType>
auto MakeCallback(CallbackType&& Callback)
{
    return FunctionTraits<CallbackType>::MakeCallback(std::forward<CallbackType>(Callback));
}

} // namespace Diligent
