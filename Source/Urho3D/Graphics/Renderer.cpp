//
// Copyright (c) 2008-2022 the Urho3D project.
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

#include "../Precompiled.h"

#include "../Core/CoreEvents.h"
#include "../Core/Context.h"
#include "../Core/Profiler.h"
#include "../Graphics/Camera.h"
#include "../Graphics/DebugRenderer.h"
#include "../Graphics/Geometry.h"
#include "../Graphics/Graphics.h"
#include "../Graphics/GraphicsEvents.h"
#include "../Graphics/IndexBuffer.h"
#include "../Graphics/Material.h"
#include "../Graphics/OcclusionBuffer.h"
#include "../Graphics/Octree.h"
#include "../Graphics/Renderer.h"
#include "../Graphics/ShaderVariation.h"
#include "../Graphics/Technique.h"
#include "../Graphics/Texture2D.h"
#include "../Graphics/TextureCube.h"
#include "../Graphics/VertexBuffer.h"
#include "../Graphics/Zone.h"
#include "../Input/InputEvents.h"
#include "../IO/Log.h"
#include "../RenderAPI/RenderAPIUtils.h"
#include "../RenderAPI/RenderDevice.h"
#include "../RenderPipeline/RenderPipeline.h"
#include "../Resource/ResourceCache.h"
#include "../Resource/XMLFile.h"
#include "../Scene/Scene.h"
#include "../UI/UI.h"

#include <EASTL/bonus/adaptors.h>
#include <EASTL/functional.h>

#include "../DebugNew.h"

#ifdef _MSC_VER
#pragma warning(disable:6293)
#endif

