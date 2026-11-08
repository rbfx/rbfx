// Copyright (c) 2026 the rbfx project.
// This work is licensed under the terms of the MIT license.

#include "AssetViewer.h"

namespace Urho3D
{

using namespace AssetViewerDetail;

void AssetViewer::DrawOverview()
{
    File file(context_, sourceFile_, FILE_READ);
    DrawOverviewSummary(file.GetSize());
    EndProperties();
    ui::Spacing();

    const auto section = [](const char* label, bool defaultOpen = true)
    {
        EndProperties();
        return ui::CollapsingHeader(label, defaultOpen ? ImGuiTreeNodeFlags_DefaultOpen : ImGuiTreeNodeFlags_None);
    };

    if (section("File"))
    {
        PropertyRow("Name", GetFileNameAndExtension(sourceFile_), &SuccessColor);
        Property("Absolute path", sourceFile_);
        Property("Extension", GetExtension(sourceFile_));
        PropertyBytes("File size", file.GetSize());
        if (!convertedFile_.empty())
            Property("Converted glTF", convertedFile_);
        Property("Engine resources", assetResources_.size());
    }
    if (section("Camera"))
    {
        Property("Preview projection", is2DPreview_ ? "Orthographic 2D" : "Perspective 3D");
        Property("Target", cameraTarget_);
        if (is2DPreview_)
            Property("Orthographic size", camera_->GetOrthoSize());
        else
            Property("Distance", distance_);
        Property("Near clip", camera_->GetNearClip());
        Property("Far clip", camera_->GetFarClip());
    }
    const BoundingBox box = GetContentBoundingBox();
    if ((box.Defined() || particleAutoFrameBounds_.Defined()) && section("Bounds"))
    {
        if (box.Defined())
        {
            Property("Min", box.min_);
            Property("Max", box.max_);
            Property("Size", box.Size());
        }
        if (particleAutoFrameBounds_.Defined())
        {
            Property("Particle sample min", particleAutoFrameBounds_.min_);
            Property("Particle sample max", particleAutoFrameBounds_.max_);
            Property("Particle sample size", particleAutoFrameBounds_.Size());
        }
    }
    if (section("Rendering"))
    {
        Property("Rendered geometries", GetSubsystem<Renderer>()->GetNumGeometries());
        Property("Fallback material slots", fallbackMaterialSlots_);
    }
    if (lodLevelCount_ > 1 && section("LOD"))
    {
        Property("Levels", lodLevelCount_);
        Property("Displayed",
            selectedLod_ < 0 ? ea::string("Auto (by camera distance)") : Format("LOD {} of {}", selectedLod_, lodLevelCount_));
    }
    if (gltfModel_ && section("glTF"))
    {
        Property("Version", gltfModel_->asset.version.c_str());
        Property("Generator", gltfModel_->asset.generator.c_str());
        Property("Scenes", gltfModel_->scenes.size());
        Property("Nodes", gltfModel_->nodes.size());
        Property("Meshes", gltfModel_->meshes.size());
        Property("Materials", gltfModel_->materials.size());
        Property("Animations", gltfModel_->animations.size());
        Property("Skins", gltfModel_->skins.size());
    }
    if (!converterOutput_.empty() && InspectorTreeNode("Importer / converter log", ImGuiTreeNodeFlags_SpanAvailWidth))
    {
        EndProperties();
        ui::TextWrapped("%s", converterOutput_.c_str());
        InspectorTreePop();
    }
    EndProperties();
}

void AssetViewer::DrawGLTF()
{
    const tinygltf::Model& model = *gltfModel_;
    if (InspectorTreeNode("Asset", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth))
    {
        Property("Version", model.asset.version.c_str());
        Property("Minimum version", model.asset.minVersion.c_str());
        Property("Generator", model.asset.generator.c_str());
        Property("Copyright", model.asset.copyright.c_str());
        Property("Default scene", model.defaultScene);
        if (!model.extensionsUsed.empty())
        {
            ea::string value;
            for (const auto& extension : model.extensionsUsed)
                value += (value.empty() ? "" : ", ") + ea::string(extension.c_str());
            Property("Extensions used", value);
        }
        if (!model.extensionsRequired.empty())
        {
            ea::string value;
            for (const auto& extension : model.extensionsRequired)
                value += (value.empty() ? "" : ", ") + ea::string(extension.c_str());
            Property("Required", value);
        }
        DrawTinyExtensionsAndExtras(model.asset.extensions, model.asset.extras);
        DrawTinyExtensionsAndExtras(model.extensions, model.extras);
        InspectorTreePop();
    }
    DrawGLTFScenes(model);
    DrawGLTFNodes(model);
    DrawGLTFMeshes(model);
    DrawGLTFMaterials(model);
    DrawGLTFAccessorsAndBuffers(model);
    DrawGLTFTextures(model);
    DrawGLTFSkinsAndAnimations(model);
    DrawGLTFCamerasAndLights(model);
}

void AssetViewer::DrawGLTFScenes(const tinygltf::Model& model)
{
    if (!TreeNode("Scenes", model.scenes.size()))
        return;
    for (unsigned i = 0; i < model.scenes.size(); ++i)
    {
        const auto& scene = model.scenes[i];
        InspectorPushID(i);
        if (InspectorTreeNode("scene", ImGuiTreeNodeFlags_SpanAvailWidth, "%s", NameOrIndex(scene.name, i).c_str()))
        {
            Property("Root nodes", JoinInts(scene.nodes));
            DrawTinyExtensionsAndExtras(scene.extensions, scene.extras);
            InspectorTreePop();
        }
        InspectorPopID();
    }
    InspectorTreePop();
}

void AssetViewer::DrawGLTFNodes(const tinygltf::Model& model)
{
    if (!TreeNode("Nodes", model.nodes.size()))
        return;
    for (unsigned i = 0; i < model.nodes.size(); ++i)
    {
        const auto& node = model.nodes[i];
        InspectorPushID(i);
        if (InspectorTreeNode("node", ImGuiTreeNodeFlags_SpanAvailWidth, "%s", NameOrIndex(node.name, i).c_str()))
        {
            Property("Mesh", node.mesh);
            Property("Skin", node.skin);
            Property("Camera", node.camera);
            Property("Children", JoinInts(node.children));
            if (!node.translation.empty())
                Property("Translation", JoinDoubles(node.translation));
            if (!node.rotation.empty())
                Property("Rotation", JoinDoubles(node.rotation));
            if (!node.scale.empty())
                Property("Scale", JoinDoubles(node.scale));
            if (!node.matrix.empty())
                Property("Matrix", JoinDoubles(node.matrix));
            if (!node.weights.empty())
                Property("Morph weights", JoinDoubles(node.weights));
            DrawTinyExtensionsAndExtras(node.extensions, node.extras);
            InspectorTreePop();
        }
        InspectorPopID();
    }
    InspectorTreePop();
}

void AssetViewer::DrawGLTFMeshes(const tinygltf::Model& model)
{
    if (!TreeNode("Meshes", model.meshes.size()))
        return;
    for (unsigned meshIndex = 0; meshIndex < model.meshes.size(); ++meshIndex)
    {
        const auto& mesh = model.meshes[meshIndex];
        InspectorPushID(meshIndex);
        if (InspectorTreeNode(
                "mesh", ImGuiTreeNodeFlags_SpanAvailWidth, "%s", NameOrIndex(mesh.name, meshIndex).c_str()))
        {
            Property("Default weights", JoinDoubles(mesh.weights));
            if (TreeNode("Primitives", mesh.primitives.size()))
            {
                for (unsigned primitiveIndex = 0; primitiveIndex < mesh.primitives.size(); ++primitiveIndex)
                {
                    const auto& primitive = mesh.primitives[primitiveIndex];
                    InspectorPushID(primitiveIndex);
                    if (InspectorTreeNode(
                            "primitive", ImGuiTreeNodeFlags_SpanAvailWidth, "Primitive %u", primitiveIndex))
                    {
                        Property("Mode", Format("{} ({})", PrimitiveName(primitive.mode), primitive.mode));
                        Property("Indices", primitive.indices);
                        Property("Material", primitive.material);
                        if (TreeNode("Attributes", primitive.attributes.size()))
                        {
                            for (const auto& [semantic, accessor] : primitive.attributes)
                                Property(semantic.c_str(), accessor);
                            InspectorTreePop();
                        }
                        if (TreeNode("Morph targets", primitive.targets.size()))
                        {
                            for (unsigned targetIndex = 0; targetIndex < primitive.targets.size(); ++targetIndex)
                            {
                                InspectorPushID(targetIndex);
                                if (InspectorTreeNode(
                                        "target", ImGuiTreeNodeFlags_SpanAvailWidth, "Target %u", targetIndex))
                                {
                                    for (const auto& [semantic, accessor] : primitive.targets[targetIndex])
                                        Property(semantic.c_str(), accessor);
                                    InspectorTreePop();
                                }
                                InspectorPopID();
                            }
                            InspectorTreePop();
                        }
                        DrawTinyExtensionsAndExtras(primitive.extensions, primitive.extras);
                        InspectorTreePop();
                    }
                    InspectorPopID();
                }
                InspectorTreePop();
            }
            DrawTinyExtensionsAndExtras(mesh.extensions, mesh.extras);
            InspectorTreePop();
        }
        InspectorPopID();
    }
    InspectorTreePop();
}

void AssetViewer::DrawTextureInfo(const char* name, const tinygltf::TextureInfo& info)
{
    if (InspectorTreeNode(name, ImGuiTreeNodeFlags_SpanAvailWidth))
    {
        Property("Texture", info.index);
        Property("TexCoord", info.texCoord);
        DrawTinyExtensionsAndExtras(info.extensions, info.extras);
        InspectorTreePop();
    }
}

void AssetViewer::DrawGLTFMaterials(const tinygltf::Model& model)
{
    if (!TreeNode("Materials", model.materials.size()))
        return;
    for (unsigned i = 0; i < model.materials.size(); ++i)
    {
        const auto& material = model.materials[i];
        InspectorPushID(i);
        if (InspectorTreeNode(
                "material", ImGuiTreeNodeFlags_SpanAvailWidth, "%s", NameOrIndex(material.name, i).c_str()))
        {
            Property("Alpha mode", material.alphaMode.c_str());
            Property("Alpha cutoff", material.alphaCutoff);
            Property("Double sided", material.doubleSided ? "true" : "false");
            Property("Emissive factor", JoinDoubles(material.emissiveFactor));
            if (InspectorTreeNode("PBR metallic-roughness", ImGuiTreeNodeFlags_SpanAvailWidth))
            {
                Property("Base color", JoinDoubles(material.pbrMetallicRoughness.baseColorFactor));
                Property("Metallic", material.pbrMetallicRoughness.metallicFactor);
                Property("Roughness", material.pbrMetallicRoughness.roughnessFactor);
                DrawTextureInfo("Base color texture", material.pbrMetallicRoughness.baseColorTexture);
                DrawTextureInfo(
                    "Metallic/roughness texture", material.pbrMetallicRoughness.metallicRoughnessTexture);
                DrawTinyExtensionsAndExtras(
                    material.pbrMetallicRoughness.extensions, material.pbrMetallicRoughness.extras);
                InspectorTreePop();
            }
            DrawTextureInfo("Emissive texture", material.emissiveTexture);
            if (InspectorTreeNode("Normal texture", ImGuiTreeNodeFlags_SpanAvailWidth))
            {
                Property("Texture", material.normalTexture.index);
                Property("TexCoord", material.normalTexture.texCoord);
                Property("Scale", material.normalTexture.scale);
                DrawTinyExtensionsAndExtras(material.normalTexture.extensions, material.normalTexture.extras);
                InspectorTreePop();
            }
            if (InspectorTreeNode("Occlusion texture", ImGuiTreeNodeFlags_SpanAvailWidth))
            {
                Property("Texture", material.occlusionTexture.index);
                Property("TexCoord", material.occlusionTexture.texCoord);
                Property("Strength", material.occlusionTexture.strength);
                DrawTinyExtensionsAndExtras(material.occlusionTexture.extensions, material.occlusionTexture.extras);
                InspectorTreePop();
            }
            DrawGLTFParameters("Values", material.values);
            DrawGLTFParameters("Additional values", material.additionalValues);
            DrawTinyExtensionsAndExtras(material.extensions, material.extras);
            InspectorTreePop();
        }
        InspectorPopID();
    }
    InspectorTreePop();
}

void AssetViewer::DrawGLTFParameters(const char* label, const tinygltf::ParameterMap& parameters)
{
    if (parameters.empty() || !TreeNode(label, parameters.size()))
        return;
    for (const auto& [name, parameter] : parameters)
    {
        if (InspectorTreeNode(name.c_str(), ImGuiTreeNodeFlags_SpanAvailWidth))
        {
            Property("Bool", parameter.bool_value ? "true" : "false");
            Property("Has number", parameter.has_number_value ? "true" : "false");
            Property("Number", parameter.number_value);
            Property("String", parameter.string_value.c_str());
            Property("Number array", JoinDoubles(parameter.number_array));
            for (const auto& [key, value] : parameter.json_double_value)
                Property(key.c_str(), value);
            InspectorTreePop();
        }
    }
    InspectorTreePop();
}

void AssetViewer::DrawGLTFAccessorsAndBuffers(const tinygltf::Model& model)
{
    if (TreeNode("Accessors", model.accessors.size()))
    {
        for (unsigned i = 0; i < model.accessors.size(); ++i)
        {
            const auto& accessor = model.accessors[i];
            InspectorPushID(i);
            if (InspectorTreeNode(
                    "accessor", ImGuiTreeNodeFlags_SpanAvailWidth, "%s", NameOrIndex(accessor.name, i).c_str()))
            {
                Property("Buffer view", accessor.bufferView);
                Property("Byte offset", accessor.byteOffset);
                Property("Count", accessor.count);
                Property("Type", Format("{} ({})", AccessorTypeName(accessor.type), accessor.type));
                Property("Component",
                    Format("{} ({})", ComponentTypeName(accessor.componentType), accessor.componentType));
                Property("Normalized", accessor.normalized ? "true" : "false");
                Property("Minimum", JoinDoubles(accessor.minValues));
                Property("Maximum", JoinDoubles(accessor.maxValues));
                Property("Sparse", accessor.sparse.isSparse ? "true" : "false");
                if (accessor.sparse.isSparse)
                {
                    Property("Sparse count", accessor.sparse.count);
                    Property("Indices view", accessor.sparse.indices.bufferView);
                    Property("Indices offset", accessor.sparse.indices.byteOffset);
                    Property("Indices component", ComponentTypeName(accessor.sparse.indices.componentType));
                    Property("Values view", accessor.sparse.values.bufferView);
                    Property("Values offset", accessor.sparse.values.byteOffset);
                }
                DrawTinyExtensionsAndExtras(accessor.extensions, accessor.extras);
                InspectorTreePop();
            }
            InspectorPopID();
        }
        InspectorTreePop();
    }
    if (TreeNode("Buffer views", model.bufferViews.size()))
    {
        for (unsigned i = 0; i < model.bufferViews.size(); ++i)
        {
            const auto& view = model.bufferViews[i];
            InspectorPushID(i);
            if (InspectorTreeNode("view", ImGuiTreeNodeFlags_SpanAvailWidth, "%s", NameOrIndex(view.name, i).c_str()))
            {
                Property("Buffer", view.buffer);
                Property("Byte offset", view.byteOffset);
                Property("Byte length", FormatBytes(view.byteLength));
                Property("Byte stride", view.byteStride);
                Property("Target", view.target);
                Property("Draco decoded", view.dracoDecoded ? "true" : "false");
                DrawTinyExtensionsAndExtras(view.extensions, view.extras);
                InspectorTreePop();
            }
            InspectorPopID();
        }
        InspectorTreePop();
    }
    if (TreeNode("Buffers", model.buffers.size()))
    {
        for (unsigned i = 0; i < model.buffers.size(); ++i)
        {
            const auto& buffer = model.buffers[i];
            InspectorPushID(i);
            if (InspectorTreeNode(
                    "buffer", ImGuiTreeNodeFlags_SpanAvailWidth, "%s", NameOrIndex(buffer.name, i).c_str()))
            {
                Property("URI", buffer.uri.c_str());
                Property("Decoded size", FormatBytes(buffer.data.size()));
                DrawTinyExtensionsAndExtras(buffer.extensions, buffer.extras);
                InspectorTreePop();
            }
            InspectorPopID();
        }
        InspectorTreePop();
    }
}

void AssetViewer::DrawGLTFTextures(const tinygltf::Model& model)
{
    if (TreeNode("Images", model.images.size()))
    {
        for (unsigned i = 0; i < model.images.size(); ++i)
        {
            const auto& image = model.images[i];
            InspectorPushID(i);
            if (InspectorTreeNode(
                    "image", ImGuiTreeNodeFlags_SpanAvailWidth, "%s", NameOrIndex(image.name, i).c_str()))
            {
                Property("URI", image.uri.c_str());
                Property("MIME type", image.mimeType.c_str());
                Property("Buffer view", image.bufferView);
                Property("Dimensions", Format("{} x {}", image.width, image.height));
                Property("Components", image.component);
                Property("Bits", image.bits);
                Property("Pixel type", ComponentTypeName(image.pixel_type));
                Property("Decoded bytes", FormatBytes(image.image.size()));
                Property("Stored as-is", image.as_is ? "true" : "false");
                DrawTinyExtensionsAndExtras(image.extensions, image.extras);
                InspectorTreePop();
            }
            InspectorPopID();
        }
        InspectorTreePop();
    }
    if (TreeNode("Textures", model.textures.size()))
    {
        for (unsigned i = 0; i < model.textures.size(); ++i)
        {
            const auto& texture = model.textures[i];
            InspectorPushID(i);
            if (InspectorTreeNode(
                    "texture", ImGuiTreeNodeFlags_SpanAvailWidth, "%s", NameOrIndex(texture.name, i).c_str()))
            {
                Property("Image", texture.source);
                Property("Sampler", texture.sampler);
                DrawTinyExtensionsAndExtras(texture.extensions, texture.extras);
                InspectorTreePop();
            }
            InspectorPopID();
        }
        InspectorTreePop();
    }
    if (TreeNode("Samplers", model.samplers.size()))
    {
        for (unsigned i = 0; i < model.samplers.size(); ++i)
        {
            const auto& sampler = model.samplers[i];
            InspectorPushID(i);
            if (InspectorTreeNode(
                    "sampler", ImGuiTreeNodeFlags_SpanAvailWidth, "%s", NameOrIndex(sampler.name, i).c_str()))
            {
                Property("Min filter", sampler.minFilter);
                Property("Mag filter", sampler.magFilter);
                Property("Wrap S", sampler.wrapS);
                Property("Wrap T", sampler.wrapT);
                DrawTinyExtensionsAndExtras(sampler.extensions, sampler.extras);
                InspectorTreePop();
            }
            InspectorPopID();
        }
        InspectorTreePop();
    }
}

void AssetViewer::DrawGLTFSkinsAndAnimations(const tinygltf::Model& model)
{
    if (TreeNode("Skins", model.skins.size()))
    {
        for (unsigned i = 0; i < model.skins.size(); ++i)
        {
            const auto& skin = model.skins[i];
            InspectorPushID(i);
            if (InspectorTreeNode("skin", ImGuiTreeNodeFlags_SpanAvailWidth, "%s", NameOrIndex(skin.name, i).c_str()))
            {
                Property("Skeleton root", skin.skeleton);
                Property("Inverse binds", skin.inverseBindMatrices);
                Property("Joints", JoinInts(skin.joints));
                DrawTinyExtensionsAndExtras(skin.extensions, skin.extras);
                InspectorTreePop();
            }
            InspectorPopID();
        }
        InspectorTreePop();
    }
    if (TreeNode("Animations", model.animations.size()))
    {
        for (unsigned i = 0; i < model.animations.size(); ++i)
        {
            const auto& animation = model.animations[i];
            InspectorPushID(i);
            if (InspectorTreeNode(
                    "animation", ImGuiTreeNodeFlags_SpanAvailWidth, "%s", NameOrIndex(animation.name, i).c_str()))
            {
                if (TreeNode("Channels", animation.channels.size()))
                {
                    for (unsigned channelIndex = 0; channelIndex < animation.channels.size(); ++channelIndex)
                    {
                        const auto& channel = animation.channels[channelIndex];
                        InspectorPushID(channelIndex);
                        if (InspectorTreeNode(
                                "channel", ImGuiTreeNodeFlags_SpanAvailWidth, "Channel %u", channelIndex))
                        {
                            Property("Sampler", channel.sampler);
                            Property("Target node", channel.target_node);
                            Property("Target path", channel.target_path.c_str());
                            DrawTinyExtensionsAndExtras(channel.extensions, channel.extras);
                            if (!channel.target_extensions.empty()
                                && TreeNode("Target extensions", channel.target_extensions.size()))
                            {
                                for (const auto& [name, value] : channel.target_extensions)
                                    DrawTinyValue(name.c_str(), value);
                                InspectorTreePop();
                            }
                            InspectorTreePop();
                        }
                        InspectorPopID();
                    }
                    InspectorTreePop();
                }
                if (TreeNode("Samplers", animation.samplers.size()))
                {
                    for (unsigned samplerIndex = 0; samplerIndex < animation.samplers.size(); ++samplerIndex)
                    {
                        const auto& sampler = animation.samplers[samplerIndex];
                        InspectorPushID(samplerIndex);
                        if (InspectorTreeNode(
                                "sampler", ImGuiTreeNodeFlags_SpanAvailWidth, "Sampler %u", samplerIndex))
                        {
                            Property("Input accessor", sampler.input);
                            Property("Output accessor", sampler.output);
                            Property("Interpolation", sampler.interpolation.c_str());
                            DrawTinyExtensionsAndExtras(sampler.extensions, sampler.extras);
                            InspectorTreePop();
                        }
                        InspectorPopID();
                    }
                    InspectorTreePop();
                }
                DrawTinyExtensionsAndExtras(animation.extensions, animation.extras);
                InspectorTreePop();
            }
            InspectorPopID();
        }
        InspectorTreePop();
    }
}

void AssetViewer::DrawGLTFCamerasAndLights(const tinygltf::Model& model)
{
    if (TreeNode("Cameras", model.cameras.size()))
    {
        for (unsigned i = 0; i < model.cameras.size(); ++i)
        {
            const auto& camera = model.cameras[i];
            InspectorPushID(i);
            if (InspectorTreeNode(
                    "camera", ImGuiTreeNodeFlags_SpanAvailWidth, "%s", NameOrIndex(camera.name, i).c_str()))
            {
                Property("Type", camera.type.c_str());
                if (camera.type == "perspective")
                {
                    Property("Aspect ratio", camera.perspective.aspectRatio);
                    Property("Y FOV", camera.perspective.yfov);
                    Property("Near", camera.perspective.znear);
                    Property("Far", camera.perspective.zfar);
                }
                else
                {
                    Property("X magnification", camera.orthographic.xmag);
                    Property("Y magnification", camera.orthographic.ymag);
                    Property("Near", camera.orthographic.znear);
                    Property("Far", camera.orthographic.zfar);
                }
                DrawTinyExtensionsAndExtras(camera.extensions, camera.extras);
                InspectorTreePop();
            }
            InspectorPopID();
        }
        InspectorTreePop();
    }
    if (TreeNode("Lights", model.lights.size()))
    {
        for (unsigned i = 0; i < model.lights.size(); ++i)
        {
            const auto& light = model.lights[i];
            InspectorPushID(i);
            if (InspectorTreeNode(
                    "light", ImGuiTreeNodeFlags_SpanAvailWidth, "%s", NameOrIndex(light.name, i).c_str()))
            {
                Property("Type", light.type.c_str());
                Property("Color", JoinDoubles(light.color));
                Property("Intensity", light.intensity);
                Property("Range", light.range);
                Property("Inner cone", light.spot.innerConeAngle);
                Property("Outer cone", light.spot.outerConeAngle);
                DrawTinyExtensionsAndExtras(light.extensions, light.extras);
                InspectorTreePop();
            }
            InspectorPopID();
        }
        InspectorTreePop();
    }
}

void AssetViewer::DrawResources()
{
    if (!missingTextures_.empty())
    {
        ui::TextColored(ImVec4(1.0f, 0.75f, 0.2f, 1.0f), "Warning: %u texture%s missing (the materials show magenta):",
            missingTextures_.size(), missingTextures_.size() == 1 ? "" : "s");
        for (const ea::string& name : missingTextures_)
            ui::BulletText("%s", name.c_str());
        ui::Spacing();
    }

    constexpr ImGuiTableFlags flags =
        ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchProp;
    ui::PushStyleColor(ImGuiCol_TableRowBgAlt, ImVec4(1.0f, 1.0f, 1.0f, 0.02f));
    if (ui::BeginTable("##ResourceList", 4, flags))
    {
        const float fontSize = ui::GetFontSize();
        // The explicit weight avoids a one frame collapse of the column in a new table.
        ui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 1.0f);
        ui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, fontSize * 8.5f);
        ui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, fontSize * 5.0f);
        ui::TableSetupColumn("Refs", ImGuiTableColumnFlags_WidthFixed, fontSize * 2.5f);
        ui::TableHeadersRow();
        for (Resource* resource : assetResources_)
        {
            if (!resource)
                continue;
            ui::TableNextRow();
            ui::TableSetColumnIndex(0);
            const ea::string& name = resource->GetName();
            ui::TextWrapped("%s", name.empty() ? "(unnamed)" : GetFileNameAndExtension(name).c_str());
            if (!name.empty())
                ShowFullTextTooltip(name, Format("{}\n{}", name, resource->GetAbsoluteFileName()).c_str());
            ui::TableSetColumnIndex(1);
            ui::TextWrapped("%s", resource->GetTypeName().c_str());
            ui::TableSetColumnIndex(2);
            const ea::string size = FormatBytes(resource->GetMemoryUse());
            ui::TextWrapped("%s", size.substr(0, size.find(" (")).c_str());
            ui::TableSetColumnIndex(3);
            ui::Text("%d", resource->Refs());
        }
        ui::EndTable();
    }
    ui::PopStyleColor();

    ui::Spacing();
    ui::PushStyleColor(ImGuiCol_Text, DimColor);
    ui::TextUnformatted("Resource details");
    ui::PopStyleColor();
    for (unsigned i = 0; i < assetResources_.size(); ++i)
    {
        Resource* resource = assetResources_[i];
        if (!resource)
            continue;
        InspectorPushID(i);
        if (InspectorTreeNode("resource", ImGuiTreeNodeFlags_SpanAvailWidth, "%s: %s", resource->GetTypeName().c_str(),
                GetFileNameAndExtension(resource->GetName()).c_str()))
        {
            DrawResource(resource);
            InspectorTreePop();
        }
        InspectorPopID();
    }
    EndProperties();
}

