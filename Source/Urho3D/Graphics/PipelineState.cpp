//
// Copyright (c) 2017-2020 the rbfx project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include "../Graphics/PipelineState.h"

#include "../Core/Context.h"
#include "../Core/Thread.h"
#include "../IO/Log.h"
#include "../Graphics/Geometry.h"
#include "../Graphics/Graphics.h"
#include "../Graphics/Shader.h"
#include "../Resource/ResourceEvents.h"

#include "../IO/VirtualFileSystem.h"
#include "../Engine/EngineEvents.h"

#include "../DebugNew.h"

namespace Urho3D
{
    static ea::string sPipelineStateCacheFileId = "PSCB";
GeometryBufferArray::GeometryBufferArray(const Geometry* geometry, VertexBuffer* instancingBuffer)
    : GeometryBufferArray(geometry->GetVertexBuffers(), geometry->GetIndexBuffer(), instancingBuffer)
{
}

void PipelineStateDesc::InitializeInputLayout(const GeometryBufferArray& buffers)
{
    indexType_ = IndexBuffer::GetIndexBufferType(buffers.indexBuffer_);
    numVertexElements_ = 0;
    for (VertexBuffer* vertexBuffer : buffers.vertexBuffers_)
    {
        if (vertexBuffer)
        {
            const auto& elements = vertexBuffer->GetElements();
            const unsigned numElements = ea::min<unsigned>(elements.size(), MaxNumVertexElements - numVertexElements_);
            if (numElements > 0)
            {
                ea::copy_n(elements.begin(), numElements, vertexElements_.begin() + numVertexElements_);
                numVertexElements_ += numElements;
            }
            if (elements.size() != numElements)
            {
                URHO3D_LOGWARNING("Too many vertex elements: PipelineState cannot handle more than {}", MaxNumVertexElements);
            }
        }
    }
}

void PipelineStateDesc::InitializeInputLayoutAndPrimitiveType(const Geometry* geometry, VertexBuffer* instancingBuffer)
{
    InitializeInputLayout(GeometryBufferArray{ geometry, instancingBuffer });
    primitiveType_ = geometry->GetPrimitiveType();
}

PipelineState::PipelineState(PipelineStateCache* owner)
    : owner_(owner)
{
#ifdef URHO3D_DILIGENT
    pipeline_ = nullptr;
#endif
}

PipelineState::~PipelineState()
{
    if (!Thread::IsMainThread())
    {
        URHO3D_LOGWARNING("Pipeline state should be released only from main thread");
        return;
    }

    // Release all allocated shader resource bindings
    shaderResourceBindings_.clear();

    if (PipelineStateCache* owner = owner_)
        owner->ReleasePipelineState(desc_);
}

ShaderResourceBinding* PipelineState::CreateSRB() {
    ShaderResourceBinding* srb = CreateInternalSRB();
    shaderResourceBindings_.push_back(SharedPtr<ShaderResourceBinding>(srb));
    return srb;
}

void PipelineState::Setup(const PipelineStateDesc& desc)
{
    assert(desc.IsInitialized());
    desc_ = desc;
}

void PipelineState::ResetCachedState()
{
    shaderProgramLayout_ = nullptr;
}

void PipelineState::RestoreCachedState(Graphics* graphics)
{
    if (!shaderProgramLayout_)
        shaderProgramLayout_ = graphics->GetShaderProgramLayout(desc_.vertexShader_, desc_.pixelShader_);
}

bool PipelineState::Apply(Graphics* graphics)
{
#ifndef URHO3D_DILIGENT
    graphics->SetShaders(desc_.vertexShader_, desc_.pixelShader_);

    graphics->SetDepthWrite(desc_.depthWriteEnabled_);
    graphics->SetDepthTest(desc_.depthCompareFunction_);
    graphics->SetStencilTest(desc_.stencilTestEnabled_, desc_.stencilCompareFunction_,
        desc_.stencilOperationOnPassed_, desc_.stencilOperationOnStencilFailed_, desc_.stencilOperationOnDepthFailed_,
        desc_.stencilReferenceValue_, desc_.stencilCompareMask_, desc_.stencilWriteMask_);

    graphics->SetFillMode(desc_.fillMode_);
    graphics->SetCullMode(desc_.cullMode_);
    graphics->SetDepthBias(desc_.constantDepthBias_, desc_.slopeScaledDepthBias_);
    graphics->SetLineAntiAlias(desc_.lineAntiAlias_);

    graphics->SetColorWrite(desc_.colorWriteEnabled_);
    graphics->SetBlendMode(desc_.blendMode_, desc_.alphaToCoverageEnabled_);
    return true;
#else
    if (!BuildPipeline(graphics))
        return false;
    graphics->SetPipelineState(this);
    return true;
#endif
}

PipelineStateCache::PipelineStateCache(Context* context)
    : Object(context)
    , GPUObject(GetSubsystem<Graphics>()),
    init_(false)
{
    SubscribeToEvent(E_RELOADFINISHED, &PipelineStateCache::HandleResourceReload);
}
void PipelineStateCache::Init()
{
    if (this->init_) {
        URHO3D_LOGWARNING("PipelineStateCache has already initialized. Skipping then!");
        return;
    }
    const VirtualFileSystem* vfs = GetSubsystem<VirtualFileSystem>();
    ea::vector<uint8_t> fileData;
    if (vfs->Exists(cacheDir_)) {
        const AbstractFilePtr file = vfs->OpenFile(cacheDir_, FILE_READ);
        // PSCB = Pipeline State Cache Binary
        if (!file || file->ReadFileID() != sPipelineStateCacheFileId) {
            URHO3D_LOGERROR("{} is not a valid pipeline state cache binary file", cacheDir_.ToUri());
        }
        else {
            file->ReadBuffer(fileData);
            URHO3D_LOGDEBUG("Loaded Pipeline State Cache ({:d})", fileData.size());
        }
    }
#ifdef URHO3D_DILIGENT
    CreatePSOCache(fileData);
#endif

    this->init_ = true;
}

void PipelineStateCache::Save()
{
    if(!this->init_)
        return;
    ByteVector psoData;
#ifdef URHO3D_DILIGENT
    ReadPSOData(psoData);
#endif

    VirtualFileSystem* vfs = GetSubsystem<VirtualFileSystem>();

    auto file = vfs->OpenFile(cacheDir_, FILE_WRITE);
    if (!file)
        return;

    file->WriteFileID(sPipelineStateCacheFileId);
    file->WriteBuffer(psoData);
    URHO3D_LOGDEBUG("Pipeline State Cache has been saved ({:d}).", psoData.size());
}

SharedPtr<PipelineState> PipelineStateCache::GetPipelineState(PipelineStateDesc desc)
{
    if (!desc.IsInitialized())
        return nullptr;

    desc.RecalculateHash();

    WeakPtr<PipelineState>& weakPipelineState = states_[desc];
    SharedPtr<PipelineState> pipelineState = weakPipelineState.Lock();
    if (!pipelineState)
    {
        pipelineState = MakeShared<PipelineState>(this);
        pipelineState->Setup(desc);
        weakPipelineState = pipelineState;
    }
    pipelineState->RestoreCachedState(graphics_);
    return pipelineState;
}

void PipelineStateCache::ReleasePipelineState(const PipelineStateDesc& desc)
{
    if (states_.erase(desc) != 1)
        URHO3D_LOGERROR("Unexpected call of PipelineStateCache::ReleasePipelineState");
}

void PipelineStateCache::OnDeviceLost()
{
    for (const auto& item : states_)
    {
        if (SharedPtr<PipelineState> pipelineState = item.second.Lock())
            pipelineState->ResetCachedState();
    }
}

void PipelineStateCache::OnDeviceReset()
{
    for (const auto& item : states_)
    {
        if (SharedPtr<PipelineState> pipelineState = item.second.Lock())
            pipelineState->RestoreCachedState(graphics_);
    }
}

void PipelineStateCache::Release()
{
    OnDeviceLost();
}

void PipelineStateCache::HandleResourceReload(StringHash /*eventType*/, VariantMap& /*eventData*/)
{
    if (context_->GetEventSender()->GetType() == Shader::GetTypeStatic())
    {
        for (const auto& item : states_)
        {
            if (SharedPtr<PipelineState> pipelineState = item.second.Lock())
                pipelineState->RestoreCachedState(graphics_);
        }
    }
}

void PipelineStateCache::SetCacheDir(const FileIdentifier& path) {
    assert(!init_);
    cacheDir_ = path;
}

}
