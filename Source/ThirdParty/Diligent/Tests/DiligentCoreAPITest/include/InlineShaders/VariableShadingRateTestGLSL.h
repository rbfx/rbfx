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

#include <string>

namespace
{

namespace GLSL
{

const std::string ShadingRateToColor{R"glsl(
#version 460
#extension GL_EXT_fragment_shading_rate : require
// in int gl_ShadingRateEXT;

//const int gl_ShadingRateFlag2VerticalPixelsEXT = 1;
//const int gl_ShadingRateFlag4VerticalPixelsEXT = 2;
//const int gl_ShadingRateFlag2HorizontalPixelsEXT = 4;
//const int gl_ShadingRateFlag4HorizontalPixelsEXT = 8;
// 1x1 = 0 | 0 = 0
// 4x4 = 2 | 8 = 10

vec4 ShadingRateToColor()
{
    float h   = clamp(gl_ShadingRateEXT * 0.1, 0.0, 1.0) / 1.35;
    vec3  col = vec3(abs(h * 6.0 - 3.0) - 1.0, 2.0 - abs(h * 6.0 - 2.0), 2.0 - abs(h * 6.0 - 4.0));
    return vec4(clamp(col, vec3(0.0), vec3(1.0)), 1.0);
}
)glsl"};


const std::string PerDrawShadingRate_VS{R"glsl(
#version 460
#extension GL_EXT_fragment_shading_rate : require

void main()
{
    gl_Position = vec4(vec2(gl_VertexIndex >> 1, gl_VertexIndex & 1) * 4.0 - 1.0, 0.0, 1.0);
}
)glsl"};


const std::string PerDrawShadingRate_PS = ShadingRateToColor + R"glsl(
layout(location=0) out vec4 out_Color;

void main()
{
    out_Color = ShadingRateToColor();
}
)glsl";


const std::string PerPrimitiveShadingRate_VS{R"glsl(
#version 460
#extension GL_EXT_fragment_shading_rate : require
// out int gl_PrimitiveShadingRateEXT

layout(location=0) in vec2 in_Pos;
layout(location=1) in int  in_ShadingRate;

void main()
{
    gl_Position = vec4(in_Pos, 0.0, 1.0);
    gl_PrimitiveShadingRateEXT = in_ShadingRate;
}
)glsl"};


const std::string PerPrimitiveShadingRate_PS = ShadingRateToColor + R"glsl(
layout(location=0) out vec4 out_Color;

void main()
{
    out_Color = ShadingRateToColor();
}
)glsl";


const std::string TextureBasedShadingRate_VS{R"glsl(
#version 460
#extension GL_EXT_fragment_shading_rate : require

void main()
{
    gl_Position = vec4(vec2(gl_VertexIndex >> 1, gl_VertexIndex & 1) * 4.0 - 1.0, 0.0, 1.0);
}
)glsl"};


const std::string TextureBasedShadingRate_PS = ShadingRateToColor + R"glsl(
layout(location=0) out vec4 out_Color;

void main()
{
    out_Color = ShadingRateToColor();
}
)glsl";

} // namespace GLSL

} // namespace
