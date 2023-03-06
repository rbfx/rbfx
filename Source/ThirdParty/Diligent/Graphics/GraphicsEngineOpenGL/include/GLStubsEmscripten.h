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

#pragma once

#include "Errors.hpp"

#ifndef GLAPIENTRY
#    define GLAPIENTRY GL_APIENTRY
#endif

// Define unsupported formats for OpenGL ES

#ifndef GL_TEXTURE_INTERNAL_FORMAT
#    define GL_TEXTURE_INTERNAL_FORMAT 0
#endif

#ifndef GL_RGBA16
#    define GL_RGBA16 0x805B
#endif

#ifndef GL_RGBA16_SNORM
#    define GL_RGBA16_SNORM 0x8F9B
#endif

#ifndef GL_RG16
#    define GL_RG16 0x822C
#endif

#ifndef GL_RG16_SNORM
#    define GL_RG16_SNORM 0x8F99
#endif

#ifndef GL_R16
#    define GL_R16 0x822A
#endif

#ifndef GL_R16_SNORM
#    define GL_R16_SNORM 0x8F98
#endif

#ifndef GL_COMPRESSED_RGB_S3TC_DXT1_EXT
#    define GL_COMPRESSED_RGB_S3TC_DXT1_EXT 0x83F0
#endif

#ifndef GL_COMPRESSED_SRGB_S3TC_DXT1_EXT
#    define GL_COMPRESSED_SRGB_S3TC_DXT1_EXT 0x8C4C
#endif

#ifndef GL_COMPRESSED_RGBA_S3TC_DXT3_EXT
#    define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT 0x83F2
#endif

#ifndef GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT
#    define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT 0x8C4E
#endif

#ifndef GL_COMPRESSED_RGBA_S3TC_DXT5_EXT
#    define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT 0x83F3
#endif

#ifndef GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT
#    define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT 0x8C4F
#endif

#ifndef GL_COMPRESSED_RED_RGTC1
#    define GL_COMPRESSED_RED_RGTC1 0x8DBB
#endif

#ifndef GL_COMPRESSED_SIGNED_RED_RGTC1
#    define GL_COMPRESSED_SIGNED_RED_RGTC1 0x8DBC
#endif

#ifndef GL_COMPRESSED_RG_RGTC2
#    define GL_COMPRESSED_RG_RGTC2 0x8DBD
#endif

#ifndef GL_COMPRESSED_SIGNED_RG_RGTC2
#    define GL_COMPRESSED_SIGNED_RG_RGTC2 0x8DBE
#endif

#ifndef GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT
#    define GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT 0x8E8F
#endif

#ifndef GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT
#    define GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT 0x8E8E
#endif

#ifndef GL_COMPRESSED_RGBA_BPTC_UNORM
#    define GL_COMPRESSED_RGBA_BPTC_UNORM 0x8E8C
#endif

#ifndef GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM
#    define GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM 0x8E8D
#endif

#ifndef GL_UNSIGNED_INT_10_10_10_2
#    define GL_UNSIGNED_INT_10_10_10_2 0x8036
#endif

#ifndef GL_UNSIGNED_SHORT_5_6_5_REV
#    define GL_UNSIGNED_SHORT_5_6_5_REV 0x8364
#endif

#ifndef GL_UNSIGNED_SHORT_1_5_5_5_REV
#    define GL_UNSIGNED_SHORT_1_5_5_5_REV 0x8366
#endif

// Define unsupported shaders
#ifndef GL_GEOMETRY_SHADER
#    define GL_GEOMETRY_SHADER 0x8DD9
#endif

#ifndef GL_TESS_CONTROL_SHADER
#    define GL_TESS_CONTROL_SHADER 0x8E88
#endif

#ifndef GL_TESS_EVALUATION_SHADER
#    define GL_TESS_EVALUATION_SHADER 0x8E87
#endif

// Define unsupported texture filtering modes
#ifndef GL_CLAMP_TO_BORDER
#    define GL_CLAMP_TO_BORDER 0
#endif

#ifndef GL_MIRROR_CLAMP_TO_EDGE
#    define GL_MIRROR_CLAMP_TO_EDGE 0
#endif

// Define unsupported bind points
#ifndef GL_TEXTURE_BINDING_1D
#    define GL_TEXTURE_BINDING_1D 0x8068
#endif