void AssetViewer::DrawResource(Resource* resource)
{
    Property("Type", resource->GetTypeName());
    Property("Name", resource->GetName());
    Property("Name hash", resource->GetNameHash().Value());
    Property("Absolute file", resource->GetAbsoluteFileName());
    PropertyBytes("Memory use", resource->GetMemoryUse());
    Property("References", resource->Refs());

    if (auto* metadataResource = dynamic_cast<ResourceWithMetadata*>(resource))
    {
        if (TreeNode("Metadata", metadataResource->GetMetadata().size()))
        {
            for (const auto& var : metadataResource->GetMetadata())
            {
                const ea::string& key = var.second.GetString();
                Property(key.c_str(),
                    Format("{}: {}", metadataResource->GetMetadata(key).GetTypeName(),
                        metadataResource->GetMetadata(key).ToString()));
            }
            InspectorTreePop();
        }
    }
    if (auto* model = dynamic_cast<Model*>(resource))
        DrawModel(model);
    else if (auto* material = dynamic_cast<Material*>(resource))
        DrawMaterial(material);
    else if (auto* animation = dynamic_cast<Animation*>(resource))
        DrawAnimation(animation);
    else if (auto* effect = dynamic_cast<ParticleEffect*>(resource))
        DrawParticleEffect(effect);
    else if (auto* effect = dynamic_cast<ParticleGraphEffect*>(resource))
        DrawParticleGraphEffect(effect);
    else if (auto* effect = dynamic_cast<ParticleEffect2D*>(resource))
        DrawParticleEffect2D(effect);
    else if (auto* sprite = dynamic_cast<Sprite2D*>(resource))
    {
        Property("Texture", sprite->GetTexture() ? sprite->GetTexture()->GetName() : "(null)");
        Property("Rectangle", sprite->GetRectangle().ToString());
        Property("Hot spot", sprite->GetHotSpot().ToString());
        Property("Offset", sprite->GetOffset().ToString());
        Property("Texture edge offset", sprite->GetTextureEdgeOffset());
        Property("Sprite sheet", sprite->GetSpriteSheet() ? sprite->GetSpriteSheet()->GetName() : "(null)");
    }
    else if (auto* texture = dynamic_cast<Texture*>(resource))
        DrawTexture(texture);
    else if (auto* image = dynamic_cast<Image*>(resource))
    {
        Property("Dimensions", Format("{} x {} x {}", image->GetWidth(), image->GetHeight(), image->GetDepth()));
        Property("Components", image->GetComponents());
        Property("GPU format", static_cast<unsigned>(image->GetGPUFormat()));
        Property("Compressed", image->IsCompressed() ? "true" : "false");
        Property("Compressed format", static_cast<unsigned>(image->GetCompressedFormat()));
        Property("Compressed levels", image->GetNumCompressedLevels());
        Property("Cubemap", image->IsCubemap() ? "true" : "false");
        Property("Array", image->IsArray() ? "true" : "false");
        Property("Source sRGB", image->IsSRGB() ? "true" : "false");
        Property("Alpha channel", image->HasAlphaChannel() ? "true" : "false");
        if (image->IsCompressed() && TreeNode("Compressed mip levels", image->GetNumCompressedLevels()))
        {
            for (unsigned i = 0; i < image->GetNumCompressedLevels(); ++i)
            {
                const CompressedLevel level = image->GetCompressedLevel(i);
                Property(Format("Level {}", i).c_str(),
                    Format("{} x {} x {} | format={} | {}", level.width_, level.height_, level.depth_,
                        static_cast<unsigned>(level.format_), FormatBytes(level.dataSize_)));
            }
            InspectorTreePop();
        }
        unsigned siblingIndex = 0;
        for (const Image* sibling = image; sibling; sibling = sibling->GetNextSibling())
            ++siblingIndex;
        Property("Images / faces / layers", siblingIndex);
    }
}

