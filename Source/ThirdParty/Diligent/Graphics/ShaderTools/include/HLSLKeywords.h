/*
 *  Copyright 2019-2024 Diligent Graphics LLC
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

// clang-format off

#define LIST_HLSL_VECTOR_AND_MATRIX_EXPANSIONS(HLSL_KEYWORD_HANDLER, type)\
HLSL_KEYWORD_HANDLER(type)\
HLSL_KEYWORD_HANDLER(type##1)\
HLSL_KEYWORD_HANDLER(type##2)\
HLSL_KEYWORD_HANDLER(type##3)\
HLSL_KEYWORD_HANDLER(type##4)\
HLSL_KEYWORD_HANDLER(type##1x1)\
HLSL_KEYWORD_HANDLER(type##1x2)\
HLSL_KEYWORD_HANDLER(type##1x3)\
HLSL_KEYWORD_HANDLER(type##1x4)\
HLSL_KEYWORD_HANDLER(type##2x1)\
HLSL_KEYWORD_HANDLER(type##2x2)\
HLSL_KEYWORD_HANDLER(type##2x3)\
HLSL_KEYWORD_HANDLER(type##2x4)\
HLSL_KEYWORD_HANDLER(type##3x1)\
HLSL_KEYWORD_HANDLER(type##3x2)\
HLSL_KEYWORD_HANDLER(type##3x3)\
HLSL_KEYWORD_HANDLER(type##3x4)\
HLSL_KEYWORD_HANDLER(type##4x1)\
HLSL_KEYWORD_HANDLER(type##4x2)\
HLSL_KEYWORD_HANDLER(type##4x3)\
HLSL_KEYWORD_HANDLER(type##4x4)

#define ITERATE_HLSL_KEYWORDS(HLSL_KEYWORD_HANDLER)\
/*All built-in types must be defined consecutively between bool and void*/\
LIST_HLSL_VECTOR_AND_MATRIX_EXPANSIONS(HLSL_KEYWORD_HANDLER, bool)\
LIST_HLSL_VECTOR_AND_MATRIX_EXPANSIONS(HLSL_KEYWORD_HANDLER, float)\
LIST_HLSL_VECTOR_AND_MATRIX_EXPANSIONS(HLSL_KEYWORD_HANDLER, int)\
LIST_HLSL_VECTOR_AND_MATRIX_EXPANSIONS(HLSL_KEYWORD_HANDLER, uint)\
LIST_HLSL_VECTOR_AND_MATRIX_EXPANSIONS(HLSL_KEYWORD_HANDLER, min16float)\
LIST_HLSL_VECTOR_AND_MATRIX_EXPANSIONS(HLSL_KEYWORD_HANDLER, min10float)\
LIST_HLSL_VECTOR_AND_MATRIX_EXPANSIONS(HLSL_KEYWORD_HANDLER, min16int)\
LIST_HLSL_VECTOR_AND_MATRIX_EXPANSIONS(HLSL_KEYWORD_HANDLER, min12int)\
LIST_HLSL_VECTOR_AND_MATRIX_EXPANSIONS(HLSL_KEYWORD_HANDLER, min16uint)\
HLSL_KEYWORD_HANDLER(matrix)\
HLSL_KEYWORD_HANDLER(void)\
/*All flow control keywords must be defined consecutively between break and while*/\
HLSL_KEYWORD_HANDLER(break)\
HLSL_KEYWORD_HANDLER(case)\
HLSL_KEYWORD_HANDLER(continue)\
HLSL_KEYWORD_HANDLER(default)\
HLSL_KEYWORD_HANDLER(do)\
HLSL_KEYWORD_HANDLER(else)\
HLSL_KEYWORD_HANDLER(for)\
HLSL_KEYWORD_HANDLER(if)\
HLSL_KEYWORD_HANDLER(return)\
HLSL_KEYWORD_HANDLER(switch)\
HLSL_KEYWORD_HANDLER(while)\
HLSL_KEYWORD_HANDLER(AppendStructuredBuffer)\
HLSL_KEYWORD_HANDLER(asm)\
HLSL_KEYWORD_HANDLER(asm_fragment)\
HLSL_KEYWORD_HANDLER(BlendState)\
HLSL_KEYWORD_HANDLER(Buffer)\
HLSL_KEYWORD_HANDLER(ByteAddressBuffer)\
HLSL_KEYWORD_HANDLER(cbuffer)\
HLSL_KEYWORD_HANDLER(centroid)\
HLSL_KEYWORD_HANDLER(class)\
HLSL_KEYWORD_HANDLER(column_major)\
HLSL_KEYWORD_HANDLER(compile)\
HLSL_KEYWORD_HANDLER(compile_fragment)\
HLSL_KEYWORD_HANDLER(CompileShader)\
HLSL_KEYWORD_HANDLER(const)\
HLSL_KEYWORD_HANDLER(ComputeShader)\
HLSL_KEYWORD_HANDLER(ConsumeStructuredBuffer)\
HLSL_KEYWORD_HANDLER(DepthStencilState)\
HLSL_KEYWORD_HANDLER(DepthStencilView)\
HLSL_KEYWORD_HANDLER(discard)\
HLSL_KEYWORD_HANDLER(double)\
HLSL_KEYWORD_HANDLER(DomainShader)\
HLSL_KEYWORD_HANDLER(dword)\
HLSL_KEYWORD_HANDLER(export)\
HLSL_KEYWORD_HANDLER(extern)\
HLSL_KEYWORD_HANDLER(false)\
HLSL_KEYWORD_HANDLER(fxgroup)\
HLSL_KEYWORD_HANDLER(GeometryShader)\
HLSL_KEYWORD_HANDLER(groupshared)\
HLSL_KEYWORD_HANDLER(half)\
HLSL_KEYWORD_HANDLER(Hullshader)\
HLSL_KEYWORD_HANDLER(in)\
HLSL_KEYWORD_HANDLER(inline)\
HLSL_KEYWORD_HANDLER(inout)\
HLSL_KEYWORD_HANDLER(InputPatch)\
HLSL_KEYWORD_HANDLER(interface)\
HLSL_KEYWORD_HANDLER(line)\
HLSL_KEYWORD_HANDLER(lineadj)\
HLSL_KEYWORD_HANDLER(linear)\
HLSL_KEYWORD_HANDLER(LineStream)\
HLSL_KEYWORD_HANDLER(namespace)\
HLSL_KEYWORD_HANDLER(nointerpolation)\
HLSL_KEYWORD_HANDLER(noperspective)\
HLSL_KEYWORD_HANDLER(NULL)\
HLSL_KEYWORD_HANDLER(out)\
HLSL_KEYWORD_HANDLER(OutputPatch)\
HLSL_KEYWORD_HANDLER(packoffset)\
HLSL_KEYWORD_HANDLER(pass)\
HLSL_KEYWORD_HANDLER(pixelfragment)\
HLSL_KEYWORD_HANDLER(PixelShader)\
HLSL_KEYWORD_HANDLER(point)\
HLSL_KEYWORD_HANDLER(PointStream)\
HLSL_KEYWORD_HANDLER(precise)\
HLSL_KEYWORD_HANDLER(RasterizerState)\
HLSL_KEYWORD_HANDLER(RenderTargetView)\
HLSL_KEYWORD_HANDLER(register)\
HLSL_KEYWORD_HANDLER(row_major)\
HLSL_KEYWORD_HANDLER(RWBuffer)\
HLSL_KEYWORD_HANDLER(RWByteAddressBuffer)\
HLSL_KEYWORD_HANDLER(RWStructuredBuffer)\
HLSL_KEYWORD_HANDLER(RWTexture1D)\
HLSL_KEYWORD_HANDLER(RWTexture1DArray)\
HLSL_KEYWORD_HANDLER(RWTexture2D)\
HLSL_KEYWORD_HANDLER(RWTexture2DArray)\
HLSL_KEYWORD_HANDLER(RWTexture3D)\
HLSL_KEYWORD_HANDLER(sample)\
HLSL_KEYWORD_HANDLER(sampler)\
HLSL_KEYWORD_HANDLER(SamplerState)\
HLSL_KEYWORD_HANDLER(SamplerComparisonState)\
HLSL_KEYWORD_HANDLER(shared)\
HLSL_KEYWORD_HANDLER(snorm)\
HLSL_KEYWORD_HANDLER(stateblock)\
HLSL_KEYWORD_HANDLER(stateblock_state)\
HLSL_KEYWORD_HANDLER(static)\
HLSL_KEYWORD_HANDLER(string)\
HLSL_KEYWORD_HANDLER(struct)\
HLSL_KEYWORD_HANDLER(StructuredBuffer)\
HLSL_KEYWORD_HANDLER(tbuffer)\
HLSL_KEYWORD_HANDLER(technique)\
HLSL_KEYWORD_HANDLER(technique10)\
HLSL_KEYWORD_HANDLER(technique11)\
HLSL_KEYWORD_HANDLER(texture)\
HLSL_KEYWORD_HANDLER(Texture1D)\
HLSL_KEYWORD_HANDLER(Texture1DArray)\
HLSL_KEYWORD_HANDLER(Texture2D)\
HLSL_KEYWORD_HANDLER(Texture2DArray)\
HLSL_KEYWORD_HANDLER(Texture2DMS)\
HLSL_KEYWORD_HANDLER(Texture2DMSArray)\
HLSL_KEYWORD_HANDLER(Texture3D)\
HLSL_KEYWORD_HANDLER(TextureCube)\
HLSL_KEYWORD_HANDLER(TextureCubeArray)\
HLSL_KEYWORD_HANDLER(true)\
HLSL_KEYWORD_HANDLER(typedef)\
HLSL_KEYWORD_HANDLER(triangle)\
HLSL_KEYWORD_HANDLER(triangleadj)\
HLSL_KEYWORD_HANDLER(TriangleStream)\
HLSL_KEYWORD_HANDLER(uniform)\
HLSL_KEYWORD_HANDLER(unorm)\
HLSL_KEYWORD_HANDLER(unsigned)\
HLSL_KEYWORD_HANDLER(vector)\
HLSL_KEYWORD_HANDLER(vertexfragment)\
HLSL_KEYWORD_HANDLER(VertexShader)\
HLSL_KEYWORD_HANDLER(volatile)