#ifndef GL_TEXTURE_1D_ARRAY
#    define GL_TEXTURE_1D_ARRAY 0x8C18
#endif

#ifndef GL_TEXTURE_1D
#    define GL_TEXTURE_1D 0x0DE0
#endif

#ifndef GL_TEXTURE_BINDING_1D_ARRAY
#    define GL_TEXTURE_BINDING_1D_ARRAY 0x8C1C
#endif

#ifndef GL_TEXTURE_2D_MULTISAMPLE
#    define GL_TEXTURE_2D_MULTISAMPLE 0x9100
#endif

#ifndef GL_TEXTURE_2D_MULTISAMPLE_ARRAY
#    define GL_TEXTURE_2D_MULTISAMPLE_ARRAY 0x9102
#endif

#ifndef GL_TEXTURE_CUBE_MAP_ARRAY
#    define GL_TEXTURE_CUBE_MAP_ARRAY 0x9009
#endif

#ifndef GL_TEXTURE_BUFFER
#    define GL_TEXTURE_BUFFER 0
#endif

#define glTexBuffer(...) 0

// Define unsupported pipeline bind flags
#ifndef GL_VERTEX_SHADER_BIT
#    define GL_VERTEX_SHADER_BIT 0x00000001
#endif

#ifndef GL_FRAGMENT_SHADER_BIT
#    define GL_FRAGMENT_SHADER_BIT 0x00000002
#endif

#ifndef GL_GEOMETRY_SHADER_BIT
#    define GL_GEOMETRY_SHADER_BIT 0x00000004
#endif

#ifndef GL_TESS_CONTROL_SHADER_BIT
#    define GL_TESS_CONTROL_SHADER_BIT 0x00000008
#endif

#ifndef GL_TESS_EVALUATION_SHADER_BIT
#    define GL_TESS_EVALUATION_SHADER_BIT 0x00000010
#endif

#ifndef GL_COMPUTE_SHADER_BIT
#    define GL_COMPUTE_SHADER_BIT 0x00000020
#endif

// Clip control
#ifndef GL_LOWER_LEFT
#    define GL_LOWER_LEFT 0x8CA1
#endif

#ifndef GL_UPPER_LEFT
#    define GL_UPPER_LEFT 0x8CA2
#endif

#ifndef GL_ZERO_TO_ONE
#    define GL_ZERO_TO_ONE 0x935F
#endif

#ifndef NEGATIVE_ONE_TO_ONE
#    define NEGATIVE_ONE_TO_ONE 0x935E
#endif

// Compute shader stubs
#ifndef GL_READ_ONLY
#    define GL_READ_ONLY 0x88B8
#endif

#ifndef GL_WRITE_ONLY
#    define GL_WRITE_ONLY 0x88B9
#endif

#ifndef GL_READ_WRITE
#    define GL_READ_WRITE 0x88BA
#endif

#ifndef GL_COMPUTE_SHADER
#    define GL_COMPUTE_SHADER 0x91B9
#endif

// Polygon mode
#ifndef GL_POINT
#    define GL_POINT 0x1B00
#endif

#ifndef GL_LINE
#    define GL_LINE 0x1B01
#endif

#ifndef GL_FILL
#    define GL_FILL 0x1B02
#endif

#ifndef GL_DEPTH_CLAMP
#    define GL_DEPTH_CLAMP 0
#endif

// Blend functions
#ifndef GL_SRC1_COLOR
#    define GL_SRC1_COLOR 0x88F9
#endif

#ifndef GL_ONE_MINUS_SRC1_COLOR
#    define GL_ONE_MINUS_SRC1_COLOR 0x88FA
#endif

#ifndef GL_SOURCE1_ALPHA
#    define GL_SOURCE1_ALPHA 0x8589
#endif

#ifndef GL_SRC1_ALPHA
#    define GL_SRC1_ALPHA GL_SOURCE1_ALPHA
#endif

#ifndef GL_ONE_MINUS_SRC1_ALPHA
#    define GL_ONE_MINUS_SRC1_ALPHA 0x88FB
#endif

// Define unsupported sampler attributes
#ifndef GL_TEXTURE_LOD_BIAS
#    define GL_TEXTURE_LOD_BIAS 0
#endif

#ifndef GL_TEXTURE_MAX_ANISOTROPY_EXT
#    define GL_TEXTURE_MAX_ANISOTROPY_EXT 0
#endif

