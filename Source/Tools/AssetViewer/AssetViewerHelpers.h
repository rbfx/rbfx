// Copyright (c) 2026 the rbfx project.
// This work is licensed under the terms of the MIT license.

#pragma once

#include <Urho3D/Core/CommandLine.h>
#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Core/ProcessUtils.h>
#include <Urho3D/Core/StringUtils.h>
#include <Urho3D/Core/WorkQueue.h>
#include <Urho3D/Engine/Application.h>
#include <Urho3D/Engine/EngineDefs.h>
#include <Urho3D/Graphics/AnimatedModel.h>
#include <Urho3D/Graphics/Animation.h>
#include <Urho3D/Graphics/AnimationController.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/DebugRenderer.h>
#include <Urho3D/Graphics/Drawable.h>
#include <Urho3D/Graphics/Geometry.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/GraphicsEvents.h>
#include <Urho3D/Graphics/IndexBuffer.h>
#include <Urho3D/Graphics/Light.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/ParticleEffect.h>
#include <Urho3D/Graphics/ParticleEmitter.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/Skybox.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Graphics/Technique.h>
#include <Urho3D/Graphics/Texture.h>
#include <Urho3D/Graphics/Texture2D.h>
#include <Urho3D/Graphics/Texture2DArray.h>
#include <Urho3D/Graphics/Texture3D.h>
#include <Urho3D/Graphics/TextureCube.h>
#include <Urho3D/Graphics/VertexBuffer.h>
#include <Urho3D/Graphics/Viewport.h>
#include <Urho3D/Graphics/Zone.h>
#include <Urho3D/Input/FreeFlyController.h>
#include <Urho3D/IO/File.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/IO/VirtualFileSystem.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/Input/InputEvents.h>
#include <Urho3D/Particles/ParticleGraph.h>
#include <Urho3D/Particles/ParticleGraphEffect.h>
#include <Urho3D/Particles/ParticleGraphEmitter.h>
#include <Urho3D/Particles/ParticleGraphLayer.h>
#include <Urho3D/Particles/ParticleGraphNode.h>
#include <Urho3D/Particles/ParticleGraphPin.h>
#include <Urho3D/RenderPipeline/ShaderConsts.h>
#include <Urho3D/Resource/Image.h>
#include <Urho3D/Resource/JSONFile.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/XMLFile.h>
#include <Urho3D/Scene/Component.h>
#include <Urho3D/Scene/Node.h>
#include <Urho3D/Scene/PrefabResource.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneResource.h>
#include <Urho3D/Scene/Serializable.h>
#include <Urho3D/SystemUI/SystemUI.h>
#include <Urho3D/SystemUI/Widgets.h>
#include <Urho3D/Urho2D/ParticleEffect2D.h>
#include <Urho3D/Urho2D/ParticleEmitter2D.h>
#include <Urho3D/Urho2D/Sprite2D.h>
#include <Urho3D/Urho2D/SpriteSheet2D.h>
#include <Urho3D/Utility/AnimationMetadata.h>
#include <Urho3D/Utility/FBXImporter.h>
#include <Urho3D/Utility/GLTFImporter.h>

#include "DroppedPath.h"

#include <EASTL/algorithm.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/unordered_set.h>

#include <cmath>
#include <tiny_gltf.h>