void AssetViewer::DrawParticleEffect(ParticleEffect* effect)
{
    Property("Material", effect->GetMaterial() ? effect->GetMaterial()->GetName() : "(null)");
    Property("Maximum particles", effect->GetNumParticles());
    Property("Update invisible", effect->GetUpdateInvisible() ? "true" : "false");
    Property("Relative", effect->IsRelative() ? "true" : "false");
    Property("Scaled", effect->IsScaled() ? "true" : "false");
    Property("Sorted", effect->IsSorted() ? "true" : "false");
    Property("Fixed screen size", effect->IsFixedScreenSize() ? "true" : "false");
    Property("Animation LOD bias", effect->GetAnimationLodBias());
    Property("Emitter type", static_cast<unsigned>(effect->GetEmitterType()));
    Property("Emitter size", effect->GetEmitterSize().ToString());
    Property("Direction minimum", effect->GetMinDirection().ToString());
    Property("Direction maximum", effect->GetMaxDirection().ToString());
    Property("Constant force", effect->GetConstantForce().ToString());
    Property("Damping force", effect->GetDampingForce());
    Property("Active time", effect->GetActiveTime());
    Property("Inactive time", effect->GetInactiveTime());
    Property("Emission rate minimum", effect->GetMinEmissionRate());
    Property("Emission rate maximum", effect->GetMaxEmissionRate());
    Property("Particle size minimum", effect->GetMinParticleSize().ToString());
    Property("Particle size maximum", effect->GetMaxParticleSize().ToString());
    Property("TTL minimum", effect->GetMinTimeToLive());
    Property("TTL maximum", effect->GetMaxTimeToLive());
    Property("Velocity minimum", effect->GetMinVelocity());
    Property("Velocity maximum", effect->GetMaxVelocity());
    Property("Rotation minimum", effect->GetMinRotation());
    Property("Rotation maximum", effect->GetMaxRotation());
    Property("Rotation speed minimum", effect->GetMinRotationSpeed());
    Property("Rotation speed maximum", effect->GetMaxRotationSpeed());
    Property("Size additive", effect->GetSizeAdd());
    Property("Size multiplier", effect->GetSizeMul());
    Property("Face camera mode", static_cast<unsigned>(effect->GetFaceCameraMode()));
    if (TreeNode("Color frames", effect->GetColorFrames().size()))
    {
        for (unsigned i = 0; i < effect->GetColorFrames().size(); ++i)
        {
            const ColorFrame& frame = effect->GetColorFrames()[i];
            Property(Format("[{}]", i).c_str(), Format("time={} color={}", frame.time_, frame.color_.ToString()));
        }
        InspectorTreePop();
    }
    if (TreeNode("Texture frames", effect->GetTextureFrames().size()))
    {
        for (unsigned i = 0; i < effect->GetTextureFrames().size(); ++i)
        {
            const TextureFrame& frame = effect->GetTextureFrames()[i];
            Property(Format("[{}]", i).c_str(), Format("time={} uv={}", frame.time_, frame.uv_.ToString()));
        }
        InspectorTreePop();
    }
}