#ifndef GL_TEXTURE_BORDER_COLOR
#    define GL_TEXTURE_BORDER_COLOR 0
#endif

// Other unsupported attributes
#ifndef GL_PROGRAM_SEPARABLE
#    define GL_PROGRAM_SEPARABLE 0x8258
#endif

// Define unsupported uniform data types
#ifndef GL_SAMPLER_1D
#    define GL_SAMPLER_1D 0x8B5D
#endif

#ifndef GL_SAMPLER_1D_SHADOW
#    define GL_SAMPLER_1D_SHADOW 0x8B61
#endif

#ifndef GL_SAMPLER_1D_ARRAY
#    define GL_SAMPLER_1D_ARRAY 0x8DC0
#endif

#ifndef GL_SAMPLER_1D_ARRAY_SHADOW
#    define GL_SAMPLER_1D_ARRAY_SHADOW 0x8DC3
#endif

#ifndef GL_INT_SAMPLER_1D
#    define GL_INT_SAMPLER_1D 0x8DC9
#endif

#ifndef GL_INT_SAMPLER_1D_ARRAY
#    define GL_INT_SAMPLER_1D_ARRAY 0x8DCE
#endif

#ifndef GL_UNSIGNED_INT_SAMPLER_1D
#    define GL_UNSIGNED_INT_SAMPLER_1D 0x8DD1
#endif

#ifndef GL_UNSIGNED_INT_SAMPLER_1D_ARRAY
#    define GL_UNSIGNED_INT_SAMPLER_1D_ARRAY 0x8DD6
#endif

#ifndef GL_SAMPLER_CUBE_MAP_ARRAY
#    define GL_SAMPLER_CUBE_MAP_ARRAY 0x900C
#endif

#ifndef GL_SAMPLER_CUBE_MAP_ARRAY_SHADOW
#    define GL_SAMPLER_CUBE_MAP_ARRAY_SHADOW 0x900D
#endif

#ifndef GL_INT_SAMPLER_CUBE_MAP_ARRAY
#    define GL_INT_SAMPLER_CUBE_MAP_ARRAY 0x900E
#endif

#ifndef GL_UNSIGNED_INT_SAMPLER_CUBE_MAP_ARRAY
#    define GL_UNSIGNED_INT_SAMPLER_CUBE_MAP_ARRAY 0x900F
#endif

#ifndef GL_SAMPLER_BUFFER
#    define GL_SAMPLER_BUFFER 0x8DC2
#endif

#ifndef GL_INT_SAMPLER_BUFFER
#    define GL_INT_SAMPLER_BUFFER 0x8DD0
#endif

#ifndef GL_UNSIGNED_INT_SAMPLER_BUFFER
#    define GL_UNSIGNED_INT_SAMPLER_BUFFER 0x8DD8
#endif

#ifndef GL_SAMPLER_2D_MULTISAMPLE
#    define GL_SAMPLER_2D_MULTISAMPLE 0x9108
#endif

#ifndef GL_INT_SAMPLER_2D_MULTISAMPLE
#    define GL_INT_SAMPLER_2D_MULTISAMPLE 0x9109
#endif

#ifndef GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE
#    define GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE 0x910A
#endif

#ifndef GL_SAMPLER_2D_MULTISAMPLE_ARRAY
#    define GL_SAMPLER_2D_MULTISAMPLE_ARRAY 0x910B
#endif

#ifndef GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY
#    define GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY 0x910C
#endif

#ifndef GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY
#    define GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY 0x910D
#endif

#ifndef GL_SAMPLER_EXTERNAL_OES
#    define GL_SAMPLER_EXTERNAL_OES 0x8D66
#endif

#ifndef GL_DRAW_INDIRECT_BUFFER
#    define GL_DRAW_INDIRECT_BUFFER 0x8F3F
#endif

#ifndef GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT
#    define GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT 0x00000001
#endif

#ifndef GL_ELEMENT_ARRAY_BARRIER_BIT
#    define GL_ELEMENT_ARRAY_BARRIER_BIT 0x00000002
#endif

#ifndef GL_TEXTURE_UPDATE_BARRIER_BIT
#    define GL_TEXTURE_UPDATE_BARRIER_BIT 0x00000100
#endif

