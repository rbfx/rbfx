// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../Foundation/StandardFileTypes.h"

#include <Urho3D/Audio/Sound.h>
#include <Urho3D/Graphics/Animation.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/Texture2D.h>
#include <Urho3D/Graphics/Texture2DArray.h>
#include <Urho3D/Graphics/Texture3D.h>
#include <Urho3D/Graphics/TextureCube.h>
#include <Urho3D/RenderPipeline/RenderPath.h>
#include <Urho3D/Resource/BinaryFile.h>
#include <Urho3D/Resource/SerializableResource.h>
#include <Urho3D/Resource/JSONFile.h>
#include <Urho3D/Resource/XMLFile.h>
#include <Urho3D/Scene/PrefabResource.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneResource.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/Utility/AssetPipeline.h>

namespace Urho3D
{

void Foundation_StandardFileTypes(Context* context, Project* project)
{
    project->AddAnalyzeFileCallback([](ResourceFileDescriptor& desc, const AnalyzeFileContext& ctx)
    {
        desc.AddObjectType<BinaryFile>();
        if (ctx.xmlFile_)
            desc.AddObjectType<XMLFile>();
        if (ctx.jsonFile_)
            desc.AddObjectType<JSONFile>();
    });

    project->AddAnalyzeFileCallback([](ResourceFileDescriptor& desc, const AnalyzeFileContext& ctx)
    {
        if (desc.HasExtension({".wav", ".ogg"}))
            desc.AddObjectType<Sound>();
    });

    project->AddAnalyzeFileCallback([](ResourceFileDescriptor& desc, const AnalyzeFileContext& ctx)
    {
        if (desc.HasExtension({".scene"}) || ctx.HasXMLRoot("scene")
            // Support new scene format with legacy extension too
            || (desc.HasExtension(".xml") && ctx.HasXMLRoot(SceneResource::GetXmlRootName())
                && ctx.xmlFile_->GetRoot().HasAttribute("_id")))
        {
            desc.AddObjectType<Scene>();
            desc.AddObjectType<SceneResource>();
        }
    });

    project->AddAnalyzeFileCallback([](ResourceFileDescriptor& desc, const AnalyzeFileContext& ctx)
    {
        if (desc.HasExtension({".material"}) || ctx.HasXMLRoot("material"))
            desc.AddObjectType<Material>();
    });

    project->AddAnalyzeFileCallback([](ResourceFileDescriptor& desc, const AnalyzeFileContext& ctx)
    {
        if (desc.HasExtension({".serializable"}))
            desc.AddObjectType<SerializableResource>();
    });

    project->AddAnalyzeFileCallback([](ResourceFileDescriptor& desc, const AnalyzeFileContext& ctx)
    {
        if (desc.HasExtension({".renderpath"}))
            desc.AddObjectType<RenderPath>();
    });

    project->AddAnalyzeFileCallback([](ResourceFileDescriptor& desc, const AnalyzeFileContext& ctx)
    {
        if (desc.HasExtension({".dds", ".bmp", ".jpg", ".jpeg", ".tga", ".png"}))
        {
            desc.AddObjectType<Texture>();
            desc.AddObjectType<Texture2D>();
        }
        else if (ctx.HasXMLRoot("cubemap"))
        {
            desc.AddObjectType<Texture>();
            desc.AddObjectType<TextureCube>();
        }
        else if (ctx.HasXMLRoot("texture3d"))
        {
            desc.AddObjectType<Texture>();
            desc.AddObjectType<Texture3D>();
        }
        else if (ctx.HasXMLRoot("texturearray"))
        {
            desc.AddObjectType<Texture>();
            desc.AddObjectType<Texture2DArray>();
        }
    });

    project->AddAnalyzeFileCallback([](ResourceFileDescriptor& desc, const AnalyzeFileContext& ctx)
    {
        if (desc.HasExtension({".mdl"}))
        {
            desc.AddObjectType<Model>();
        }
    });

    project->AddAnalyzeFileCallback([](ResourceFileDescriptor& desc, const AnalyzeFileContext& ctx)
    {
        if (desc.HasExtension({".ani"}) || ctx.HasXMLRoot("animation"))
        {
            desc.AddObjectType<Animation>();
        }
    });

    project->AddAnalyzeFileCallback([](ResourceFileDescriptor& desc, const AnalyzeFileContext& ctx)
    {
        if (desc.HasExtension({".assetpipeline", ".AssetPipeline.json"}))
        {
            desc.AddObjectType<AssetPipeline>();
        }
    });

    project->AddAnalyzeFileCallback([](ResourceFileDescriptor& desc, const AnalyzeFileContext& ctx)
    {
        if (desc.HasExtension({".sdf", ".ttf"}))
        {
            desc.AddObjectType<Font>();
        }
    });

    project->AddAnalyzeFileCallback([](ResourceFileDescriptor& desc, const AnalyzeFileContext& ctx)
    {
        if (desc.HasExtension({".prefab"}) || ctx.HasXMLRoot("scene"))
        {
            desc.AddObjectType<PrefabResource>();
        }
    });
}

}
