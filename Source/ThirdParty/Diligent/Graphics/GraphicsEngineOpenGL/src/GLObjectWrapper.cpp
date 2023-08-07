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

#include "pch.h"
#include "GLObjectWrapper.hpp"

namespace GLObjectWrappers
{

// clang-format off
const char *GLBufferObjCreateReleaseHelper  :: Name = "buffer";
const char *GLProgramObjCreateReleaseHelper :: Name = "program";
const char *GLShaderObjCreateReleaseHelper  :: Name = "shader";
const char *GLPipelineObjCreateReleaseHelper:: Name = "pipeline";
const char *GLVAOCreateReleaseHelper        :: Name = "vertex array";
const char *GLTextureCreateReleaseHelper    :: Name = "texture";
const char *GLSamplerCreateReleaseHelper    :: Name = "sampler";
const char *GLFBOCreateReleaseHelper        :: Name = "framebuffer";
const char *GLRBOCreateReleaseHelper        :: Name = "renderbuffer";
const char *GLQueryCreateReleaseHelper      :: Name = "query";
// clang-format on

// clang-format off
GLenum GLBufferObjCreateReleaseHelper  :: Type = GL_BUFFER;
GLenum GLProgramObjCreateReleaseHelper :: Type = GL_PROGRAM;
GLenum GLShaderObjCreateReleaseHelper  :: Type = GL_SHADER;
GLenum GLPipelineObjCreateReleaseHelper:: Type = GL_PROGRAM_PIPELINE;
GLenum GLVAOCreateReleaseHelper        :: Type = GL_VERTEX_ARRAY;
GLenum GLTextureCreateReleaseHelper    :: Type = GL_TEXTURE;
GLenum GLSamplerCreateReleaseHelper    :: Type = GL_SAMPLER;
GLenum GLFBOCreateReleaseHelper        :: Type = GL_FRAMEBUFFER;
GLenum GLRBOCreateReleaseHelper        :: Type = GL_RENDERBUFFER;
GLenum GLQueryCreateReleaseHelper      :: Type = GL_QUERY;
// clang-format on

} // namespace GLObjectWrappers