namespace Urho3D
{

static const float dirLightVertexData[] =
{
    -1, 1, 0,
    1, 1, 0,
    1, -1, 0,
    -1, -1, 0,
};

static const unsigned short dirLightIndexData[] =
{
    0, 1, 2,
    2, 3, 0,
};

static const float pointLightVertexData[] =
{
    -0.423169f, -1.000000f, 0.423169f,
    -0.423169f, -1.000000f, -0.423169f,
    0.423169f, -1.000000f, -0.423169f,
    0.423169f, -1.000000f, 0.423169f,
    0.423169f, 1.000000f, -0.423169f,
    -0.423169f, 1.000000f, -0.423169f,
    -0.423169f, 1.000000f, 0.423169f,
    0.423169f, 1.000000f, 0.423169f,
    -1.000000f, 0.423169f, -0.423169f,
    -1.000000f, -0.423169f, -0.423169f,
    -1.000000f, -0.423169f, 0.423169f,
    -1.000000f, 0.423169f, 0.423169f,
    0.423169f, 0.423169f, -1.000000f,
    0.423169f, -0.423169f, -1.000000f,
    -0.423169f, -0.423169f, -1.000000f,
    -0.423169f, 0.423169f, -1.000000f,
    1.000000f, 0.423169f, 0.423169f,
    1.000000f, -0.423169f, 0.423169f,
    1.000000f, -0.423169f, -0.423169f,
    1.000000f, 0.423169f, -0.423169f,
    0.423169f, -0.423169f, 1.000000f,
    0.423169f, 0.423169f, 1.000000f,
    -0.423169f, 0.423169f, 1.000000f,
    -0.423169f, -0.423169f, 1.000000f
};

static const unsigned short pointLightIndexData[] =
{
    0, 1, 2,
    0, 2, 3,
    4, 5, 6,
    4, 6, 7,
    8, 9, 10,
    8, 10, 11,
    12, 13, 14,
    12, 14, 15,
    16, 17, 18,
    16, 18, 19,
    20, 21, 22,
    20, 22, 23,
    0, 10, 9,
    0, 9, 1,
    13, 2, 1,
    13, 1, 14,
    23, 0, 3,
    23, 3, 20,
    17, 3, 2,
    17, 2, 18,
    21, 7, 6,
    21, 6, 22,
    7, 16, 19,
    7, 19, 4,
    5, 8, 11,
    5, 11, 6,
    4, 12, 15,
    4, 15, 5,
    22, 11, 10,
    22, 10, 23,
    8, 15, 14,
    8, 14, 9,
    12, 19, 18,
    12, 18, 13,
    16, 21, 20,
    16, 20, 17,
    0, 23, 10,
    1, 9, 14,
    2, 13, 18,
    3, 17, 20,
    6, 11, 22,
    5, 15, 8,
    4, 19, 12,
    7, 21, 16
};

static const float spotLightVertexData[] =
{
    0.00001f, 0.00001f, 0.00001f,
    0.00001f, -0.00001f, 0.00001f,
    -0.00001f, -0.00001f, 0.00001f,
    -0.00001f, 0.00001f, 0.00001f,
    1.00000f, 1.00000f, 0.99999f,
    1.00000f, -1.00000f, 0.99999f,
    -1.00000f, -1.00000f, 0.99999f,
    -1.00000f, 1.00000f, 0.99999f,
};

static const unsigned short spotLightIndexData[] =
{
    3, 0, 1,
    3, 1, 2,
    0, 4, 5,
    0, 5, 1,
    3, 7, 4,
    3, 4, 0,
    7, 3, 2,
    7, 2, 6,
    6, 2, 1,
    6, 1, 5,
    7, 5, 4,
    7, 6, 5
};

static const char* geometryVSVariations[] =
{
    "",
    "SKINNED ",
    "INSTANCED ",
    "BILLBOARD ",
    "DIRBILLBOARD ",
    "TRAILFACECAM ",
    "TRAILBONE "
};

static const char* lightVSVariations[] =
{
    "PERPIXEL DIRLIGHT ",
    "PERPIXEL SPOTLIGHT ",
    "PERPIXEL POINTLIGHT ",
    "PERPIXEL DIRLIGHT SHADOW ",
    "PERPIXEL SPOTLIGHT SHADOW ",
    "PERPIXEL POINTLIGHT SHADOW ",
    "PERPIXEL DIRLIGHT SHADOW NORMALOFFSET ",
    "PERPIXEL SPOTLIGHT SHADOW NORMALOFFSET ",
    "PERPIXEL POINTLIGHT SHADOW NORMALOFFSET "
};

static const char* vertexLightVSVariations[] =
{
    "",
    "NUMVERTEXLIGHTS=1 ",
    "NUMVERTEXLIGHTS=2 ",
    "NUMVERTEXLIGHTS=3 ",
    "NUMVERTEXLIGHTS=4 ",
};

static const char* deferredLightVSVariations[] =
{
    "",
    "DIRLIGHT ",
    "ORTHO ",
    "DIRLIGHT ORTHO "
};

static const char* lightPSVariations[] =
{
    "PERPIXEL DIRLIGHT ",
    "PERPIXEL SPOTLIGHT ",
    "PERPIXEL POINTLIGHT ",
    "PERPIXEL POINTLIGHT CUBEMASK ",
    "PERPIXEL DIRLIGHT SPECULAR ",
    "PERPIXEL SPOTLIGHT SPECULAR ",
    "PERPIXEL POINTLIGHT SPECULAR ",
    "PERPIXEL POINTLIGHT CUBEMASK SPECULAR ",
    "PERPIXEL DIRLIGHT SHADOW ",
    "PERPIXEL SPOTLIGHT SHADOW ",
    "PERPIXEL POINTLIGHT SHADOW ",
    "PERPIXEL POINTLIGHT CUBEMASK SHADOW ",
    "PERPIXEL DIRLIGHT SPECULAR SHADOW ",
    "PERPIXEL SPOTLIGHT SPECULAR SHADOW ",
    "PERPIXEL POINTLIGHT SPECULAR SHADOW ",
    "PERPIXEL POINTLIGHT CUBEMASK SPECULAR SHADOW ",
    "PERPIXEL DIRLIGHT SHADOW NORMALOFFSET ",
    "PERPIXEL SPOTLIGHT SHADOW NORMALOFFSET ",
    "PERPIXEL POINTLIGHT SHADOW NORMALOFFSET ",
    "PERPIXEL POINTLIGHT CUBEMASK SHADOW NORMALOFFSET ",
    "PERPIXEL DIRLIGHT SPECULAR SHADOW NORMALOFFSET ",
    "PERPIXEL SPOTLIGHT SPECULAR SHADOW NORMALOFFSET ",
    "PERPIXEL POINTLIGHT SPECULAR SHADOW NORMALOFFSET ",
    "PERPIXEL POINTLIGHT CUBEMASK SPECULAR SHADOW NORMALOFFSET "
};

static const char* heightFogVariations[] =
{
    "",
    "HEIGHTFOG "
};

static const unsigned MAX_BUFFER_AGE = 1000;

static const int MAX_EXTRA_INSTANCING_BUFFER_ELEMENTS = 4;

inline ea::vector<VertexElement> CreateInstancingBufferElements(unsigned numExtraElements)
{
    static const unsigned NUM_INSTANCEMATRIX_ELEMENTS = 3;
    static const unsigned NUM_SHADERPARAMETER_ELEMENTS = 7;
    static const unsigned FIRST_UNUSED_TEXCOORD = 4;

    ea::vector<VertexElement> elements;
    for (unsigned i = 0; i < NUM_INSTANCEMATRIX_ELEMENTS + NUM_SHADERPARAMETER_ELEMENTS + numExtraElements; ++i)
        elements.push_back(VertexElement(TYPE_VECTOR4, SEM_TEXCOORD, FIRST_UNUSED_TEXCOORD + i, true));
    return elements;
}

Renderer::Renderer(Context* context) :
    Object(context),
    defaultZone_(MakeShared<Zone>(context))
{
    SubscribeToEvent(E_SCREENMODE, URHO3D_HANDLER(Renderer, HandleScreenMode));

    // Try to initialize right now, but skip if screen mode is not yet set
    Initialize();
}

Renderer::~Renderer() = default;

void Renderer::SetBackbufferRenderSurface(RenderSurface* renderSurface)
{
    if (backbufferSurface_ != renderSurface)
    {
        if (backbufferSurface_)
            backbufferSurface_->SetNumViewports(0);
        backbufferSurface_ = renderSurface;
        backbufferSurfaceViewportsDirty_ = true;
    }
}

void Renderer::SetNumViewports(unsigned num)
{
    viewports_.resize(num);
}

void Renderer::SetViewport(unsigned index, Viewport* viewport)
{
    if (index >= viewports_.size())
        viewports_.resize(index + 1);

    viewports_[index] = viewport;
}

void Renderer::SetDefaultTechnique(Technique* technique)
{
    defaultTechnique_ = technique;
}

void Renderer::SetTextureAnisotropy(int level)
{
    textureAnisotropy_ = Max(level, 1);
}

void Renderer::SetTextureFilterMode(TextureFilterMode mode)
{
    textureFilterMode_ = mode;
}

void Renderer::SetTextureQuality(MaterialQuality quality)
{
    quality = Clamp(quality, QUALITY_LOW, QUALITY_HIGH);

    if (quality != textureQuality_)
    {
        textureQuality_ = quality;
        ReloadTextures();
    }
}

void Renderer::SetSkinningMode(SkinningMode mode)
{
    skinningMode_ = mode;
}

void Renderer::SetNumSoftwareSkinningBones(unsigned numBones)
{
    numSoftwareSkinningBones_ = numBones;
}

Viewport* Renderer::GetViewport(unsigned index) const
{
    return index < viewports_.size() ? viewports_[index] : nullptr;
}

Viewport* Renderer::GetViewportForScene(Scene* scene, unsigned index) const
{
    for (unsigned i = 0; i < viewports_.size(); ++i)
    {
        Viewport* viewport = viewports_[i];
        if (viewport && viewport->GetScene() == scene)
        {
            if (index == 0)
                return viewport;
            else
                --index;
        }
    }
    return nullptr;
}

Technique* Renderer::GetDefaultTechnique() const
{
    // Assign default when first asked if not assigned yet
    if (!defaultTechnique_)
        const_cast<SharedPtr<Technique>& >(defaultTechnique_) = GetSubsystem<ResourceCache>()->GetResource<Technique>("Techniques/NoTexture.xml");

    return defaultTechnique_;
}

unsigned Renderer::GetNumGeometries() const
{
    unsigned numGeometries = 0;

    for (const RenderPipelineView* view : renderPipelineViews_)
        numGeometries += view ? view->GetStats().numGeometries_ : 0;

    return numGeometries;
}

unsigned Renderer::GetNumLights() const
{
    unsigned numLights = 0;

    for (const RenderPipelineView* view : renderPipelineViews_)
        numLights += view ? view->GetStats().numLights_ : 0;

    return numLights;
}

unsigned Renderer::GetNumShadowMaps() const
{
    unsigned numShadowMaps = 0;

    for (const RenderPipelineView* view : renderPipelineViews_)
        numShadowMaps += view ? view->GetStats().numShadowedLights_ : 0;

    return numShadowMaps;
}

unsigned Renderer::GetNumOccluders() const
{
    unsigned numOccluders = 0;

    for (const RenderPipelineView* view : renderPipelineViews_)
        numOccluders += view ? view->GetStats().numOccluders_ : 0;

    return numOccluders;
}

void Renderer::Update(float timeStep)
{
    URHO3D_PROFILE("UpdateViews");

    renderPipelineViews_.clear();

    // If device lost, do not perform update. This is because any dynamic vertex/index buffer updates happen already here,
    // and if the device is lost, the updates queue up, causing memory use to rise constantly
    if (!graphics_ || !graphics_->IsInitialized())
        return;

    // Set up the frameinfo structure for this frame
    frame_.frameNumber_ = GetSubsystem<Time>()->GetFrameNumber();
    frame_.timeStep_ = timeStep;
    frame_.camera_ = nullptr;
    updatedOctrees_.clear();

    // Assign viewports to the render surface
    if (backbufferSurfaceViewportsDirty_)
    {
        backbufferSurfaceViewportsDirty_ = false;
        if (backbufferSurface_)
        {
            const unsigned numViewports = viewports_.size();
            backbufferSurface_->SetNumViewports(numViewports);
            for (unsigned i = 0; i < numViewports; ++i)
                backbufferSurface_->SetViewport(i, viewports_[i]);
        }
    }

    // Queue update of the main viewports. Use reverse order, as rendering order is also reverse
    // to render auxiliary views before dependent main views
    for (unsigned i = viewports_.size() - 1; i < viewports_.size(); --i)
        QueueViewport(backbufferSurface_, viewports_[i]);

    // Update main viewports. This may queue further views
    unsigned numMainViewports = queuedViewports_.size();
    for (unsigned i = 0; i < numMainViewports; ++i)
        UpdateQueuedViewport(i);

    // Gather queued & autoupdated render surfaces
    SendEvent(E_RENDERSURFACEUPDATE);

    // Update viewports that were added as result of the event above
    for (unsigned i = numMainViewports; i < queuedViewports_.size(); ++i)
        UpdateQueuedViewport(i);

    queuedViewports_.clear();
    resetViews_ = false;
}

void Renderer::Render()
{
    // Engine does not render when window is closed or device is lost
    auto renderDevice = GetSubsystem<RenderDevice>();
    URHO3D_ASSERT(renderDevice);

    URHO3D_PROFILE("RenderViews");

    renderDevice->SetDefaultTextureFilterMode(textureFilterMode_);
    renderDevice->SetDefaultTextureAnisotropy(textureAnisotropy_);

    // If no views that render to the backbuffer, clear the screen so that e.g. the UI is not rendered on top of previous frame
    bool hasBackbufferViews = false;
    for (unsigned i = 0; i < renderPipelineViews_.size(); ++i)
    {
        if (!renderPipelineViews_[i])
            continue;

        if (!renderPipelineViews_[i]->GetFrameInfo().renderTarget_)
        {
            hasBackbufferViews = true;
            break;
        }
    }
    if (!hasBackbufferViews)
    {
        graphics_->ResetRenderTargets();
        graphics_->Clear(CLEAR_COLOR | CLEAR_DEPTH | CLEAR_STENCIL, defaultZone_->GetFogColor());
    }

    // Render RenderPipeline views.
    for (RenderPipelineView* renderPipelineView : ea::reverse(renderPipelineViews_))
        renderPipelineView->Render();

    // All views done, custom rendering can now be done before UI
    SendEvent(E_ENDALLVIEWSRENDER);
}

void Renderer::DrawDebugGeometry(bool depthTest)
{
    URHO3D_PROFILE("RendererDrawDebug");

    for (RenderPipelineView* view : renderPipelineViews_)
    {
        if (!view || !view->GetRenderPipeline()->GetSettings().drawDebugGeometry_)
            continue;

        view->DrawDebugGeometries(depthTest);
        view->DrawDebugLights(depthTest);
    }
}

void Renderer::QueueRenderSurface(RenderSurface* renderTarget)
{
    if (renderTarget)
    {
        unsigned numViewports = renderTarget->GetNumViewports();

        for (unsigned i = 0; i < numViewports; ++i)
            QueueViewport(renderTarget, renderTarget->GetViewport(i));
    }
}

void Renderer::QueueViewport(RenderSurface* renderTarget, Viewport* viewport)
{
    if (viewport)
    {
        ea::pair<WeakPtr<RenderSurface>, WeakPtr<Viewport> > newView =
            ea::make_pair(WeakPtr<RenderSurface>(renderTarget), WeakPtr<Viewport>(viewport));

        // Prevent double add of the same rendertarget/viewport combination
        if (!queuedViewports_.contains(newView))
            queuedViewports_.push_back(newView);
    }
}

Geometry* Renderer::GetLightGeometry(Light* light)
{
    switch (light->GetLightType())
    {
    case LIGHT_DIRECTIONAL:
        return dirLightGeometry_;
    case LIGHT_SPOT:
        return spotLightGeometry_;
    case LIGHT_POINT:
        return pointLightGeometry_;
    }

    return nullptr;
}

Geometry* Renderer::GetQuadGeometry()
{
    return dirLightGeometry_;
}

void Renderer::UpdateQueuedViewport(unsigned index)
{
    WeakPtr<RenderSurface> renderTarget = queuedViewports_[index].first;
    Viewport* viewport = queuedViewports_[index].second;

    // Null pointer means backbuffer view. Differentiate between that and an expired rendersurface
    if ((renderTarget && renderTarget.Expired()) || !viewport || !viewport->GetScene())
        return;

    // (Re)allocate the view structure if necessary
    const bool isInitialized = viewport->GetRenderPipelineView() != nullptr;
    if (!isInitialized || resetViews_)
        viewport->AllocateView();

    RenderPipelineView* renderPipelineView = viewport->GetRenderPipelineView();
    assert(renderPipelineView);

    if (!renderPipelineView->Define(renderTarget, viewport))
        return;

    renderPipelineViews_.push_back(WeakPtr<RenderPipelineView>(renderPipelineView));

    const IntRect& viewRect = viewport->GetRect();
    Scene* scene = viewport->GetScene();
    if (!scene)
        return;

    auto* octree = scene->GetComponent<Octree>();

    // Update octree (perform early update for drawables which need that, and reinsert moved drawables).
    // However, if the same scene is viewed from multiple cameras, update the octree only once
    if (!updatedOctrees_.contains(octree))
    {
        frame_.camera_ = viewport->GetCamera();
        frame_.viewSize_ = viewRect.Size();
        if (frame_.viewSize_ == IntVector2::ZERO)
            frame_.viewSize_ = IntVector2(graphics_->GetWidth(), graphics_->GetHeight());
        octree->Update(frame_);
        updatedOctrees_.insert(octree);

        // Set also the view for the debug renderer already here, so that it can use culling
        /// \todo May result in incorrect debug geometry culling if the same scene is drawn from multiple viewports
        auto* debug = scene->GetComponent<DebugRenderer>();
        if (debug && viewport->GetDrawDebug())
            debug->SetView(viewport->GetCamera());
    }

    // Update view. This may queue further views. View will send update begin/end events once its state is set
    renderPipelineView->Update(frame_);
}

void Renderer::Initialize()
{
    auto* graphics = GetSubsystem<Graphics>();
    auto* renderDevice = GetSubsystem<RenderDevice>();
    auto* cache = GetSubsystem<ResourceCache>();

    if (!graphics || !graphics->IsInitialized() || !cache)
        return;

    URHO3D_PROFILE("InitRenderer");

    graphics_ = graphics;

    hardwareSkinningSupported_ = true;

    defaultLightRamp_ = cache->GetResource<Texture2D>("Textures/Ramp.png");
    defaultLightSpot_ = cache->GetResource<Texture2D>("Textures/Spot.png");
    defaultMaterial_ = MakeShared<Material>(context_);

    CreateGeometries();

    viewports_.resize(1);

    initialized_ = true;

    SubscribeToEvent(E_INPUTEND, &Renderer::UpdateMousePositionsForMainViewports);

    SubscribeToEvent(E_RENDERUPDATE, URHO3D_HANDLER(Renderer, HandleRenderUpdate));
    SubscribeToEvent(E_ENDFRAME, [this] { frameStats_ = FrameStatistics(); });

    URHO3D_LOGINFO("Initialized renderer");
}

void Renderer::ReleaseMaterialShaders()
{
    auto* cache = GetSubsystem<ResourceCache>();
    ea::vector<Material*> materials;

    cache->GetResources<Material>(materials);

    for (unsigned i = 0; i < materials.size(); ++i)
        materials[i]->ReleaseShaders();
}

void Renderer::ReloadTextures()
{
    auto* cache = GetSubsystem<ResourceCache>();
    ea::vector<Resource*> textures;

    cache->GetResources(textures, Texture2D::GetTypeStatic());
    for (unsigned i = 0; i < textures.size(); ++i)
        cache->ReloadResource(textures[i]);

    cache->GetResources(textures, TextureCube::GetTypeStatic());
    for (unsigned i = 0; i < textures.size(); ++i)
        cache->ReloadResource(textures[i]);
}

void Renderer::CreateGeometries()
{
    SharedPtr<VertexBuffer> dlvb(MakeShared<VertexBuffer>(context_));
    dlvb->SetDebugName("DirectionalLight");
    dlvb->SetShadowed(true);
    dlvb->SetSize(4, MASK_POSITION);
    dlvb->Update(dirLightVertexData);

    SharedPtr<IndexBuffer> dlib(MakeShared<IndexBuffer>(context_));
    dlib->SetDebugName("DirectionalLight");
    dlib->SetShadowed(true);
    dlib->SetSize(6, false);
    dlib->Update(dirLightIndexData);

    dirLightGeometry_ = MakeShared<Geometry>(context_);
    dirLightGeometry_->SetVertexBuffer(0, dlvb);
    dirLightGeometry_->SetIndexBuffer(dlib);
    dirLightGeometry_->SetDrawRange(TRIANGLE_LIST, 0, dlib->GetIndexCount());

    SharedPtr<VertexBuffer> slvb(MakeShared<VertexBuffer>(context_));
    slvb->SetDebugName("SpotLight");
    slvb->SetShadowed(true);
    slvb->SetSize(8, MASK_POSITION);
    slvb->Update(spotLightVertexData);

    SharedPtr<IndexBuffer> slib(MakeShared<IndexBuffer>(context_));
    slib->SetDebugName("SpotLight");
    slib->SetShadowed(true);
    slib->SetSize(36, false);
    slib->Update(spotLightIndexData);

    spotLightGeometry_ = MakeShared<Geometry>(context_);
    spotLightGeometry_->SetVertexBuffer(0, slvb);
    spotLightGeometry_->SetIndexBuffer(slib);
    spotLightGeometry_->SetDrawRange(TRIANGLE_LIST, 0, slib->GetIndexCount());

    SharedPtr<VertexBuffer> plvb(MakeShared<VertexBuffer>(context_));
    plvb->SetDebugName("PointLight");
    plvb->SetShadowed(true);
    plvb->SetSize(24, MASK_POSITION);
    plvb->Update(pointLightVertexData);

    SharedPtr<IndexBuffer> plib(MakeShared<IndexBuffer>(context_));
    plib->SetDebugName("PointLight");
    plib->SetShadowed(true);
    plib->SetSize(132, false);
    plib->Update(pointLightIndexData);

    pointLightGeometry_ = MakeShared<Geometry>(context_);
    pointLightGeometry_->SetVertexBuffer(0, plvb);
    pointLightGeometry_->SetIndexBuffer(plib);
    pointLightGeometry_->SetDrawRange(TRIANGLE_LIST, 0, plib->GetIndexCount());

    blackCubeMap_ = MakeShared<TextureCube>(context_);
    blackCubeMap_->SetName("BlackCubeMap");
    blackCubeMap_->SetNumLevels(1);
    blackCubeMap_->SetSize(1, TextureFormat::TEX_FORMAT_RGBA8_UNORM);
    blackCubeMap_->SetFilterMode(FILTER_NEAREST);
    const unsigned char blackCubeMapData[4] = { 0, 0, 0, 255 };
    for (unsigned i = 0; i < MAX_CUBEMAP_FACES; ++i)
        blackCubeMap_->SetData((CubeMapFace)i, 0, 0, 0, 1, 1, blackCubeMapData);
}

void Renderer::HandleScreenMode(StringHash eventType, VariantMap& eventData)
{
    if (!initialized_)
        Initialize();
    else
        resetViews_ = true;
}

void Renderer::HandleRenderUpdate(StringHash eventType, VariantMap& eventData)
{
    using namespace RenderUpdate;

    Update(eventData[P_TIMESTEP].GetFloat());
}

void Renderer::UpdateMousePositionsForMainViewports()
{
    auto* ui = GetSubsystem<UI>();

    for (Viewport* viewport : viewports_)
    {
        if (!viewport || !viewport->GetCamera())
            continue;

        const IntRect rect = viewport->GetEffectiveRect(backbufferSurface_, false);
        const IntVector2 mousePosition = ui->GetSystemCursorPosition();

        const auto rectPos = rect.Min().ToVector2();
        const auto rectSizeMinusOne = (rect.Size() - IntVector2::ONE).ToVector2();
        const auto mousePositionNormalized = (mousePosition.ToVector2() - rectPos) / rectSizeMinusOne;

        Camera* camera = viewport->GetCamera();
        camera->SetMousePosition(mousePositionNormalized);
    }
}

}