void AssetViewer::DrawParticleEffect2D(ParticleEffect2D* effect)
{
    Property("Sprite", effect->GetSprite() ? effect->GetSprite()->GetName() : "(null)");
    Property("Source position variance", effect->GetSourcePositionVariance().ToString());
    Property("Speed", effect->GetSpeed());
    Property("Speed variance", effect->GetSpeedVariance());
    Property("Particle lifespan", effect->GetParticleLifeSpan());
    Property("Lifespan variance", effect->GetParticleLifespanVariance());
    Property("Angle", effect->GetAngle());
    Property("Angle variance", effect->GetAngleVariance());
    Property("Gravity", effect->GetGravity().ToString());
    Property("Radial acceleration", effect->GetRadialAcceleration());
    Property("Radial accel variance", effect->GetRadialAccelVariance());
    Property("Tangential acceleration", effect->GetTangentialAcceleration());
    Property("Tangential accel variance", effect->GetTangentialAccelVariance());
    Property("Start color", effect->GetStartColor().ToString());
    Property("Start color variance", effect->GetStartColorVariance().ToString());
    Property("Finish color", effect->GetFinishColor().ToString());
    Property("Finish color variance", effect->GetFinishColorVariance().ToString());
    Property("Maximum particles", effect->GetMaxParticles());
    Property("Start size", effect->GetStartParticleSize());
    Property("Start size variance", effect->GetStartParticleSizeVariance());
    Property("Finish size", effect->GetFinishParticleSize());
    Property("Finish size variance", effect->GetFinishParticleSizeVariance());
    Property("Duration", effect->GetDuration());
    Property("Emitter type", static_cast<unsigned>(effect->GetEmitterType()));
    Property("Maximum radius", effect->GetMaxRadius());
    Property("Maximum radius variance", effect->GetMaxRadiusVariance());
    Property("Minimum radius", effect->GetMinRadius());
    Property("Minimum radius variance", effect->GetMinRadiusVariance());
    Property("Rotate per second", effect->GetRotatePerSecond());
    Property("Rotate per second variance", effect->GetRotatePerSecondVariance());
    Property("Blend mode", static_cast<unsigned>(effect->GetBlendMode()));
    Property("Rotation start", effect->GetRotationStart());
    Property("Rotation start variance", effect->GetRotationStartVariance());
    Property("Rotation end", effect->GetRotationEnd());
    Property("Rotation end variance", effect->GetRotationEndVariance());
}

