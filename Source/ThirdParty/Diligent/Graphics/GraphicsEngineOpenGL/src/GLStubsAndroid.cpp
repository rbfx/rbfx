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

#include "pch.h"
#include "GLStubsAndroid.h"
#include <EGL/egl.h>

// clang-format off

#define DECLARE_GL_FUNCTION_NO_STUB(Func, FuncType, ...) \
    FuncType Func = nullptr;

#define DECLARE_GL_FUNCTION(Func, FuncType, ...)             \
    DECLARE_GL_FUNCTION_NO_STUB(Func, FuncType, __VA_ARGS__) \
    void Func##Stub(__VA_ARGS__)                             \
    {                                                        \
        UnsupportedGLFunctionStub(#Func);                    \
    }

#ifdef LOAD_GL_BIND_IMAGE_TEXTURE
    DECLARE_GL_FUNCTION( glBindImageTexture, PFNGLBINDIMAGETEXTUREPROC, GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format )
#endif

#ifdef LOAD_GL_DISPATCH_COMPUTE
    DECLARE_GL_FUNCTION( glDispatchCompute, PFNGLDISPATCHCOMPUTEPROC, GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z )
#endif

#ifdef LOAD_GL_PROGRAM_UNIFORM_1I
    DECLARE_GL_FUNCTION( glProgramUniform1i, PFNGLPROGRAMUNIFORM1IPROC, GLuint program, GLint location, GLint x )
#endif

#ifdef LOAD_GL_MEMORY_BARRIER
    DECLARE_GL_FUNCTION( glMemoryBarrier, PFNGLMEMORYBARRIERPROC, GLbitfield barriers )
#endif

#ifdef LOAD_DRAW_ELEMENTS_INDIRECT
    DECLARE_GL_FUNCTION( glDrawElementsIndirect, PFNGLDRAWELEMENTSINDIRECTPROC, GLenum mode, GLenum type, const GLvoid *indirect )
#endif

#ifdef LOAD_DRAW_ARRAYS_INDIRECT
    DECLARE_GL_FUNCTION( glDrawArraysIndirect, PFNGLDRAWARRAYSINDIRECTPROC, GLenum mode, const GLvoid *indirect )
#endif

#ifdef LOAD_GEN_PROGRAM_PIPELINES
    DECLARE_GL_FUNCTION( glGenProgramPipelines, PFNGLGENPROGRAMPIPELINESPROC, GLsizei n, GLuint* pipelines )
#endif

#ifdef LOAD_GL_DELETE_PROGRAM_PIPELINES
    DECLARE_GL_FUNCTION( glDeleteProgramPipelines, PFNGLDELETEPROGRAMPIPELINESPROC, GLsizei n, const GLuint* pipelines )
#endif

#ifdef LOAD_GL_BIND_PROGRAM_PIPELINE
    DECLARE_GL_FUNCTION( glBindProgramPipeline, PFNGLBINDPROGRAMPIPELINEPROC, GLuint pipeline )
#endif

#ifdef LOAD_GL_USE_PROGRAM_STAGES
    DECLARE_GL_FUNCTION( glUseProgramStages, PFNGLUSEPROGRAMSTAGESPROC, GLuint pipeline, GLbitfield stages, GLuint program )
#endif

#ifdef LOAD_GL_TEX_STORAGE_2D_MULTISAMPLE
    DECLARE_GL_FUNCTION( glTexStorage2DMultisample, PFNGLTEXSTORAGE2DMULTISAMPLEPROC, GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations )
#endif

#ifdef LOAD_GL_GET_PROGRAM_INTERFACEIV
    DECLARE_GL_FUNCTION( glGetProgramInterfaceiv, PFNGLGETPROGRAMINTERFACEIVPROC, GLuint program, GLenum programInterface, GLenum pname, GLint* params )
#endif

#ifdef LOAD_GL_GET_PROGRAM_RESOURCE_NAME
    DECLARE_GL_FUNCTION( glGetProgramResourceName, PFNGLGETPROGRAMRESOURCENAMEPROC, GLuint program, GLenum programInterface, GLuint index, GLsizei bufSize, GLsizei* length, GLchar *name )
#endif

#ifdef LOAD_GL_GET_PROGRAM_RESOURCE_INDEX
    PFNGLGETPROGRAMRESOURCEINDEXPROC glGetProgramResourceIndex = nullptr;
    GLuint glGetProgramResourceIndexStub(GLuint program, GLenum programInterface, const GLchar *name)
    {
        UnsupportedGLFunctionStub("glGetProgramResourceIndex");
        return 0;
    }
#endif

#ifdef LOAD_GL_GET_PROGRAM_RESOURCEIV
    DECLARE_GL_FUNCTION( glGetProgramResourceiv, PFNGLGETPROGRAMRESOURCEIVPROC, GLuint program, GLenum programInterface, GLuint index, GLsizei propCount, const GLenum* props, GLsizei bufSize, GLsizei *length, GLint *params )
#endif

#ifdef LOAD_DISPATCH_COMPUTE_INDIRECT
    DECLARE_GL_FUNCTION( glDispatchComputeIndirect, PFNGLDISPATCHCOMPUTEINDIRECTPROC, GLintptr indirect )
#endif

#ifdef LOAD_GL_TEX_BUFFER
    DECLARE_GL_FUNCTION( glTexBuffer, PFNGLTEXBUFFERPROC, GLenum, GLenum, GLuint)
#endif

#ifdef LOAD_GL_POLYGON_MODE
    DECLARE_GL_FUNCTION_NO_STUB( glPolygonMode, PFNGLPOLYGONMODE)
#endif

#ifdef LOAD_GL_BLEND_FUNC_SEPARATEI
    DECLARE_GL_FUNCTION( glBlendFuncSeparatei, PFNGLBLENDFUNCSEPARATEIPROC, GLuint buf, GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha)
#endif

#ifdef LOAD_GL_BLEND_EQUATION_SEPARATEI
    DECLARE_GL_FUNCTION( glBlendEquationSeparatei, PFNGLBLENDEQUATIONSEPARATEIPROC, GLuint buf, GLenum modeRGB, GLenum modeAlpha)
#endif

#ifdef LOAD_GL_ENABLEI
    DECLARE_GL_FUNCTION( glEnablei, PFNGLENABLEIPROC, GLenum, GLuint)
#endif

#ifdef LOAD_GL_DISABLEI
    DECLARE_GL_FUNCTION( glDisablei, PFNGLDISABLEIPROC, GLenum, GLuint)
#endif

#ifdef LOAD_GL_COLOR_MASKI
    DECLARE_GL_FUNCTION( glColorMaski, PFNGLCOLORMASKIPROC, GLuint, GLboolean, GLboolean, GLboolean, GLboolean)
#endif

#ifdef LOAD_GL_VIEWPORT_INDEXEDF
    PFNGLVIEWPORTINDEXEDFPROC glViewportIndexedf = nullptr;
    void glViewportIndexedfStub(GLuint index, GLfloat x, GLfloat y, GLfloat w, GLfloat h)
    {
        (void)index;
        glViewport(static_cast<GLint>(x), static_cast<GLint>(y), static_cast<GLsizei>(w), static_cast<GLsizei>(h));
    }
#endif

#ifdef LOAD_GL_SCISSOR_INDEXED
    PFNGLSCISSORINDEXEDPROC glScissorIndexed = nullptr;
    void glScissorIndexedStub(GLuint index, GLint left, GLint bottom, GLsizei width, GLsizei height)
    {
        (void)index;
        glScissor(static_cast<GLint>(left), static_cast<GLint>(bottom), static_cast<GLsizei>(width), static_cast<GLsizei>(height));
    }
#endif

#ifdef LOAD_GL_DEPTH_RANGE_INDEXED
    PFNGLDEPTHRANGEINDEXEDPROC glDepthRangeIndexed = nullptr;
    void glDepthRangeIndexedStub(GLuint index, GLfloat n, GLfloat f)
    {
        (void)index;
        glDepthRangef(n, f);
    }
#endif

#ifdef LOAD_GL_FRAMEBUFFER_TEXTURE
    DECLARE_GL_FUNCTION( glFramebufferTexture, PFNGLFRAMEBUFFERTEXTUREPROC, GLenum, GLenum, GLuint, GLint)
#endif

#ifdef LOAD_GL_FRAMEBUFFER_TEXTURE_1D
    DECLARE_GL_FUNCTION( glFramebufferTexture1D, PFNGLFRAMEBUFFERTEXTURE1DPROC, GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level)
#endif

#ifdef LOAD_GL_COPY_TEX_SUBIMAGE_1D
    DECLARE_GL_FUNCTION( glCopyTexSubImage1D, PFNGLCOPYTEXSUBIMAGE1DEXTPROC, GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width)
#endif

#ifdef LOAD_GL_FRAMEBUFFER_TEXTURE_3D
    DECLARE_GL_FUNCTION( glFramebufferTexture3D, PFNGLFRAMEBUFFERTEXTURE3DPROC, GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint layer )
#endif

#ifdef LOAD_GL_COPY_IMAGE_SUB_DATA
    DECLARE_GL_FUNCTION( glCopyImageSubData, PFNGLCOPYIMAGESUBDATAPROC, GLuint srcName, GLenum srcTarget, GLint srcLevel, GLint srcX, GLint srcY, GLint srcZ, GLuint dstName, GLenum dstTarget, GLint dstLevel, GLint dstX, GLint dstY, GLint dstZ, GLsizei srcWidth, GLsizei srcHeight, GLsizei srcDepth )
#endif

#ifdef LOAD_GL_PATCH_PARAMTER_I
    DECLARE_GL_FUNCTION( glPatchParameteri, PFNGLPATCHPARAMETERIPROC, GLenum pname, GLint value )
#endif

#ifdef LOAD_GET_TEX_LEVEL_PARAMETER_IV
    DECLARE_GL_FUNCTION( glGetTexLevelParameteriv, PFNGLGETTEXLEVELPARAMETERIVPROC, GLenum target, GLint level, GLenum pname, GLint *params )
#endif

#ifdef LOAD_GL_SHADER_STORAGE_BLOCK_BINDING
    DECLARE_GL_FUNCTION_NO_STUB( glShaderStorageBlockBinding, PFNGLSHADERSTORAGEBLOCKBINDINGPROC, GLuint program, GLuint storageBlockIndex, GLuint storageBlockBinding )
#endif

#ifdef LOAD_GL_TEX_STORAGE_3D_MULTISAMPLE
    DECLARE_GL_FUNCTION( glTexStorage3DMultisample, PFNGLTEXSTORAGE3DMULTISAMPLEPROC, GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedsamplelocations )
#endif

#ifdef LOAD_GL_TEXTURE_VIEW
    DECLARE_GL_FUNCTION_NO_STUB( glTextureView, PFNGLTEXTUREVIEWPROC, GLuint texture, GLenum target, GLuint origtexture, GLenum internalformat, GLuint minlevel, GLuint numlevels, GLuint minlayer, GLuint numlayers)
#endif

#ifdef LOAD_GL_DRAW_ELEMENTS_INSTANCED_BASE_VERTEX_BASE_INSTANCE
    DECLARE_GL_FUNCTION( glDrawElementsInstancedBaseVertexBaseInstance, PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXBASEINSTANCEPROC, GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount, GLint basevertex, GLuint baseinstance)
#endif

#ifdef LOAD_GL_DRAW_ELEMENTS_INSTANCED_BASE_VERTEX
    DECLARE_GL_FUNCTION( glDrawElementsInstancedBaseVertex, PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXPROC, GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount, GLint basevertex)
#endif

#ifdef LOAD_GL_DRAW_ELEMENTS_INSTANCED_BASE_INSTANCE
    DECLARE_GL_FUNCTION( glDrawElementsInstancedBaseInstance, PFNGLDRAWELEMENTSINSTANCEDBASEINSTANCEPROC, GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount, GLuint baseinstance)
#endif

#ifdef LOAD_GL_DRAW_ARRAYS_INSTANCED_BASE_INSTANCE
    DECLARE_GL_FUNCTION( glDrawArraysInstancedBaseInstance, PFNGLDRAWARRAYSINSTANCEDBASEINSTANCEPROC, GLenum mode, GLint first, GLsizei count, GLsizei instancecount, GLuint baseinstance)
#endif

#ifdef LOAD_GL_DRAW_ELEMENTS_BASE_VERTEX
    DECLARE_GL_FUNCTION( glDrawElementsBaseVertex, PFNGLDRAWELEMENTSBASEVERTEXPROC, GLenum mode, GLsizei count, GLenum type, const void *indices, GLint basevertex)
#endif

#ifdef LOAD_DEBUG_MESSAGE_CALLBACK
    DECLARE_GL_FUNCTION( glDebugMessageCallback, PFNGLDEBUGMESSAGECALLBACKPROC, GLDEBUGPROC callback, const void *userParam)
#endif

#ifdef LOAD_DEBUG_MESSAGE_CONTROL
    DECLARE_GL_FUNCTION( glDebugMessageControl, PFNGLDEBUGMESSAGECONTROLPROC, GLenum source, GLenum type, GLenum severity, GLsizei count, const GLuint* ids, GLboolean enabled);
#endif

#ifdef LOAD_GL_GET_QUERY_OBJECT_UI64V
    DECLARE_GL_FUNCTION( glGetQueryObjectui64v, PFNGLGETQUERYOBJECTUI64VPROC, GLuint id, GLenum pname, GLuint64* params)
#endif

#ifdef LOAD_GL_QUERY_COUNTER
    DECLARE_GL_FUNCTION( glQueryCounter, PFNGLQUERYCOUNTERPROC, GLuint id, GLenum target)
#endif

#ifdef LOAD_GL_OBJECT_LABEL
    DECLARE_GL_FUNCTION_NO_STUB( glObjectLabel, PFNGLOBJECTLABELPROC)
#endif

#ifdef LOAD_GL_POP_DEBUG_GROUP
    DECLARE_GL_FUNCTION_NO_STUB( glPopDebugGroup, PFNGLPOPDEBUGGROUPPROC)
#endif

#ifdef LOAD_GL_PUSH_DEBUG_GROUP
    DECLARE_GL_FUNCTION_NO_STUB( glPushDebugGroup, PFNGLPUSHDEBUGGROUPPROC)
#endif

#ifdef LOAD_GL_DEBUG_MESSAGE_INSERT
    DECLARE_GL_FUNCTION_NO_STUB( glDebugMessageInsert, PFNGLDEBUGMESSAGEINSERTPROC)
#endif

#ifdef LOAD_GL_CLIP_CONTROL
    DECLARE_GL_FUNCTION_NO_STUB( glClipControl, PFNGLCLIPCONTROLPROC)
#endif

#ifdef LOAD_GL_MULTIDRAW_ARRAYS_INDIRECT
    DECLARE_GL_FUNCTION( glMultiDrawArraysIndirect, PFNGLMULTIDRAWARRAYSINDIRECTPROC, GLenum mode, const void *indirect, GLsizei primcount, GLsizei stride)
#endif

#ifdef LOAD_GL_MULTIDRAW_ELEMENTS_INDIRECT
    DECLARE_GL_FUNCTION( glMultiDrawElementsIndirect, PFNGLMULTIDRAWELEMENTSINDIRECTPROC, GLenum mode, GLenum type, const void *indirect, GLsizei primcount, GLsizei stride)
#endif

#ifdef LOAD_GL_MULTI_DRAW_ARRAYS
    DECLARE_GL_FUNCTION( glMultiDrawArrays, PFNGLMULTIDRAWARRAYSPROC, GLenum mode, const GLint *first, const GLsizei *count, GLsizei drawcount)
#endif

#ifdef LOAD_GL_MULTI_DRAW_ELEMENTS
	DECLARE_GL_FUNCTION( glMultiDrawElements, PFNGLMULTIDRAWELEMENTSPROC, GLenum mode, const GLsizei *count, GLenum type, const void *const*indices, GLsizei drawcount)
#endif

void LoadGLFunctions()
{
    Diligent::Version glesVer{3, 0};
    int MajorVersion = 0, MinorVersion = 0;
    glGetIntegerv(GL_MAJOR_VERSION, &MajorVersion);
    if (glGetError() == GL_NO_ERROR)
    {
        glGetIntegerv(GL_MINOR_VERSION, &MinorVersion);
        if (glGetError() == GL_NO_ERROR)
            glesVer = {static_cast<Diligent::Uint32>(MajorVersion), static_cast<Diligent::Uint32>(MinorVersion)};
    }
    VERIFY_EXPR(glesVer >= Diligent::Version(3, 0));

    struct FuncNameAndVersion
    {
        const char*       Name;
        Diligent::Version Ver;
    };
    const auto LoadFn = [glesVer](auto& Fn, std::initializer_list<FuncNameAndVersion> FnNames, auto* FnStub) //
    {
        for (auto& NameAndVer : FnNames)
        {
            if (glesVer < NameAndVer.Ver)
                continue;

            Fn = reinterpret_cast<std::remove_reference_t<decltype(Fn)>>(eglGetProcAddress(NameAndVer.Name));
            if (Fn != nullptr)
                break;
        }
        if (Fn == nullptr)
            Fn = FnStub;
    };

#define LOAD_GL_FUNCTION(Func) \
    LoadFn(Func, {{#Func, {3,0}}}, &Func##Stub);

#define LOAD_GL_FUNCTION2(Func, ...) \
    LoadFn(Func, __VA_ARGS__, &Func##Stub);

#define LOAD_GL_FUNCTION_NO_STUB(Func, ...) \
    LoadFn(Func, __VA_ARGS__, (decltype(Func))nullptr);


#ifdef LOAD_GL_BIND_IMAGE_TEXTURE
    LOAD_GL_FUNCTION(glBindImageTexture)
#endif

#ifdef LOAD_GL_DISPATCH_COMPUTE
    LOAD_GL_FUNCTION(glDispatchCompute)
#endif

#ifdef LOAD_DISPATCH_COMPUTE_INDIRECT
    LOAD_GL_FUNCTION(glDispatchComputeIndirect)
#endif

#ifdef LOAD_GEN_PROGRAM_PIPELINES
    LOAD_GL_FUNCTION2(glGenProgramPipelines, {{"glGenProgramPipelines", {3,1}}, {"glGenProgramPipelinesEXT", {3,0}}} )
#endif

#ifdef LOAD_GL_DELETE_PROGRAM_PIPELINES
    LOAD_GL_FUNCTION2(glDeleteProgramPipelines, {{"glDeleteProgramPipelines", {3,1}}, {"glDeleteProgramPipelinesEXT", {3,0}}} )
#endif

#ifdef LOAD_GL_BIND_PROGRAM_PIPELINE
    LOAD_GL_FUNCTION2(glBindProgramPipeline, {{"glBindProgramPipeline", {3,1}}, {"glBindProgramPipelineEXT", {3,0}}} )
#endif

#ifdef LOAD_GL_USE_PROGRAM_STAGES
    LOAD_GL_FUNCTION2(glUseProgramStages, {{"glUseProgramStages", {3,1}}, {"glUseProgramStagesEXT", {3,0}}} )
#endif

#ifdef LOAD_GL_PROGRAM_UNIFORM_1I
    LOAD_GL_FUNCTION2(glProgramUniform1i, {{"glProgramUniform1i", {3,1}}, {"glProgramUniform1iEXT", {3,0}}} )
#endif

#ifdef LOAD_GL_MEMORY_BARRIER
    LOAD_GL_FUNCTION(glMemoryBarrier)
#endif

#ifdef LOAD_DRAW_ELEMENTS_INDIRECT
    LOAD_GL_FUNCTION(glDrawElementsIndirect)
#endif

#ifdef LOAD_DRAW_ARRAYS_INDIRECT
    LOAD_GL_FUNCTION(glDrawArraysIndirect)
#endif

#ifdef LOAD_GL_TEX_STORAGE_2D_MULTISAMPLE
    LOAD_GL_FUNCTION(glTexStorage2DMultisample)
#endif

#ifdef LOAD_GL_GET_PROGRAM_INTERFACEIV
    LOAD_GL_FUNCTION_NO_STUB(glGetProgramInterfaceiv, {{"glGetProgramInterfaceiv", {3,1}}} )
#endif

#ifdef LOAD_GL_GET_PROGRAM_RESOURCE_NAME
    LOAD_GL_FUNCTION(glGetProgramResourceName)
#endif

#ifdef LOAD_GL_GET_PROGRAM_RESOURCE_INDEX
    LOAD_GL_FUNCTION(glGetProgramResourceIndex)
#endif

#ifdef LOAD_GL_GET_PROGRAM_RESOURCEIV
    LOAD_GL_FUNCTION(glGetProgramResourceiv)
#endif

#ifdef LOAD_GL_TEX_BUFFER
    LOAD_GL_FUNCTION2(glTexBuffer, {{"glTexBuffer", {3,2}}, {"glTexBufferOES", {3,1}}, {"glTexBufferEXT", {3,1}}} )
#endif

#ifdef LOAD_GL_POLYGON_MODE
    LOAD_GL_FUNCTION_NO_STUB(glPolygonMode, {{"glPolygonModeNV", {3,1}}} );
#endif

#ifdef LOAD_GL_BLEND_FUNC_SEPARATEI
    LOAD_GL_FUNCTION2(glBlendFuncSeparatei, {{"glBlendFuncSeparatei", {3,2}}, {"glBlendFuncSeparateiOES", {3,0}}, {"glBlendFuncSeparateiEXT", {3,0}}} )
#endif

#ifdef LOAD_GL_BLEND_EQUATION_SEPARATEI
    LOAD_GL_FUNCTION2(glBlendEquationSeparatei, {{"glBlendEquationSeparatei", {3,2}}, {"glBlendEquationSeparateiOES", {3,0}}, {"glBlendEquationSeparateiEXT", {3,0}}} )
#endif

#ifdef LOAD_GL_ENABLEI
    LOAD_GL_FUNCTION2(glEnablei, {{"glEnablei", {3,2}}, {"glEnableiOES", {3,0}}, {"glEnableiEXT", {3,0}}} )
#endif

#ifdef LOAD_GL_DISABLEI
    LOAD_GL_FUNCTION2(glDisablei, {{"glDisablei", {3,2}}, {"glDisableiOES", {3,0}}, {"glDisableiEXT", {3,0}}} )
#endif

#ifdef LOAD_GL_COLOR_MASKI
    LOAD_GL_FUNCTION2(glColorMaski, {{"glColorMaski", {3,2}}, {"glColorMaskiOES", {3,0}}, {"glColorMaskiEXT", {3,0}}} )
#endif

#ifdef LOAD_GL_VIEWPORT_INDEXEDF
    LOAD_GL_FUNCTION2(glViewportIndexedf, {{"glViewportIndexedfOES", {3,2}}, {"glViewportIndexedfNV", {3,1}}} )
#endif

#ifdef LOAD_GL_SCISSOR_INDEXED
    LOAD_GL_FUNCTION2(glScissorIndexed, {{"glScissorIndexedOES", {3,2}}, {"glScissorIndexedNV", {3,1}}} )
#endif

#ifdef LOAD_GL_DEPTH_RANGE_INDEXED
    LOAD_GL_FUNCTION2(glDepthRangeIndexed, {{"glDepthRangeIndexedfOES", {3,2}}, {"glDepthRangeIndexedfNV", {3,1}}} )
#endif

#ifdef LOAD_GL_FRAMEBUFFER_TEXTURE
    LOAD_GL_FUNCTION2(glFramebufferTexture, {{"glFramebufferTexture", {3,2}}, {"glFramebufferTextureOES", {3,1}}, {"glFramebufferTextureEXT", {3,1}}} )
#endif

#ifdef LOAD_GL_FRAMEBUFFER_TEXTURE_1D
    LOAD_GL_FUNCTION(glFramebufferTexture1D)
#endif

#ifdef LOAD_GL_COPY_TEX_SUBIMAGE_1D
    LOAD_GL_FUNCTION(glCopyTexSubImage1D)
#endif

#ifdef LOAD_GL_FRAMEBUFFER_TEXTURE_3D
    LOAD_GL_FUNCTION2(glFramebufferTexture3D, {{"glFramebufferTexture3DOES", {3,0}}} )
#endif

#ifdef LOAD_GL_COPY_IMAGE_SUB_DATA
    LOAD_GL_FUNCTION_NO_STUB(glCopyImageSubData, {{"glCopyImageSubData", {3,2}}, {"glCopyImageSubDataOES", {3,0}}, {"glCopyImageSubDataEXT", {3,0}}} )
#endif

#ifdef LOAD_GL_PATCH_PARAMTER_I
    LOAD_GL_FUNCTION2(glPatchParameteri, {{"glPatchParameteri", {3,2}}, {"glPatchParameteriOES", {3,1}}, {"glPatchParameteriEXT", {3,1}}} )
#endif

#ifdef LOAD_GET_TEX_LEVEL_PARAMETER_IV
    LOAD_GL_FUNCTION(glGetTexLevelParameteriv)
#endif

#ifdef LOAD_GL_SHADER_STORAGE_BLOCK_BINDING
    //LOAD_GL_FUNCTION_NO_STUB(glShaderStorageBlockBinding, "glShaderStorageBlockBinding")
#endif

#ifdef LOAD_GL_TEX_STORAGE_3D_MULTISAMPLE
    LOAD_GL_FUNCTION2(glTexStorage3DMultisample, {{"glTexStorage3DMultisample", {3,2}}, {"glTexStorage3DMultisampleOES", {3,1}}} )
#endif

#ifdef LOAD_GL_TEXTURE_VIEW
    LOAD_GL_FUNCTION_NO_STUB(glTextureView, {{"glTextureViewOES", {3,1}}, {"glTextureViewEXT", {3,1}}} )
#endif

#ifdef LOAD_GL_DRAW_ELEMENTS_INSTANCED_BASE_VERTEX_BASE_INSTANCE
    LOAD_GL_FUNCTION2(glDrawElementsInstancedBaseVertexBaseInstance, {{"glDrawElementsInstancedBaseVertexBaseInstanceEXT", {3,0}}} )
#endif

#ifdef LOAD_GL_DRAW_ELEMENTS_INSTANCED_BASE_VERTEX
    LOAD_GL_FUNCTION2(glDrawElementsInstancedBaseVertex, {{"glDrawElementsInstancedBaseVertexEXT", {3,0}}, {"glDrawElementsInstancedBaseVertexOES", {3,0}}} )
#endif

#ifdef LOAD_GL_DRAW_ELEMENTS_INSTANCED_BASE_INSTANCE
    LOAD_GL_FUNCTION2(glDrawElementsInstancedBaseInstance, {{"glDrawElementsInstancedBaseInstanceEXT", {3,0}}} )
#endif

#ifdef LOAD_GL_DRAW_ARRAYS_INSTANCED_BASE_INSTANCE
    LOAD_GL_FUNCTION2(glDrawArraysInstancedBaseInstance, {{"glDrawArraysInstancedBaseInstanceEXT", {3,0}}} )
#endif

#ifdef LOAD_GL_DRAW_ELEMENTS_BASE_VERTEX
    LOAD_GL_FUNCTION2(glDrawElementsBaseVertex, {{"glDrawElementsBaseVertex", {3,2}}, {"glDrawElementsBaseVertexOES", {3,0}}, {"glDrawElementsBaseVertexEXT", {3,0}}} )
#endif

#ifdef LOAD_GL_GET_QUERY_OBJECT_UI64V
    LOAD_GL_FUNCTION_NO_STUB(glGetQueryObjectui64v, {{"glGetQueryObjectui64vEXT", {3,0}}} );
#endif

#ifdef LOAD_GL_QUERY_COUNTER
    LOAD_GL_FUNCTION_NO_STUB(glQueryCounter, {{"glQueryCounterEXT", {3,0}}} );
#endif

#ifdef LOAD_GL_OBJECT_LABEL
    LOAD_GL_FUNCTION_NO_STUB(glObjectLabel, {{"glObjectLabel", {3,2}}, {"glObjectLabelKHR", {3,0}}} );
#endif

#ifdef LOAD_GL_POP_DEBUG_GROUP
    LOAD_GL_FUNCTION_NO_STUB(glPopDebugGroup, {{"glPopDebugGroup", {3,2}}, {"glPopDebugGroupKHR", {3,0}}} );
#endif

#ifdef LOAD_GL_PUSH_DEBUG_GROUP
    LOAD_GL_FUNCTION_NO_STUB(glPushDebugGroup, {{"glPushDebugGroup", {3,2}}, {"glPushDebugGroupKHR", {3,0}}} );
#endif

#ifdef LOAD_GL_DEBUG_MESSAGE_INSERT
    LOAD_GL_FUNCTION_NO_STUB(glDebugMessageInsert, {{"glDebugMessageInsert", {3,2}}, {"glDebugMessageInsertKHR", {3,0}}} );
#endif

#ifdef LOAD_DEBUG_MESSAGE_CALLBACK
    LOAD_GL_FUNCTION_NO_STUB(glDebugMessageCallback, {{"glDebugMessageCallback", {3,2}}, {"glDebugMessageCallbackKHR", {3,0}}} )
#endif

#ifdef LOAD_DEBUG_MESSAGE_CONTROL
    LOAD_GL_FUNCTION_NO_STUB(glDebugMessageControl, {{"glDebugMessageControl", {3,2}}, {"glDebugMessageControlKHR", {3,0}}} );
#endif

#ifdef LOAD_GL_CLIP_CONTROL
    LOAD_GL_FUNCTION_NO_STUB(glClipControl, {{"glClipControlEXT", {3,0}}} );
#endif

#ifdef LOAD_GL_MULTIDRAW_ARRAYS_INDIRECT
    LOAD_GL_FUNCTION_NO_STUB(glMultiDrawArraysIndirect, {{"glMultiDrawArraysIndirectEXT", {3,1}}} );
#endif

#ifdef LOAD_GL_MULTIDRAW_ELEMENTS_INDIRECT
    LOAD_GL_FUNCTION_NO_STUB(glMultiDrawElementsIndirect, {{"glMultiDrawElementsIndirectEXT", {3,1}}} );
#endif

#ifdef LOAD_GL_MULTI_DRAW_ARRAYS
    LOAD_GL_FUNCTION_NO_STUB(glMultiDrawArrays, {{"glMultiDrawArraysEXT", {3,0}}} );
#endif

#ifdef LOAD_GL_MULTI_DRAW_ELEMENTS
	LOAD_GL_FUNCTION_NO_STUB(glMultiDrawElements, {{"glMultiDrawElementsEXT", {3,0}}} );
#endif
}
