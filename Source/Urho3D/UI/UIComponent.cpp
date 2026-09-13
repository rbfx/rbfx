// Copyright (c) 2008-2022 the Urho3D project.
// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/Core/Context.h"
#include "Urho3D/Graphics/BillboardSet.h"
#include "Urho3D/Graphics/Graphics.h"
#include "Urho3D/Graphics/Octree.h"
#include "Urho3D/Graphics/Technique.h"
#include "Urho3D/Graphics/Material.h"
#include "Urho3D/Graphics/Texture2D.h"
#include "Urho3D/Graphics/StaticModel.h"
#include "Urho3D/Graphics/Renderer.h"
#include "Urho3D/Graphics/Camera.h"
#include "Urho3D/Graphics/VertexBuffer.h"
#include "Urho3D/Scene/Scene.h"
#include "Urho3D/Scene/SceneEvents.h"
#include "Urho3D/RenderPipeline/ShaderConsts.h"
#include "Urho3D/Resource/ResourceCache.h"
#include "Urho3D/IO/Log.h"
#include "Urho3D/UI/UI.h"
#include "Urho3D/UI/UIComponent.h"
#include "Urho3D/UI/UIEvents.h"

#include "Urho3D/DebugNew.h"

namespace Urho3D
{

static const int UICOMPONENT_DEFAULT_TEXTURE_SIZE = 512;
static const int UICOMPONENT_MIN_TEXTURE_SIZE = 64;
static const int UICOMPONENT_MAX_TEXTURE_SIZE = 4096;

class UIElement3D : public UIElement
{
    URHO3D_OBJECT(UIElement3D, UIElement);
public:
    /// Construct.
    explicit UIElement3D(Context* context) : UIElement(context) { }
    /// Destruct.
    ~UIElement3D() override = default;
    /// Set UIComponent which is using this element as root element.
    void SetNode(Node* node) { node_ = node; }
    /// Set active viewport through which this element is rendered. If viewport is not set, it defaults to first viewport.
    void SetViewport(Viewport* viewport) { viewport_ = viewport; }
    /// Convert element coordinates to screen coordinates.
    IntVector2 ElementToScreen(const IntVector2& position) override
    {
        URHO3D_LOGERROR("UIElement3D::ElementToScreen is not implemented.");
        return {-1, -1};
    }
    /// Convert screen coordinates to element coordinates.
    IntVector2 ScreenToElement(const IntVector2& screenPos) override
    {
        IntVector2 result(-1, -1);

        if (node_.Expired())
            return result;

        Scene* scene = node_->GetScene();
        auto* model = node_->GetComponent<StaticModel>();
        if (scene == nullptr || model == nullptr)
            return result;

        auto* renderer = GetSubsystem<Renderer>();
        if (renderer == nullptr)
            return result;

        // \todo Always uses the first viewport, in case there are multiple
        auto* octree = scene->GetComponent<Octree>();
        if (viewport_ == nullptr)
            viewport_ = renderer->GetViewportForScene(scene, 0);

        if (viewport_.Expired() || octree == nullptr)
            return result;

        if (viewport_->GetScene() != scene)
        {
            URHO3D_LOGERROR("UIComponent and Viewport set to component's root element belong to different scenes.");
            return result;
        }

        Camera* camera = viewport_->GetCamera();

        if (camera == nullptr)
            return result;

        IntRect rect = viewport_->GetRect();
        if (rect == IntRect::ZERO)
        {
            auto* graphics = GetSubsystem<Graphics>();
            rect.right_ = graphics->GetWidth();
            rect.bottom_ = graphics->GetHeight();
        }

        auto* ui = GetSubsystem<UI>();

        // Convert to system mouse position
        IntVector2 pos;
        pos = ui->ConvertUIToSystem(screenPos);

        Ray ray(camera->GetScreenRay((float)pos.x_ / rect.Width(), (float)pos.y_ / rect.Height()));
        ea::vector<RayQueryResult> queryResultVector;
        RayOctreeQuery query(queryResultVector, ray, RAY_TRIANGLE_UV, M_INFINITY, DRAWABLE_GEOMETRY, DEFAULT_VIEWMASK);

        octree->Raycast(query);

        if (queryResultVector.empty())
            return result;

        for (unsigned i = 0; i < queryResultVector.size(); i++)
        {
            RayQueryResult& queryResult = queryResultVector[i];
            if (queryResult.drawable_ != model)
            {
                // ignore billboard sets by default
                if (queryResult.drawable_->IsInstanceOf<BillboardSet>())
                    continue;
                return result;
            }

            Vector2& uv = queryResult.textureUV_;
            result = IntVector2(static_cast<int>(uv.x_ * GetWidth()),
                static_cast<int>(uv.y_ * GetHeight()));

            // Convert back to scaled UI position
            result = ui->ConvertSystemToUI(result);

            return result;
        }
        return result;
    }

protected:
    /// A UIComponent which owns this element.
    WeakPtr<Node> node_;
    /// Viewport which renders this element.
    WeakPtr<Viewport> viewport_;
};

UIComponent::UIComponent(Context* context)
    : Component(context),
    viewportIndex_(0)
{
    texture_ = MakeShared<Texture2D>(context_);
    texture_->SetFilterMode(FILTER_BILINEAR);
    texture_->SetAddressMode(TextureCoordinate::U, ADDRESS_CLAMP);
    texture_->SetAddressMode(TextureCoordinate::V, ADDRESS_CLAMP);
    texture_->SetNumLevels(1);                                        // No mipmaps
    if (texture_->SetSize(UICOMPONENT_DEFAULT_TEXTURE_SIZE, UICOMPONENT_DEFAULT_TEXTURE_SIZE,
            TextureFormat::TEX_FORMAT_RGBA8_UNORM, TextureFlag::BindRenderTarget))
        texture_->GetRenderSurface()->SetUpdateMode(SURFACE_MANUALUPDATE);
    else
        URHO3D_LOGERROR("Resizing of UI rendertarget texture failed.");

    rootElement_ = MakeShared<UIElement3D>(context_);
    rootElement_->SetTraversalMode(TM_BREADTH_FIRST);
    rootElement_->SetEnabled(true);
    rootElement_->SetSize(UICOMPONENT_DEFAULT_TEXTURE_SIZE, UICOMPONENT_DEFAULT_TEXTURE_SIZE);

    rootModalElement_ = MakeShared<UIElement3D>(context_);
    rootModalElement_->SetTraversalMode(TM_BREADTH_FIRST);
    rootModalElement_->SetEnabled(true);
    rootModalElement_->SetSize(UICOMPONENT_DEFAULT_TEXTURE_SIZE, UICOMPONENT_DEFAULT_TEXTURE_SIZE);

    offScreenUI_ = new UI(context_);
    offScreenUI_->SetRoot(rootElement_);
    offScreenUI_->SetRootModalElement(rootModalElement_);
    offScreenUI_->SetRenderTarget(texture_);

    material_ = MakeShared<Material>(context_);
    material_->SetTechnique(0, GetSubsystem<ResourceCache>()->GetResource<Technique>("Techniques/Diff.xml"));
    material_->SetTexture(ShaderResources::Albedo, texture_);
}

UIComponent::~UIComponent() = default;

void UIComponent::RegisterObject(Context* context)
{
    context->AddFactoryReflection<UIComponent>();
    context->AddFactoryReflection<UIElement3D>();
}

UIElement* UIComponent::GetRoot() const
{
    return rootElement_;
}

Material* UIComponent::GetMaterial() const
{
    return material_;
}

Texture2D* UIComponent::GetTexture() const
{
    return texture_;
}

void UIComponent::OnNodeSet(Node* previousNode, Node* currentNode)
{
    rootElement_->SetNode(node_);
    if (node_)
    {
        auto* renderer = GetSubsystem<Renderer>();
        auto* model = node_->GetComponent<StaticModel>();
        rootElement_->SetViewport(renderer->GetViewportForScene(GetScene(), viewportIndex_));
        if (model == nullptr)
            model_ = model = node_->CreateComponent<StaticModel>();
        model->SetMaterial(material_);
    }
    else
    {
        if (model_)
        {
            model_->Remove();
            model_ = nullptr;
        }
    }
}

void UIComponent::SetViewportIndex(unsigned int index)
{
    viewportIndex_ = index;
    if (Scene* scene = GetScene())
    {
        auto* renderer = GetSubsystem<Renderer>();
        Viewport* viewport = renderer->GetViewportForScene(scene, index);
        rootElement_->SetViewport(viewport);
    }
}

}
