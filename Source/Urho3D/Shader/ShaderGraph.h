// SPDX-License-Identifier: MIT

#pragma once

#include <Urho3D/Urho3D.h>
#include <Urho3D/Core/Variant.h>

#include <EASTL/string.h>
#include <EASTL/unordered_map.h>
#include <EASTL/vector.h>

namespace Urho3D
{

enum class ShaderGraphValueType
{
    Float,
    Vector2,
    Vector3,
    Vector4,
    Color,
    Bool,
    Texture2D
};

enum class ShaderGraphNodeKind
{
    Constant,
    Parameter,
    Add,
    Multiply,
    Lerp,
    TextureSample,
    Output
};

enum class ShaderGraphLanguage
{
    GLSL,
    HLSL
};

struct URHO3D_API ShaderGraphParameter
{
    ea::string name;
    ShaderGraphValueType type{ShaderGraphValueType::Float};
    Variant defaultValue;
};

struct URHO3D_API ShaderGraphNode
{
    unsigned id{};
    ea::string name;
    ShaderGraphNodeKind kind{ShaderGraphNodeKind::Constant};
    ShaderGraphValueType valueType{ShaderGraphValueType::Float};
    Variant value;
};

struct URHO3D_API ShaderGraphConnection
{
    unsigned fromNode{};
    ea::string fromPin;
    unsigned toNode{};
    ea::string toPin;
};

/// Backend-neutral material graph with deterministic GLSL and HLSL generation.
class URHO3D_API ShaderGraph
{
public:
    unsigned AddNode(const ea::string& name, ShaderGraphNodeKind kind, ShaderGraphValueType valueType,
        const Variant& value = {});
    bool RemoveNode(unsigned nodeId);
    bool Connect(unsigned fromNode, const ea::string& fromPin, unsigned toNode, const ea::string& toPin);
    bool Disconnect(unsigned toNode, const ea::string& toPin);
    bool SetOutputNode(unsigned nodeId);
    bool SetParameter(const ShaderGraphParameter& parameter);
    bool RemoveParameter(const ea::string& name);

    const ShaderGraphNode* GetNode(unsigned nodeId) const;
    ShaderGraphNode* GetNode(unsigned nodeId);
    const ShaderGraphNode* GetOutputNode() const;
    const ea::vector<ShaderGraphNode>& GetNodes() const { return nodes_; }
    const ea::vector<ShaderGraphConnection>& GetConnections() const { return connections_; }
    const ea::vector<ShaderGraphParameter>& GetParameters() const { return parameters_; }

    bool Validate(ea::string* error = nullptr) const;
    ea::string Generate(ShaderGraphLanguage language, ea::string* error = nullptr) const;
    ea::string GenerateGLSL(ea::string* error = nullptr) const { return Generate(ShaderGraphLanguage::GLSL, error); }
    ea::string GenerateHLSL(ea::string* error = nullptr) const { return Generate(ShaderGraphLanguage::HLSL, error); }
    void Clear();

private:
    const ShaderGraphConnection* FindInput(unsigned nodeId, const ea::string& pin) const;
    ea::string BuildExpression(unsigned nodeId, const ea::string& pin, ShaderGraphLanguage language,
        ea::vector<unsigned>& visiting, ea::string* error) const;
    static ea::string TypeName(ShaderGraphValueType type, ShaderGraphLanguage language);
    static ea::string Literal(const Variant& value, ShaderGraphValueType type, ShaderGraphLanguage language);
    static ea::string Sanitize(const ea::string& value);

    ea::vector<ShaderGraphNode> nodes_;
    ea::vector<ShaderGraphConnection> connections_;
    ea::vector<ShaderGraphParameter> parameters_;
    unsigned outputNodeId_{};
};

} // namespace Urho3D