namespace Urho3D
{

namespace AssetViewerDetail
{

inline bool IsFiniteBoundingBox(const BoundingBox& box)
{
    const auto isFinite = [](const Vector3& value)
    { return std::isfinite(value.x_) && std::isfinite(value.y_) && std::isfinite(value.z_); };
    return box.Defined() && isFinite(box.min_) && isFinite(box.max_) && box.min_.x_ <= box.max_.x_
        && box.min_.y_ <= box.max_.y_ && box.min_.z_ <= box.max_.z_;
}

inline bool IsImageExtension(const ea::string& extension)
{
    return extension == ".dds" || extension == ".ktx" || extension == ".pvr" || extension == ".bmp"
        || extension == ".jpg" || extension == ".jpeg" || extension == ".png" || extension == ".tga"
        || extension == ".webp" || extension == ".hdr" || extension == ".psd" || extension == ".gif"
        || extension == ".pic" || extension == ".pnm";
}

inline bool IsSupportedAssetExtension(const ea::string& extension)
{
    return extension == ".gltf" || extension == ".glb" || extension == ".fbx" || extension == ".blend"
        || extension == ".mdl" || extension == ".prefab" || extension == ".scene" || extension == ".material"
        || extension == ".ani" || extension == ".pex" || extension == ".xml" || IsImageExtension(extension);
}

inline const char* GetSupportedExtensionsText()
{
    return ".fbx, .gltf, .glb, .blend, .mdl, .prefab, .scene, .xml, .material, .ani, .pex, .dds, .ktx, .pvr, .bmp, "
           ".jpg, .jpeg, .png, .tga, .webp, .hdr, .psd, .gif, .pic, .pnm";
}

inline ea::string FormatBytes(unsigned long long value)
{
    static const char* suffixes[]{"B", "KiB", "MiB", "GiB", "TiB"};
    double amount = static_cast<double>(value);
    unsigned suffix = 0;
    while (amount >= 1024.0 && suffix + 1 < ea::size(suffixes))
    {
        amount /= 1024.0;
        ++suffix;
    }
    return suffix == 0 ? Format("{} {}", value, suffixes[suffix])
                       : Format("{:.2f} {} ({} bytes)", amount, suffixes[suffix], value);
}

inline ea::string NameOrIndex(const std::string& name, unsigned index)
{
    return !name.empty() ? Format("{} [{}]", name.c_str(), index) : Format("[{}]", index);
}

inline ea::string JoinInts(const std::vector<int>& values)
{
    ea::string result;
    for (unsigned i = 0; i < values.size(); ++i)
    {
        if (i != 0)
            result += ", ";
        result += Format("{}", values[i]);
    }
    return result.empty() ? "(none)" : result;
}

inline ea::string JoinUnsigned(const ea::vector<unsigned>& values)
{
    ea::string result;
    for (unsigned i = 0; i < values.size(); ++i)
    {
        if (i != 0)
            result += ", ";
        result += Format("{}", values[i]);
    }
    return result.empty() ? "(none)" : result;
}

inline ea::string JoinDoubles(const std::vector<double>& values)
{
    ea::string result;
    for (unsigned i = 0; i < values.size(); ++i)
    {
        if (i != 0)
            result += ", ";
        result += Format("{:.9g}", values[i]);
    }
    return result.empty() ? "(none)" : result;
}

inline ea::string JoinStrings(const StringVector& values)
{
    ea::string result;
    for (const ea::string& value : values)
        result += (result.empty() ? "" : ", ") + value;
    return result.empty() ? "(none)" : result;
}

// Text colors of the viewer UI.
inline const ImVec4 SuccessColor(0.40f, 0.85f, 0.50f, 1.0f);
inline const ImVec4 DimColor(0.60f, 0.60f, 0.62f, 1.0f);

/// State of the label / value table that Property opens when needed.
struct PropertyTableState
{
    bool open_{};
    unsigned counter_{};
};

inline PropertyTableState& GetPropertyTableState()
{
    static PropertyTableState state;
    return state;
}

/// Return the width of the label column. The width depends on the font size.
inline float GetPropertyLabelWidth() { return ui::GetFontSize() * 11.0f; }

/// Begin a table with a dimmed label column and a stretching value column.
inline bool BeginPropertyTable(const char* id)
{
    constexpr ImGuiTableFlags flags =
        ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchProp;
    // A subtle alternate row color keeps the dimmed labels readable.
    ui::PushStyleColor(ImGuiCol_TableRowBgAlt, ImVec4(1.0f, 1.0f, 1.0f, 0.02f));
    if (!ui::BeginTable(id, 2, flags))
    {
        ui::PopStyleColor();
        return false;
    }
    ui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, GetPropertyLabelWidth());
    // The explicit weight is important. Without it, SizingStretchProp uses the content width of the previous frame.
    // A new table has no such width, so the value column is 4 pixels wide for one frame.
    ui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch, 1.0f);
    return true;
}

inline void EndPropertyTable()
{
    ui::EndTable();
    ui::PopStyleColor();
}

/// Close the table that Property opened. Call it before any item that is not a property row.
inline void EndProperties()
{
    PropertyTableState& state = GetPropertyTableState();
    if (state.open_)
        EndPropertyTable();
    state.open_ = false;
}