void AssetViewer::DrawParticleGraphSpan(const char* label, const ParticleGraphSpan& span)
{
    Property(label, Format("offset={} size={}", span.offset_, FormatBytes(span.size_)));
}

void AssetViewer::DrawParticleGraph(const char* label, ParticleGraph& graph)
{
    if (!TreeNode(label, graph.GetNumNodes()))
        return;
    for (unsigned nodeIndex = 0; nodeIndex < graph.GetNumNodes(); ++nodeIndex)
    {
        ParticleGraphNode* node = graph.GetNode(nodeIndex);
        InspectorPushID(nodeIndex);
        if (InspectorTreeNode(
                "node", ImGuiTreeNodeFlags_SpanAvailWidth, "%s [%u]", node->GetTypeName().c_str(), nodeIndex))
        {
            Property("Type hash", node->GetType().Value());
            Property("Instance size", FormatBytes(node->EvaluateInstanceSize()));
            DrawSerializable(node);
            if (TreeNode("Pins", node->GetNumPins()))
            {
                for (unsigned pinIndex = 0; pinIndex < node->GetNumPins(); ++pinIndex)
                {
                    const ParticleGraphPin& pin = node->GetPin(pinIndex);
                    InspectorPushID(pinIndex);
                    if (InspectorTreeNode(
                            "pin", ImGuiTreeNodeFlags_SpanAvailWidth, "%s [%u]", pin.GetName().c_str(), pinIndex))
                    {
                        Property("Direction", pin.IsInput() ? "Input" : "Output");
                        Property("Flags", pin.GetFlags().AsInteger());
                        Property("Name hash", pin.GetNameHash().Value());
                        Property("Requested type", Variant::GetTypeName(pin.GetRequestedType()));
                        Property("Runtime type", Variant::GetTypeName(pin.GetValueType()));
                        Property("Container type", static_cast<unsigned>(pin.GetContainerType()));
                        Property("Attribute index", pin.GetAttributeIndex());
                        const ParticleGraphPinRef memory = pin.GetMemoryReference();
                        Property("Memory reference",
                            Format("container={} index={}", static_cast<unsigned>(memory.type_), memory.index_));
                        Property("Connected", pin.GetConnected() ? "true" : "false");
                        if (pin.GetConnected())
                            Property("Source",
                                Format("node={} pin={}", pin.GetConnectedNodeIndex(), pin.GetConnectedPinIndex()));
                        InspectorTreePop();
                    }
                    InspectorPopID();
                }
                InspectorTreePop();
            }
            InspectorTreePop();
        }
        InspectorPopID();
    }
    InspectorTreePop();
}

