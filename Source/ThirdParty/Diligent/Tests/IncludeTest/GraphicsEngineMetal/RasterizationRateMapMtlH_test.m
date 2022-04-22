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

#include <Metal/Metal.h>
#include "DiligentCore/Graphics/GraphicsEngineMetal/interface/RasterizationRateMapMtl.h"

void TestRasterizationRateMapMtl_CInterface(IRasterizationRateMapMtl* pRRM)
{
    const RasterizationRateMapDesc* rrmDesc = IRasterizationRateMapMtl_GetDesc(pRRM);
    (void)rrmDesc;

    if (@available(macos 10.15.4, ios 13.0, *))
    {
        id<MTLRasterizationRateMap> mtlRRM = IRasterizationRateMapMtl_GetMtlResource(pRRM);
        (void)mtlRRM;
    }
    ITextureView* pView = IRasterizationRateMapMtl_GetView(pRRM);
    (void)pView;
    IRasterizationRateMapMtl_GetPhysicalSizeForLayer(pRRM, 0, (Uint32*)NULL, (Uint32*)NULL);
    IRasterizationRateMapMtl_GetPhysicalGranularity(pRRM, (Uint32*)NULL, (Uint32*)NULL);
    IRasterizationRateMapMtl_MapScreenToPhysicalCoordinates(pRRM, 0, 0.f, 0.f, (float*)NULL, (float*)NULL);
    IRasterizationRateMapMtl_MapPhysicalToScreenCoordinates(pRRM, 0, 0.f, 0.f, (float*)NULL, (float*)NULL);
    IRasterizationRateMapMtl_GetParameterBufferSizeAndAlign(pRRM, (Uint64*)NULL, (Uint32*)NULL);
    IRasterizationRateMapMtl_CopyParameterDataToBuffer(pRRM, (struct IBuffer*)NULL, 0);
}