/// Reset the table counter at the start of a frame.
inline void BeginPropertyFrame()
{
    EndProperties();
    GetPropertyTableState().counter_ = 0;
}

/// Show the full text in a tooltip and copy it on a double click.
inline void ShowFullTextTooltip(const ea::string& text, const char* tooltip)
{
    if (!ui::IsItemHovered())
        return;
    if (ui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        ui::SetClipboardText(text.c_str());
    ui::BeginTooltip();
    ui::PushTextWrapPos(ui::GetFontSize() * 28.0f);
    ui::TextUnformatted(tooltip && tooltip[0] ? tooltip : text.c_str());
    ui::PushStyleColor(ImGuiCol_Text, DimColor);
    ui::TextUnformatted("Double-click to copy the value.");
    ui::PopStyleColor();
    ui::PopTextWrapPos();
    ui::EndTooltip();
}

/// Draw a row of the current property table.
inline void PropertyRow(
    const char* label, const ea::string& value, const ImVec4* valueColor = nullptr, const char* tooltip = nullptr)
{
    PropertyTableState& state = GetPropertyTableState();
    if (!state.open_)
    {
        const ea::string id = Format("##Properties{}", state.counter_++);
        if (!BeginPropertyTable(id.c_str()))
            return;
        state.open_ = true;
    }

    ui::TableNextRow();
    ui::TableSetColumnIndex(0);
    ui::PushStyleColor(ImGuiCol_Text, DimColor);
    ui::TextWrapped("%s", label);
    ui::PopStyleColor();
    ui::TableSetColumnIndex(1);
    if (valueColor)
        ui::PushStyleColor(ImGuiCol_Text, *valueColor);
    ui::TextWrapped("%s", value.c_str());
    if (valueColor)
        ui::PopStyleColor();
    if (value.length() > 40 || (tooltip && tooltip[0]))
        ShowFullTextTooltip(value, tooltip);
}

inline void Property(const char* label, const ea::string& value) { PropertyRow(label, value); }

inline void Property(const char* label, const char* value) { PropertyRow(label, ea::string(value ? value : "")); }

inline void Property(const char* label, float value) { PropertyRow(label, Format("{:.3f}", value)); }

/// Show a vector as "x, y, z" with 3 decimals.
inline void Property(const char* label, const Vector3& value)
{
    PropertyRow(label, Format("{:.3f}, {:.3f}, {:.3f}", value.x_, value.y_, value.z_));
}

/// Show a size in a human readable form.
inline void PropertyBytes(const char* label, unsigned long long value) { PropertyRow(label, FormatBytes(value)); }

template <class T> void Property(const char* label, const T& value)
{
    PropertyRow(label, Format("{}", value));
}

inline bool TreeNode(const char* label, unsigned count)
{
    EndProperties();
    return ui::TreeNodeEx(label, ImGuiTreeNodeFlags_SpanAvailWidth, "%s (%u)", label, count);
}

/// Wrappers of the ImGui functions that close the property table before the item.
template <class... Args> bool InspectorTreeNode(const char* id, ImGuiTreeNodeFlags flags, const char* format, Args... args)
{
    EndProperties();
    return ui::TreeNodeEx(id, flags, format, args...);
}

inline bool InspectorTreeNode(const char* id, ImGuiTreeNodeFlags flags)
{
    EndProperties();
    return ui::TreeNodeEx(id, flags);
}

inline void InspectorTreePop()
{
    EndProperties();
    ui::TreePop();
}

template <class T> void InspectorPushID(T id)
{
    EndProperties();
    ui::PushID(id);
}

inline void InspectorPopID()
{
    EndProperties();
    ui::PopID();
}

inline void DrawTinyValue(const char* label, const tinygltf::Value& value)
{
    if (value.IsObject())
    {
        const auto& object = value.Get<tinygltf::Value::Object>();
        if (InspectorTreeNode(label, ImGuiTreeNodeFlags_SpanAvailWidth, "%s {%zu}", label, object.size()))
        {
            for (const auto& [key, child] : object)
                DrawTinyValue(key.c_str(), child);
            InspectorTreePop();
        }
    }
    else if (value.IsArray())
    {
        const auto& array = value.Get<tinygltf::Value::Array>();
        if (InspectorTreeNode(label, ImGuiTreeNodeFlags_SpanAvailWidth, "%s [%zu]", label, array.size()))
        {
            for (unsigned i = 0; i < array.size(); ++i)
            {
                const ea::string itemLabel = Format("[{}]", i);
                DrawTinyValue(itemLabel.c_str(), array[i]);
            }
            InspectorTreePop();
        }
    }
    else if (value.IsString())
        Property(label, value.Get<std::string>().c_str());
    else if (value.IsBool())
        Property(label, value.Get<bool>() ? "true" : "false");
    else if (value.IsInt())
        Property(label, value.Get<int>());
    else if (value.IsNumber())
        Property(label, value.GetNumberAsDouble());
    else if (value.IsBinary())
        Property(label, FormatBytes(value.Get<std::vector<unsigned char>>().size()));
    else
        Property(label, "null");
}

inline void DrawTinyExtensionsAndExtras(const tinygltf::ExtensionMap& extensions, const tinygltf::Value& extras)
{
    if (!extensions.empty() && TreeNode("Extensions", extensions.size()))
    {
        for (const auto& [name, value] : extensions)
            DrawTinyValue(name.c_str(), value);
        InspectorTreePop();
    }
    if (extras.Type() != tinygltf::NULL_TYPE)
        DrawTinyValue("Extras", extras);
}

inline const char* PrimitiveName(int mode)
{
    switch (mode)
    {
    case TINYGLTF_MODE_POINTS: return "Points";
    case TINYGLTF_MODE_LINE: return "Lines";
    case TINYGLTF_MODE_LINE_LOOP: return "Line Loop";
    case TINYGLTF_MODE_LINE_STRIP: return "Line Strip";
    case TINYGLTF_MODE_TRIANGLES: return "Triangles";
    case TINYGLTF_MODE_TRIANGLE_STRIP: return "Triangle Strip";
    case TINYGLTF_MODE_TRIANGLE_FAN: return "Triangle Fan";
    default: return "Unspecified";
    }
}

inline const char* AccessorTypeName(int type)
{
    switch (type)
    {
    case TINYGLTF_TYPE_SCALAR: return "SCALAR";
    case TINYGLTF_TYPE_VEC2: return "VEC2";
    case TINYGLTF_TYPE_VEC3: return "VEC3";
    case TINYGLTF_TYPE_VEC4: return "VEC4";
    case TINYGLTF_TYPE_MAT2: return "MAT2";
    case TINYGLTF_TYPE_MAT3: return "MAT3";
    case TINYGLTF_TYPE_MAT4: return "MAT4";
    default: return "Unknown";
    }
}

inline const char* ComponentTypeName(int type)
{
    switch (type)
    {
    case TINYGLTF_COMPONENT_TYPE_BYTE: return "BYTE";
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: return "UNSIGNED_BYTE";
    case TINYGLTF_COMPONENT_TYPE_SHORT: return "SHORT";
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: return "UNSIGNED_SHORT";
    case TINYGLTF_COMPONENT_TYPE_INT: return "INT";
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: return "UNSIGNED_INT";
    case TINYGLTF_COMPONENT_TYPE_FLOAT: return "FLOAT";
    case TINYGLTF_COMPONENT_TYPE_DOUBLE: return "DOUBLE";
    default: return "Unknown";
    }
}

inline ea::string LoadGLTFJsonChunk(Context* context, const ea::string& fileName)
{
    File file(context, fileName, FILE_READ);
    if (!file.IsOpen())
        return {};

    if (GetExtension(fileName) == ".gltf")
    {
        ea::string result;
        result.resize(file.GetSize());
        if (!result.empty())
            file.Read(result.data(), result.size());
        return result;
    }

    if (file.GetSize() < 20 || file.ReadUInt() != 0x46546c67)
        return {};
    file.ReadUInt(); // Version
    file.ReadUInt(); // Total length
    const unsigned jsonLength = file.ReadUInt();
    const unsigned chunkType = file.ReadUInt();
    if (chunkType != 0x4e4f534a || jsonLength > file.GetSize() - file.GetPosition())
        return {};

    ea::string result;
    result.resize(jsonLength);
    file.Read(result.data(), jsonLength);
    while (!result.empty()
        && (result.back() == '\0' || result.back() == ' ' || result.back() == '\n' || result.back() == '\r'))
        result.pop_back();
    return result;
}

} // namespace AssetViewerDetail

} // namespace Urho3D