#ifndef GL_BUFFER_UPDATE_BARRIER_BIT
#    define GL_BUFFER_UPDATE_BARRIER_BIT 0x00000200
#endif

#ifndef GL_UNIFORM_BARRIER_BIT
#    define GL_UNIFORM_BARRIER_BIT 0x00000004
#endif
#ifndef GL_TEXTURE_FETCH_BARRIER_BIT
#    define GL_TEXTURE_FETCH_BARRIER_BIT 0x00000008
#endif

#ifndef GL_IMAGE_1D
#    define GL_IMAGE_1D 0x904C
#endif

#ifndef GL_IMAGE_2D
#    define GL_IMAGE_2D 0x904D
#endif

#ifndef GL_IMAGE_3D
#    define GL_IMAGE_3D 0x904E
#endif

#ifndef GL_IMAGE_2D_RECT
#    define GL_IMAGE_2D_RECT 0x904F
#endif

#ifndef GL_IMAGE_CUBE
#    define GL_IMAGE_CUBE 0x9050
#endif

#ifndef GL_IMAGE_BUFFER
#    define GL_IMAGE_BUFFER 0x9051
#endif

#ifndef GL_IMAGE_1D_ARRAY
#    define GL_IMAGE_1D_ARRAY 0x9052
#endif

#ifndef GL_IMAGE_2D_ARRAY
#    define GL_IMAGE_2D_ARRAY 0x9053
#endif

#ifndef GL_IMAGE_CUBE_MAP_ARRAY
#    define GL_IMAGE_CUBE_MAP_ARRAY 0x9054
#endif

#ifndef GL_IMAGE_2D_MULTISAMPLE
#    define GL_IMAGE_2D_MULTISAMPLE 0x9055
#endif

#ifndef GL_IMAGE_2D_MULTISAMPLE_ARRAY
#    define GL_IMAGE_2D_MULTISAMPLE_ARRAY 0x9056
#endif

#ifndef GL_INT_IMAGE_1D
#    define GL_INT_IMAGE_1D 0x9057
#endif

#ifndef GL_INT_IMAGE_2D
#    define GL_INT_IMAGE_2D 0x9058
#endif

#ifndef GL_INT_IMAGE_3D
#    define GL_INT_IMAGE_3D 0x9059
#endif

#ifndef GL_INT_IMAGE_2D_RECT
#    define GL_INT_IMAGE_2D_RECT 0x905A
#endif

#ifndef GL_INT_IMAGE_CUBE
#    define GL_INT_IMAGE_CUBE 0x905B
#endif

#ifndef GL_INT_IMAGE_BUFFER
#    define GL_INT_IMAGE_BUFFER 0x905C
#endif

#ifndef GL_INT_IMAGE_1D_ARRAY
#    define GL_INT_IMAGE_1D_ARRAY 0x905D
#endif

#ifndef GL_INT_IMAGE_2D_ARRAY
#    define GL_INT_IMAGE_2D_ARRAY 0x905E
#endif

#ifndef GL_INT_IMAGE_CUBE_MAP_ARRAY
#    define GL_INT_IMAGE_CUBE_MAP_ARRAY 0x905F
#endif

#ifndef GL_INT_IMAGE_2D_MULTISAMPLE
#    define GL_INT_IMAGE_2D_MULTISAMPLE 0x9060
#endif

#ifndef GL_INT_IMAGE_2D_MULTISAMPLE_ARRAY
#    define GL_INT_IMAGE_2D_MULTISAMPLE_ARRAY 0x9061
#endif

#ifndef GL_UNSIGNED_INT_IMAGE_1D
#    define GL_UNSIGNED_INT_IMAGE_1D 0x9062
#endif

#ifndef GL_UNSIGNED_INT_IMAGE_2D
#    define GL_UNSIGNED_INT_IMAGE_2D 0x9063
#endif

#ifndef GL_UNSIGNED_INT_IMAGE_3D
#    define GL_UNSIGNED_INT_IMAGE_3D 0x9064
#endif

#ifndef GL_UNSIGNED_INT_IMAGE_2D_RECT
#    define GL_UNSIGNED_INT_IMAGE_2D_RECT 0x9065
#endif

#ifndef GL_UNSIGNED_INT_IMAGE_CUBE
#    define GL_UNSIGNED_INT_IMAGE_CUBE 0x9066
#endif