void AssetViewer::DrawParticleGraphEffect(ParticleGraphEffect* effect)
{
    Property("Layers", effect->GetNumLayers());
    for (unsigned layerIndex = 0; layerIndex < effect->GetNumLayers(); ++layerIndex)
    {
        ParticleGraphLayer* layer = effect->GetLayer(layerIndex);
        InspectorPushID(layerIndex);
        if (InspectorTreeNode("layer", ImGuiTreeNodeFlags_SpanAvailWidth, "Layer %u", layerIndex))
        {
            Property("Capacity", layer->GetCapacity());
            Property("Time scale", layer->GetTimeScale());
            Property("Loop", layer->IsLoop() ? "true" : "false");
            Property("Duration", layer->GetDuration());
            Property("Temporary buffer", FormatBytes(layer->GetTempBufferSize()));
            DrawSerializable(layer);

            const ParticleGraphAttributeLayout& attributes = layer->GetAttributeLayout();
            if (TreeNode("Particle attributes", attributes.GetNumAttributes()))
            {
                Property("Required memory", FormatBytes(attributes.GetRequiredMemory()));
                for (unsigned i = 0; i < attributes.GetNumAttributes(); ++i)
                {
                    const ParticleGraphSpan span = attributes.GetSpan(i);
                    Property(Format("{} [{}]", attributes.GetName(i), i).c_str(),
                        Format("type={} offset={} size={}", Variant::GetTypeName(attributes.GetType(i)),
                            span.offset_, FormatBytes(span.size_)));
                }
                InspectorTreePop();
            }

            const ParticleGraphLayer::AttributeBufferLayout& layout = layer->GetAttributeBufferLayout();
            if (InspectorTreeNode("Runtime memory layout", ImGuiTreeNodeFlags_SpanAvailWidth))
            {
                Property("Attribute buffer", FormatBytes(layout.attributeBufferSize_));
                DrawParticleGraphSpan("Emit pointers", layout.emitNodePointers_);
                DrawParticleGraphSpan("Init pointers", layout.initNodePointers_);
                DrawParticleGraphSpan("Update pointers", layout.updateNodePointers_);
                DrawParticleGraphSpan("Node instances", layout.nodeInstances_);
                DrawParticleGraphSpan("Indices", layout.indices_);
                DrawParticleGraphSpan("Scalar indices", layout.scalarIndices_);
                DrawParticleGraphSpan("Natural indices", layout.naturalIndices_);
                DrawParticleGraphSpan("Destruction queue", layout.destructionQueue_);
                DrawParticleGraphSpan("Values", layout.values_);
                InspectorTreePop();
            }
            DrawParticleGraph("Emit graph", layer->GetEmitGraph());
            DrawParticleGraph("Initialization graph", layer->GetInitGraph());
            DrawParticleGraph("Update graph", layer->GetUpdateGraph());
            InspectorTreePop();
        }
        InspectorPopID();
    }
}

void AssetViewer::DrawModel(Model* model)
{
    const BoundingBox& box = model->GetBoundingBox();
    Property("Bounds min", box.min_.ToString());
    Property("Bounds max", box.max_.ToString());
    Property("Bounds size", box.Size().ToString());
    Property("Geometry centers", model->GetGeometryCenters().size());
    if (TreeNode("Vertex buffers", model->GetVertexBuffers().size()))
    {
        for (unsigned i = 0; i < model->GetVertexBuffers().size(); ++i)
        {
            VertexBuffer* buffer = model->GetVertexBuffers()[i];
            InspectorPushID(i);
            if (InspectorTreeNode("vb", ImGuiTreeNodeFlags_SpanAvailWidth, "Vertex buffer %u", i))
            {
                Property("Vertices", buffer->GetVertexCount());
                Property("Vertex size", FormatBytes(buffer->GetVertexSize()));
                Property("Total size",
                    FormatBytes(
                        static_cast<unsigned long long>(buffer->GetVertexCount()) * buffer->GetVertexSize()));
                Property("Dynamic", buffer->IsDynamic() ? "true" : "false");
                Property("Morph range start", model->GetMorphRangeStart(i));
                Property("Morph range count", model->GetMorphRangeCount(i));
                if (TreeNode("Elements", buffer->GetElements().size()))
                {
                    for (unsigned elementIndex = 0; elementIndex < buffer->GetElements().size(); ++elementIndex)
                    {
                        const VertexElement& element = buffer->GetElements()[elementIndex];
                        Property(Format("Element {}", elementIndex).c_str(),
                            Format("type={} semantic={} index={} offset={} step={} size={}",
                                static_cast<unsigned>(element.type_), static_cast<unsigned>(element.semantic_),
                                element.index_, element.offset_, element.stepRate_,
                                ELEMENT_TYPESIZES[element.type_]));
                    }
                    InspectorTreePop();
                }
                InspectorTreePop();
            }
            InspectorPopID();
        }
        InspectorTreePop();
    }
    if (TreeNode("Index buffers", model->GetIndexBuffers().size()))
    {
        for (unsigned i = 0; i < model->GetIndexBuffers().size(); ++i)
        {
            IndexBuffer* buffer = model->GetIndexBuffers()[i];
            Property(Format("Index buffer {}", i).c_str(),
                Format("{} indices, {}-byte, {}, dynamic={}", buffer->GetIndexCount(), buffer->GetIndexSize(),
                    FormatBytes(static_cast<unsigned long long>(buffer->GetIndexCount()) * buffer->GetIndexSize()),
                    buffer->IsDynamic()));
        }
        InspectorTreePop();
    }
    if (TreeNode("Geometries", model->GetGeometries().size()))
    {
        for (unsigned geometryIndex = 0; geometryIndex < model->GetGeometries().size(); ++geometryIndex)
        {
            InspectorPushID(geometryIndex);
            if (InspectorTreeNode("geometry", ImGuiTreeNodeFlags_SpanAvailWidth, "Geometry %u", geometryIndex))
            {
                Property("Center", model->GetGeometryCenter(geometryIndex).ToString());
                if (geometryIndex < model->GetGeometryBoneMappings().size())
                    Property("Bone mapping", JoinUnsigned(model->GetGeometryBoneMappings()[geometryIndex]));
                for (unsigned lod = 0; lod < model->GetGeometries()[geometryIndex].size(); ++lod)
                {
                    Geometry* geometry = model->GetGeometries()[geometryIndex][lod];
                    InspectorPushID(lod);
                    if (InspectorTreeNode("lod", ImGuiTreeNodeFlags_SpanAvailWidth, "LOD %u", lod))
                    {
                        Property("LOD distance", geometry->GetLodDistance());
                        Property("Primitive type", static_cast<unsigned>(geometry->GetPrimitiveType()));
                        Property("Primitive count", geometry->GetPrimitiveCount());
                        Property("Index start", geometry->GetIndexStart());
                        Property("Index count", geometry->GetIndexCount());
                        Property("Vertex start", geometry->GetVertexStart());
                        Property("Vertex count", geometry->GetVertexCount());
                        Property("Vertex buffers", geometry->GetNumVertexBuffers());
                        Property("Indexed", geometry->GetIndexBuffer() ? "true" : "false");
                        InspectorTreePop();
                    }
                    InspectorPopID();
                }
                InspectorTreePop();
            }
            InspectorPopID();
        }
        InspectorTreePop();
    }
    DrawSkeleton(model->GetSkeleton());
    if (TreeNode("Morphs", model->GetMorphs().size()))
    {
        for (unsigned i = 0; i < model->GetMorphs().size(); ++i)
        {
            const ModelMorph& morph = model->GetMorphs()[i];
            InspectorPushID(i);
            if (InspectorTreeNode("morph", ImGuiTreeNodeFlags_SpanAvailWidth, "%s [%u]", morph.name_.c_str(), i))
            {
                Property("Name hash", morph.nameHash_.Value());
                Property("Default weight", morph.weight_);
                if (TreeNode("Buffers", morph.buffers_.size()))
                {
                    for (const auto& [bufferIndex, bufferMorph] : morph.buffers_)
                    {
                        Property(Format("Buffer {}", bufferIndex).c_str(),
                            Format("mask={} vertices={} data={}", bufferMorph.elementMask_.AsInteger(),
                                bufferMorph.vertexCount_, FormatBytes(bufferMorph.dataSize_)));
                    }
                    InspectorTreePop();
                }
                InspectorTreePop();
            }
            InspectorPopID();
        }
        InspectorTreePop();
    }
}

