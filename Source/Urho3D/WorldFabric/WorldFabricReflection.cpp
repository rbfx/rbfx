// Copyright (c) 2026 rbfx-blueprint contributors.
// SPDX-License-Identifier: MIT

#include "WorldFabricReflection.h"

#include <Urho3D/Core/Context.h>
#include <Urho3D/Core/ObjectReflection.h>
#include <Urho3D/RbScript/RbScriptType.h>

#include <algorithm>

namespace Urho3D
{

unsigned WorldFabricReflection::RegisterObjectReflection(Context* context, WorldFabricGraph& graph)
{
    if (!context)
        return 0;

    unsigned count = 0;
    ea::vector<const ObjectReflection*> reflections;
    reflections.reserve(context->GetObjectReflections().size());
    for (const auto& entry : context->GetObjectReflections())
    {
        if (entry.second)
            reflections.push_back(entry.second);
    }
    std::sort(reflections.begin(), reflections.end(), [](const ObjectReflection* lhs, const ObjectReflection* rhs)
    {
        return lhs->GetTypeName() < rhs->GetTypeName();
    });

    for (const ObjectReflection* reflection : reflections)
    {
        const ea::string typeName = reflection->GetTypeName();
        if (typeName.empty())
            continue;

        const ea::string typeKey = "reflection/type/" + typeName;
        StringVariantMap typeMetadata;
        typeMetadata["category"] = Variant(reflection->GetCategory());
        typeMetadata["attributeCount"] = Variant(static_cast<int>(reflection->GetNumAttributes()));
        const WorldFabricId typeId = graph.AddNode(typeKey, WorldFabricNodeKind::Component, typeName, typeMetadata);
        if (typeId != InvalidWorldFabricId)
            ++count;

        ea::vector<const AttributeInfo*> attributes;
        attributes.reserve(reflection->GetAttributes().size());
        for (const AttributeInfo& attribute : reflection->GetAttributes())
        {
            if (!(attribute.mode_ & AM_NOEDIT))
                attributes.push_back(&attribute);
        }
        std::sort(attributes.begin(), attributes.end(), [](const AttributeInfo* lhs, const AttributeInfo* rhs)
        {
            return lhs->name_ < rhs->name_;
        });

        for (const AttributeInfo* attribute : attributes)
        {
            const ea::string propertyKey = typeKey + "/property/" + attribute->name_;
            StringVariantMap metadata;
            metadata["owner"] = Variant(typeName);
            metadata["variantType"] = Variant(static_cast<int>(attribute->type_));
            metadata["mode"] = Variant(static_cast<int>(attribute->mode_));
            metadata["scope"] = Variant(static_cast<int>(attribute->scopeHint_));
            metadata["default"] = attribute->defaultValue_;
            const WorldFabricId propertyId = graph.AddNode(propertyKey, WorldFabricNodeKind::Component,
                attribute->name_, metadata);
            if (propertyId == InvalidWorldFabricId)
                continue;
            ++count;
            if (typeId != InvalidWorldFabricId)
                graph.AddDependency(propertyId, typeId, WorldFabricDependencyKind::Requires, "declared-by");
        }
    }
    return count;
}

unsigned WorldFabricReflection::RegisterRbScriptReflection(const RbScriptTypeRegistry& registry, WorldFabricGraph& graph)
{
    unsigned count = 0;
    ea::vector<ea::string> typeNames = registry.GetTypeNames();
    std::sort(typeNames.begin(), typeNames.end());
    for (const ea::string& typeName : typeNames)
    {
        const RbScriptType type = registry.Resolve(typeName);
        const ea::string key = "rbscript/type/" + typeName;
        StringVariantMap metadata;
        metadata["kind"] = Variant(static_cast<int>(type.kind));
        metadata["type"] = Variant(type.ToString());
        if (graph.AddNode(key, WorldFabricNodeKind::RbScript, typeName, metadata) != InvalidWorldFabricId)
            ++count;
    }

    ea::vector<ea::string> functionNames = registry.GetFunctionNames();
    std::sort(functionNames.begin(), functionNames.end());
    for (const ea::string& functionName : functionNames)
    {
        const RbScriptFunctionSignature* function = registry.FindFunction(functionName);
        if (!function)
            continue;
        const ea::string key = "rbscript/function/" + functionName;
        StringVariantMap metadata;
        metadata["returnType"] = Variant(function->returnType.ToString());
        metadata["parameterCount"] = Variant(static_cast<int>(function->parameterTypes.size()));
        metadata["asynchronous"] = Variant(function->asynchronous);
        metadata["blueprintCallable"] = Variant(function->blueprintCallable);
        if (graph.AddNode(key, WorldFabricNodeKind::RbScript, functionName, metadata) != InvalidWorldFabricId)
            ++count;
    }
    return count;
}

} // namespace Urho3D
