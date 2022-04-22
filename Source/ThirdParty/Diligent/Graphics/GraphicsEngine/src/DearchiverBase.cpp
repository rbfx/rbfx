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

#include "DearchiverBase.hpp"
#include "ArchiveFileImpl.hpp"
#include "ArchiveMemoryImpl.hpp"

namespace Diligent
{

#define CHECK_UNPACK_PARAMATER(Expr, ...)   \
    do                                      \
    {                                       \
        DEV_CHECK_ERR(Expr, ##__VA_ARGS__); \
        if (!(Expr))                        \
            return false;                   \
    } while (false)

bool DearchiverBase::VerifyPipelineStateUnpackInfo(const PipelineStateUnpackInfo& DeArchiveInfo, IPipelineState** ppPSO) const
{
#define CHECK_UNPACK_PSO_PARAM(Expr, ...) CHECK_UNPACK_PARAMATER("Invalid PSO unpack parameter: ", ##__VA_ARGS__)
    CHECK_UNPACK_PSO_PARAM(ppPSO != nullptr, "ppPSO must not be null");
    CHECK_UNPACK_PSO_PARAM(DeArchiveInfo.pArchive != nullptr, "pArchive must not be null");
    CHECK_UNPACK_PSO_PARAM(DeArchiveInfo.Name != nullptr, "Name must not be null");
    CHECK_UNPACK_PSO_PARAM(DeArchiveInfo.pDevice != nullptr, "pDevice must not be null");
    CHECK_UNPACK_PSO_PARAM(DeArchiveInfo.PipelineType <= PIPELINE_TYPE_LAST, "PipelineType must be valid");
#undef CHECK_UNPACK_PSO_PARAM

    return true;
}

bool DearchiverBase::VerifyResourceSignatureUnpackInfo(const ResourceSignatureUnpackInfo& DeArchiveInfo, IPipelineResourceSignature** ppSignature) const
{
#define CHECK_UNPACK_SIGN_PARAM(Expr, ...) CHECK_UNPACK_PARAMATER("Invalid signature unpack parameter: ", ##__VA_ARGS__)
    CHECK_UNPACK_SIGN_PARAM(ppSignature != nullptr, "ppSignature must not be null");
    CHECK_UNPACK_SIGN_PARAM(DeArchiveInfo.pArchive != nullptr, "pArchive must not be null");
    CHECK_UNPACK_SIGN_PARAM(DeArchiveInfo.Name != nullptr, "Name must not be null");
    CHECK_UNPACK_SIGN_PARAM(DeArchiveInfo.pDevice != nullptr, "pDevice must not be null");
#undef CHECK_UNPACK_SIGN_PARAM

    return true;
}

bool DearchiverBase::VerifyRenderPassUnpackInfo(const RenderPassUnpackInfo& DeArchiveInfo, IRenderPass** ppRP) const
{
#define CHECK_UNPACK_RENDER_PASS_PARAM(Expr, ...) CHECK_UNPACK_PARAMATER("Invalid signature unpack parameter: ", ##__VA_ARGS__)
    CHECK_UNPACK_RENDER_PASS_PARAM(ppRP != nullptr, "ppRP must not be null");
    CHECK_UNPACK_RENDER_PASS_PARAM(DeArchiveInfo.pArchive != nullptr, "pArchive must not be null");
    CHECK_UNPACK_RENDER_PASS_PARAM(DeArchiveInfo.Name != nullptr, "Name must not be null");
    CHECK_UNPACK_RENDER_PASS_PARAM(DeArchiveInfo.pDevice != nullptr, "pDevice must not be null");
#undef CHECK_UNPACK_RENDER_PASS_PARAM

    return true;
}

} // namespace Diligent