void AssetViewer::DrawSkeleton(const Skeleton& skeleton)
{
    if (!TreeNode("Skeleton", skeleton.GetBones().size()))
        return;
    Property("Bone order", JoinUnsigned(skeleton.GetBonesOrder()));
    for (unsigned i = 0; i < skeleton.GetBones().size(); ++i)
    {
        const Bone& bone = skeleton.GetBones()[i];
        InspectorPushID(i);
        if (InspectorTreeNode("bone", ImGuiTreeNodeFlags_SpanAvailWidth, "%s [%u]", bone.name_.c_str(), i))
        {
            Property("Name hash", bone.nameHash_.Value());
            Property("Parent", bone.parentIndex_);
            Property("Position", bone.initialPosition_.ToString());
            Property("Rotation", bone.initialRotation_.ToString());
            Property("Scale", bone.initialScale_.ToString());
            Property("Offset matrix", bone.offsetMatrix_.ToString());
            Property("Animated", bone.animated_ ? "true" : "false");
            Property("Collision mask", bone.collisionMask_.AsInteger());
            Property("Radius", bone.radius_);
            Property("Bounds min", bone.boundingBox_.min_.ToString());
            Property("Bounds max", bone.boundingBox_.max_.ToString());
            InspectorTreePop();
        }
        InspectorPopID();
    }
    InspectorTreePop();
}

void AssetViewer::DrawMaterial(Material* material)
{
    Property("Techniques", material->GetNumTechniques());
    Property("Cull mode", cullModeNames[material->GetCullMode()]);
    Property("Shadow cull", cullModeNames[material->GetShadowCullMode()]);
    Property("Fill mode", fillModeNames[material->GetFillMode()]);
    Property("Render order", material->GetRenderOrder());
    if (TreeNode("Technique entries", material->GetTechniques().size()))
    {
        for (unsigned i = 0; i < material->GetTechniques().size(); ++i)
        {
            const TechniqueEntry& entry = material->GetTechniques()[i];
            Property(Format("Technique {}", i).c_str(),
                Format("{} | quality={} | LOD distance={}",
                    entry.technique_ ? entry.technique_->GetName() : EMPTY_STRING,
                    static_cast<unsigned>(entry.qualityLevel_), entry.lodDistance_));
        }
        InspectorTreePop();
    }
    if (TreeNode("Textures", material->GetTextures().size()))
    {
        for (const auto& [unit, texture] : material->GetTextures())
            Property(texture.name_.c_str(),
                texture.value_ ? Format("{} ({})", texture.value_->GetName(), unit.Value()) : "(null)");
        InspectorTreePop();
    }
    if (TreeNode("Shader parameters", material->GetShaderParameters().size()))
    {
        for (const auto& [hash, parameter] : material->GetShaderParameters())
            Property(parameter.name_.c_str(),
                Format("{}: {} | hash={} | custom={}", parameter.value_.GetTypeName(), parameter.value_.ToString(),
                    hash.Value(), parameter.isCustom_));
        InspectorTreePop();
    }
}

void AssetViewer::DrawAnimation(Animation* animation)
{
    Property("Animation name", animation->GetAnimationName());
    Property("Name hash", animation->GetAnimationNameHash().Value());
    Property("Length", animation->GetLength());
    if (TreeNode("Bone/node tracks", animation->GetTracks().size()))
    {
        unsigned trackIndex = 0;
        for (const auto& [_, track] : animation->GetTracks())
        {
            InspectorPushID(trackIndex++);
            if (InspectorTreeNode("track", ImGuiTreeNodeFlags_SpanAvailWidth, "%s (%zu keyframes)",
                    track.name_.c_str(), track.keyFrames_.size()))
            {
                Property("Name hash", track.nameHash_.Value());
                Property("Channels", track.channelMask_.AsInteger());
                Property("Position weight", track.positionWeight_);
                Property("Rotation weight", track.rotationWeight_);
                Property("Scale weight", track.scaleWeight_);
                if (TreeNode("Keyframes", track.keyFrames_.size()))
                {
                    for (unsigned i = 0; i < track.keyFrames_.size(); ++i)
                    {
                        const AnimationKeyFrame& frame = track.keyFrames_[i];
                        Property(Format("[{}] t={}", i, frame.time_).c_str(),
                            Format("P={} R={} S={}", frame.position_.ToString(), frame.rotation_.ToString(),
                                frame.scale_.ToString()));
                    }
                    InspectorTreePop();
                }
                InspectorTreePop();
            }
            InspectorPopID();
        }
        InspectorTreePop();
    }
    if (TreeNode("Variant tracks", animation->GetVariantTracks().size()))
    {
        unsigned trackIndex = 0;
        for (const auto& [_, track] : animation->GetVariantTracks())
        {
            InspectorPushID(trackIndex++);
            if (InspectorTreeNode("track", ImGuiTreeNodeFlags_SpanAvailWidth, "%s (%zu keyframes)",
                    track.name_.c_str(), track.keyFrames_.size()))
            {
                Property("Name hash", track.nameHash_.Value());
                Property("Type", Variant::GetTypeName(track.type_));
                Property("Interpolation", static_cast<unsigned>(track.interpolation_));
                Property("Spline tension", track.splineTension_);
                Property("Weight", track.weight_);
                for (unsigned i = 0; i < track.keyFrames_.size(); ++i)
                    Property(Format("[{}] t={}", i, track.keyFrames_[i].time_).c_str(),
                        track.keyFrames_[i].value_.ToString());
                InspectorTreePop();
            }
            InspectorPopID();
        }
        InspectorTreePop();
    }
    if (TreeNode("Triggers", animation->GetTriggers().size()))
    {
        for (unsigned i = 0; i < animation->GetTriggers().size(); ++i)
        {
            const AnimationTriggerPoint& trigger = animation->GetTriggers()[i];
            Property(Format("Trigger {}", i).c_str(),
                Format("t={} | {}: {}", trigger.time_, trigger.data_.GetTypeName(), trigger.data_.ToString()));
        }
        InspectorTreePop();
    }
}