#ifndef GL_UNSIGNED_INT_IMAGE_BUFFER
#    define GL_UNSIGNED_INT_IMAGE_BUFFER 0x9067
#endif

#ifndef GL_UNSIGNED_INT_IMAGE_1D_ARRAY
#    define GL_UNSIGNED_INT_IMAGE_1D_ARRAY 0x9068
#endif

#ifndef GL_UNSIGNED_INT_IMAGE_2D_ARRAY
#    define GL_UNSIGNED_INT_IMAGE_2D_ARRAY 0x9069
#endif

#ifndef GL_UNSIGNED_INT_IMAGE_CUBE_MAP_ARRAY
#    define GL_UNSIGNED_INT_IMAGE_CUBE_MAP_ARRAY 0x906A
#endif

#ifndef GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE
#    define GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE 0x906B
#endif

#ifndef GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE_ARRAY
#    define GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE_ARRAY 0x906C
#endif

// Compute shader stubs
#ifndef GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT
#    define GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT 0x00004000
#endif

#ifndef GL_FRAMEBUFFER_BARRIER_BIT
#    define GL_FRAMEBUFFER_BARRIER_BIT 0x00000400
#endif

// Query
#ifndef GL_QUERY_BUFFER
#    define GL_QUERY_BUFFER 0x9192
#endif

// ---------------------  Framebuffer SRGB -----------------------
#ifndef GL_FRAMEBUFFER_SRGB
#    define GL_FRAMEBUFFER_SRGB 0x8DB9
#endif

// -------------------- Incomplete FBO error codes ---------------
#ifndef GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS
#    define GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS 0x8DA8
#endif

#ifndef GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER
#    define GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER 0x8CDB
#endif

#ifndef GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER
#    define GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER 0x8CDC
#endif

// ---------------------- Primitive topologies -------------------
#ifndef GL_LINES_ADJACENCY
#    define GL_LINES_ADJACENCY 0x000A
#endif

#ifndef GL_LINE_STRIP_ADJACENCY
#    define GL_LINE_STRIP_ADJACENCY 0x000B
#endif

#ifndef GL_TRIANGLES_ADJACENCY
#    define GL_TRIANGLES_ADJACENCY 0x000C
#endif

#ifndef GL_TRIANGLE_STRIP_ADJACENCY
#    define GL_TRIANGLE_STRIP_ADJACENCY 0x000D
#endif


// ---------------------- Uniform data types -------------------
#ifndef GL_DOUBLE
#    define GL_DOUBLE 0x140A
#endif

#ifndef GL_DOUBLE_MAT2
#    define GL_DOUBLE_MAT2 0x8F46
#endif

#ifndef GL_DOUBLE_MAT3
#    define GL_DOUBLE_MAT3 0x8F47
#endif

#ifndef GL_DOUBLE_MAT4
#    define GL_DOUBLE_MAT4 0x8F48
#endif

#ifndef GL_DOUBLE_MAT2x3
#    define GL_DOUBLE_MAT2x3 0x8F49
#endif

#ifndef GL_DOUBLE_MAT2x4
#    define GL_DOUBLE_MAT2x4 0x8F4A
#endif

#ifndef GL_DOUBLE_MAT3x2
#    define GL_DOUBLE_MAT3x2 0x8F4B
#endif

#ifndef GL_DOUBLE_MAT3x4
#    define GL_DOUBLE_MAT3x4 0x8F4C
#endif

#ifndef GL_DOUBLE_MAT4x2
#    define GL_DOUBLE_MAT4x2 0x8F4D
#endif

#ifndef GL_DOUBLE_MAT4x3
#    define GL_DOUBLE_MAT4x3 0x8F4E
#endif

#ifndef GL_DOUBLE_VEC2
#    define GL_DOUBLE_VEC2 0x8FFC
#endif

#ifndef GL_DOUBLE_VEC3
#    define GL_DOUBLE_VEC3 0x8FFD
#endif

#ifndef GL_DOUBLE_VEC4
#    define GL_DOUBLE_VEC4 0x8FFE
#endif


// Define unsupported GL function stubs
// We need a Variatic Template to turn off the warning about unused variables
template <typename T, typename... Args>
void UnsupportedGLFunctionStub(const T& Name, Args const&... Arg)
{
    LOG_ERROR_MESSAGE(Name, "() is not supported on Emscripten!\n");
}