void AssetViewer::DrawTexture(Texture* texture)
{
    Property("Dimensions", Format("{} x {} x {}", texture->GetWidth(), texture->GetHeight(), texture->GetDepth()));
    Property("Levels", texture->GetLevels());
    Property("Format", static_cast<unsigned>(texture->GetFormat()));
    Property("Components", texture->GetComponents());
    Property("Compressed", texture->IsCompressed() ? "true" : "false");
    Property("Linear", texture->GetLinear() ? "true" : "false");
    Property("sRGB", texture->GetSRGB() ? "true" : "false");
    Property("Filter mode", static_cast<unsigned>(texture->GetFilterMode()));
    Property("Address U", static_cast<unsigned>(texture->GetAddressMode(TextureCoordinate::U)));
    Property("Address V", static_cast<unsigned>(texture->GetAddressMode(TextureCoordinate::V)));
    Property("Address W", static_cast<unsigned>(texture->GetAddressMode(TextureCoordinate::W)));
    Property("Anisotropy", texture->GetAnisotropy());
    Property("Minimum LOD", texture->GetMinLod());
    Property("Maximum LOD", texture->GetMaxLod());
    Property("Shadow compare", texture->GetShadowCompare() ? "true" : "false");
    Property("Multisample", texture->GetMultiSample());
    Property("Autoresolve", texture->GetAutoResolve() ? "true" : "false");
    Property("Render target", texture->IsRenderTarget() ? "true" : "false");
    Property("Depth stencil", texture->IsDepthStencil() ? "true" : "false");
    Property("Unordered access", texture->IsUnorderedAccess() ? "true" : "false");
    Property("Backup texture", texture->GetBackupTexture() ? texture->GetBackupTexture()->GetName() : "(none)");
    if (TreeNode("Mip levels", texture->GetLevels()))
    {
        for (unsigned i = 0; i < texture->GetLevels(); ++i)
        {
            const int width = texture->GetLevelWidth(i);
            const int height = texture->GetLevelHeight(i);
            const int depth = texture->GetLevelDepth(i);
            Property(Format("Level {}", i).c_str(),
                Format("{} x {} x {} | row={} | {}", width, height, depth,
                    FormatBytes(texture->GetRowDataSize(width)),
                    FormatBytes(texture->GetDataSize(width, height, depth))));
        }
        InspectorTreePop();
    }
    if (RenderSurface* surface = texture->GetRenderSurface())
    {
        Property("Render surface size", surface->GetSize().ToString());
        Property("Render surface update mode", static_cast<unsigned>(surface->GetUpdateMode()));
    }
}

void AssetViewer::DrawNode(Node* node)
{
    if (!node || node == cameraNode_ || environment_.IsEnvironmentNode(node))
        return;
    InspectorPushID(node);
    const ea::string label =
        Format("{} [{}]", node->GetName().empty() ? "(unnamed)" : node->GetName(), node->GetID());
    if (InspectorTreeNode(
            "node", ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen, "%s", label.c_str()))
    {
        Property("Name", node->GetName().empty() ? ea::string("(unnamed)") : node->GetName());
        Property("ID", node->GetID());
        Property("Enabled", node->IsEnabled() ? "true" : "false");
        Property("Temporary", node->IsTemporary() ? "true" : "false");
        Property("Tags", JoinStrings(node->GetTags()));
        Property("Position", node->GetPosition());
        Property("Rotation (euler)", node->GetRotation().EulerAngles());
        Property("Scale", node->GetScale());
        Property("World position", node->GetWorldPosition());
        Property("World rotation (euler)", node->GetWorldRotation().EulerAngles());
        Property("World scale", node->GetWorldScale());
        if (!node->GetComponents().empty())
        {
            ea::string types;
            for (const SharedPtr<Component>& component : node->GetComponents())
                types += (types.empty() ? "" : ", ") + component->GetTypeName();
            Property("Components", types);
        }
        if (TreeNode("Variables", node->GetVars().size()))
        {
            for (const auto& [key, value] : node->GetVars())
                Property(key.c_str(), Format("{}: {}", value.GetTypeName(), value.ToString()));
            InspectorTreePop();
        }
        if (TreeNode("Node attributes", node->GetNumAttributes()))
        {
            DrawSerializable(node);
            InspectorTreePop();
        }
        if (TreeNode("Components", node->GetComponents().size()))
        {
            for (const SharedPtr<Component>& component : node->GetComponents())
            {
                InspectorPushID(component.Get());
                if (InspectorTreeNode("component", ImGuiTreeNodeFlags_SpanAvailWidth, "%s [%u]",
                        component->GetTypeName().c_str(), component->GetID()))
                {
                    Property("Enabled", component->IsEnabled() ? "true" : "false");
                    Property("Temporary", component->IsTemporary() ? "true" : "false");
                    Property("Type hash", component->GetType().Value());
                    DrawSerializable(component);
                    InspectorTreePop();
                }
                InspectorPopID();
            }
            InspectorTreePop();
        }
        if (TreeNode("Children", node->GetChildren().size()))
        {
            for (const SharedPtr<Node>& child : node->GetChildren())
                DrawNode(child);
            InspectorTreePop();
        }
        InspectorTreePop();
    }
    InspectorPopID();
}

void AssetViewer::DrawSerializable(Serializable* serializable)
{
    const ea::vector<AttributeInfo>* attributes = serializable->GetAttributes();
    if (!attributes)
        return;
    for (unsigned i = 0; i < attributes->size(); ++i)
    {
        const AttributeInfo& attribute = (*attributes)[i];
        const Variant value = serializable->GetAttribute(i);
        ea::string renderedValue = value.ToString();
        if (!attribute.enumNames_.empty() && value.GetUInt() < attribute.enumNames_.size())
            renderedValue = Format("{} ({})", attribute.enumNames_[value.GetUInt()], value.GetUInt());
        const ea::string details = Format("{}: {}\nType: {}\nDefault: {}\nMode: {}", attribute.name_, renderedValue,
            Variant::GetTypeName(attribute.type_), attribute.defaultValue_.ToString(), attribute.mode_.AsInteger());
        PropertyRow(attribute.name_.c_str(), renderedValue, nullptr, details.c_str());
    }
}

} // namespace Urho3D