#define glDrawElementsInstancedBaseVertexBaseInstance(...) UnsupportedGLFunctionStub("glDrawElementsInstancedBaseVertexBaseInstance", __VA_ARGS__)
#define glDrawElementsInstancedBaseVertex(...)             UnsupportedGLFunctionStub("glDrawElementsInstancedBaseVertex", __VA_ARGS__)
#define glDrawElementsInstancedBaseInstance(...)           UnsupportedGLFunctionStub("glDrawElementsInstancedBaseInstance", __VA_ARGS__)
#define glDrawArraysInstancedBaseInstance(...)             UnsupportedGLFunctionStub("glDrawArraysInstancedBaseInstance", __VA_ARGS__)
#define glDrawElementsBaseVertex(...)                      UnsupportedGLFunctionStub("glDrawElementsBaseVertex", __VA_ARGS__)
static void (*glTextureView)(GLuint texture, GLenum target, GLuint origtexture, GLenum internalformat, GLuint minlevel, GLuint numlevels, GLuint minlayer, GLuint numlayers) = nullptr;
#define glTexStorage1D(...)            UnsupportedGLFunctionStub("glTexStorage1D", __VA_ARGS__)
#define glViewportIndexedf(...)        UnsupportedGLFunctionStub("glViewportIndexedf", __VA_ARGS__)
#define glScissorIndexed(...)          UnsupportedGLFunctionStub("glScissorIndexed", __VA_ARGS__)
#define glTexSubImage1D(...)           UnsupportedGLFunctionStub("glTexSubImage1D", __VA_ARGS__)
#define glTexStorage3DMultisample(...) UnsupportedGLFunctionStub("glTexStorage3DMultisample", __VA_ARGS__)
#define glGetTexLevelParameteriv(...)  UnsupportedGLFunctionStub("glGetTexLevelParameteriv", __VA_ARGS__)
#define glClipControl(...)             UnsupportedGLFunctionStub("glClipControl", __VA_ARGS__)
#define glDepthRangeIndexed(...)       UnsupportedGLFunctionStub("glDepthRangeIndexed", __VA_ARGS__)
static void (*glPolygonMode)(GLenum face, GLenum mode) = nullptr;
#define glEnablei(...)                UnsupportedGLFunctionStub("glEnablei")
#define glBlendFuncSeparatei(...)     UnsupportedGLFunctionStub("glBlendFuncSeparatei", __VA_ARGS__)
#define glBlendEquationSeparatei(...) UnsupportedGLFunctionStub("glBlendEquationSeparatei", __VA_ARGS__)
#define glDisablei(...)               UnsupportedGLFunctionStub("glDisablei", __VA_ARGS__)
#define glColorMaski(...)             UnsupportedGLFunctionStub("glColorMaski", __VA_ARGS__)
#define glFramebufferTexture(...)     UnsupportedGLFunctionStub("glFramebufferTexture", __VA_ARGS__)
#define glFramebufferTexture1D(...)   UnsupportedGLFunctionStub("glFramebufferTexture1D", __VA_ARGS__)
static void (*glGetQueryObjectui64v)(GLuint id, GLenum pname, GLuint64* params) = nullptr;
#define glGenProgramPipelines(...)     UnsupportedGLFunctionStub("glGenProgramPipelines", __VA_ARGS__)
#define glBindProgramPipeline(...)     UnsupportedGLFunctionStub("glBindProgramPipeline", __VA_ARGS__)
#define glDeleteProgramPipelines(...)  UnsupportedGLFunctionStub("glDeleteProgramPipelines", __VA_ARGS__)
#define glUseProgramStages(...)        UnsupportedGLFunctionStub("glUseProgramStages", __VA_ARGS__)
#define glUseProgramStages(...)        UnsupportedGLFunctionStub("glUseProgramStages", __VA_ARGS__)
#define glBindImageTexture(...)        UnsupportedGLFunctionStub("glBindImageTexture", __VA_ARGS__)
#define glDispatchCompute(...)         UnsupportedGLFunctionStub("glDispatchCompute", __VA_ARGS__)
#define glPatchParameteri(...)         UnsupportedGLFunctionStub("glPatchParameteri", __VA_ARGS__)
#define glTexStorage2DMultisample(...) UnsupportedGLFunctionStub("glTexStorage2DMultisample", __VA_ARGS__)
