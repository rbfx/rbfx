// SPDX-License-Identifier: MIT

#include "ShaderGraph.h"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace Urho3D
{

unsigned ShaderGraph::AddNode(const ea::string& name, ShaderGraphNodeKind kind, ShaderGraphValueType valueType,
    const Variant& value)
{
    if (name.empty())
        return 0;
    ShaderGraphNode node;
    node.id = nodes_.size() + 1;
    node.name = name;
    node.kind = kind;
    node.valueType = valueType;
    node.value = value;
    nodes_.push_back(node);
    if (kind == ShaderGraphNodeKind::Output)
        outputNodeId_ = node.id;
    return node.id;
}

bool ShaderGraph::RemoveNode(unsigned nodeId)
{
    if (!GetNode(nodeId))
        return false;
    connections_.erase(std::remove_if(connections_.begin(), connections_.end(), [&](const ShaderGraphConnection& connection)
    {
        return connection.fromNode == nodeId || connection.toNode == nodeId;
    }), connections_.end());
    if (outputNodeId_ == nodeId)
        outputNodeId_ = 0;
    ShaderGraphNode* node = GetNode(nodeId);
    node->id = 0;
    node->name.clear();
    return true;
}

bool ShaderGraph::Connect(unsigned fromNode, const ea::string& fromPin, unsigned toNode, const ea::string& toPin)
{
    if (!GetNode(fromNode) || !GetNode(toNode) || fromNode == toNode || fromPin.empty() || toPin.empty())
        return false;
    if (FindInput(toNode, toPin))
        return false;
    connections_.push_back({fromNode, fromPin, toNode, toPin});
    return true;
}

bool ShaderGraph::Disconnect(unsigned toNode, const ea::string& toPin)
{
    const auto iter = std::find_if(connections_.begin(), connections_.end(), [&](const ShaderGraphConnection& connection)
    {
        return connection.toNode == toNode && connection.toPin == toPin;
    });
    if (iter == connections_.end())
        return false;
    connections_.erase(iter);
    return true;
}

bool ShaderGraph::SetOutputNode(unsigned nodeId)
{
    const ShaderGraphNode* node = GetNode(nodeId);
    if (!node || node->kind != ShaderGraphNodeKind::Output)
        return false;
    outputNodeId_ = nodeId;
    return true;
}

bool ShaderGraph::SetParameter(const ShaderGraphParameter& parameter)
{
    if (parameter.name.empty())
        return false;
    for (ShaderGraphParameter& existing : parameters_)
    {
        if (existing.name == parameter.name)
        {
            existing = parameter;
            return true;
        }
    }
    parameters_.push_back(parameter);
    return true;
}

bool ShaderGraph::RemoveParameter(const ea::string& name)
{
    const auto iter = std::find_if(parameters_.begin(), parameters_.end(), [&](const ShaderGraphParameter& parameter)
    {
        return parameter.name == name;
    });
    if (iter == parameters_.end())
        return false;
    parameters_.erase(iter);
    return true;
}

const ShaderGraphNode* ShaderGraph::GetNode(unsigned nodeId) const
{
    if (nodeId == 0 || nodeId > nodes_.size() || nodes_[nodeId - 1].id != nodeId)
        return nullptr;
    return &nodes_[nodeId - 1];
}

ShaderGraphNode* ShaderGraph::GetNode(unsigned nodeId)
{
    return const_cast<ShaderGraphNode*>(static_cast<const ShaderGraph*>(this)->GetNode(nodeId));
}

const ShaderGraphNode* ShaderGraph::GetOutputNode() const
{
    return GetNode(outputNodeId_);
}

bool ShaderGraph::Validate(ea::string* error) const
{
    if (outputNodeId_ == 0 || !GetOutputNode() || GetOutputNode()->kind != ShaderGraphNodeKind::Output)
    {
        if (error)
            *error = "ShaderGraph requires one Output node.";
        return false;
    }
    ea::unordered_map<ea::string, bool> parameterNames;
    for (const ShaderGraphParameter& parameter : parameters_)
    {
        if (parameter.name.empty() || parameterNames.find(parameter.name) != parameterNames.end())
        {
            if (error)
                *error = "ShaderGraph parameter names must be unique and non-empty.";
            return false;
        }
        parameterNames[parameter.name] = true;
    }
    ea::unordered_map<unsigned, ea::unordered_map<ea::string, bool>> pins;
    for (const ShaderGraphConnection& connection : connections_)
    {
        if (!GetNode(connection.fromNode) || !GetNode(connection.toNode) || connection.fromPin.empty() || connection.toPin.empty())
        {
            if (error)
                *error = "ShaderGraph contains an invalid connection.";
            return false;
        }
        if (pins[connection.toNode].find(connection.toPin) != pins[connection.toNode].end())
        {
            if (error)
                *error = "ShaderGraph input pins accept only one connection.";
            return false;
        }
        pins[connection.toNode][connection.toPin] = true;
    }
    ea::vector<unsigned> visiting;
    ea::string expressionError;
    if (BuildExpression(outputNodeId_, "color", ShaderGraphLanguage::GLSL, visiting, &expressionError).empty())
    {
        if (error)
            *error = expressionError.empty() ? "ShaderGraph output cannot be evaluated." : expressionError;
        return false;
    }
    return true;
}

ea::string ShaderGraph::Generate(ShaderGraphLanguage language, ea::string* error) const
{
    if (!Validate(error))
        return {};
    ea::vector<unsigned> visiting;
    ea::string expression = BuildExpression(outputNodeId_, "color", language, visiting, error);
    if (expression.empty())
        return {};

    ea::string source;
    if (language == ShaderGraphLanguage::GLSL)
    {
        source += "#version 330 core\n";
        source += "in vec2 vUv;\n";
        source += "layout(location = 0) out vec4 fragColor;\n";
        for (const ShaderGraphParameter& parameter : parameters_)
        {
            source += "uniform ";
            source += TypeName(parameter.type, language);
            source += " u_";
            source += Sanitize(parameter.name);
            source += ";\n";
        }
        source += "void main() { fragColor = ";
        source += expression;
        source += "; }\n";
    }
    else
    {
        source += "struct PSInput { float2 uv : TEXCOORD0; };\n";
        for (const ShaderGraphParameter& parameter : parameters_)
        {
            source += TypeName(parameter.type, language);
            source += " u_";
            source += Sanitize(parameter.name);
            source += ";\n";
        }
        source += "float4 main(PSInput input) : SV_Target { return ";
        source += expression;
        source += "; }\n";
    }
    return source;
}

void ShaderGraph::Clear()
{
    nodes_.clear();
    connections_.clear();
    parameters_.clear();
    outputNodeId_ = 0;
}

const ShaderGraphConnection* ShaderGraph::FindInput(unsigned nodeId, const ea::string& pin) const
{
    for (const ShaderGraphConnection& connection : connections_)
    {
        if (connection.toNode == nodeId && connection.toPin == pin)
            return &connection;
    }
    return nullptr;
}

ea::string ShaderGraph::BuildExpression(unsigned nodeId, const ea::string& pin, ShaderGraphLanguage language,
    ea::vector<unsigned>& visiting, ea::string* error) const
{
    const ShaderGraphNode* node = GetNode(nodeId);
    if (!node)
    {
        if (error)
            *error = "ShaderGraph references a missing node.";
        return {};
    }
    if (std::find(visiting.begin(), visiting.end(), nodeId) != visiting.end())
    {
        if (error)
            *error = "ShaderGraph contains a cycle.";
        return {};
    }
    visiting.push_back(nodeId);
    auto input = [&](const char* inputPin, const char* fallback) -> ea::string
    {
        const ShaderGraphConnection* connection = FindInput(nodeId, inputPin);
        return connection ? BuildExpression(connection->fromNode, connection->fromPin, language, visiting, error) : ea::string(fallback);
    };

    ea::string expression;
    switch (node->kind)
    {
    case ShaderGraphNodeKind::Constant:
        expression = Literal(node->value, node->valueType, language);
        break;
    case ShaderGraphNodeKind::Parameter:
    {
        const ea::string parameterName = node->value.GetType() == VAR_STRING ? node->value.GetString() : node->name;
        expression = "u_" + Sanitize(parameterName);
        break;
    }
    case ShaderGraphNodeKind::Add:
        expression = "(" + input("a", "0.0") + " + " + input("b", "0.0") + ")";
        break;
    case ShaderGraphNodeKind::Multiply:
        expression = "(" + input("a", "1.0") + " * " + input("b", "1.0") + ")";
        break;
    case ShaderGraphNodeKind::Lerp:
        if (language == ShaderGraphLanguage::GLSL)
            expression = "mix(" + input("a", "0.0") + ", " + input("b", "1.0") + ", " + input("alpha", "0.5") + ")";
        else
            expression = "lerp(" + input("a", "0.0") + ", " + input("b", "1.0") + ", " + input("alpha", "0.5") + ")";
        break;
    case ShaderGraphNodeKind::TextureSample:
    {
        const ea::string textureName = node->value.GetType() == VAR_STRING ? node->value.GetString() : node->name;
        const ea::string uv = input("uv", language == ShaderGraphLanguage::GLSL ? "vUv" : "input.uv");
        expression = language == ShaderGraphLanguage::GLSL
            ? "texture(u_" + Sanitize(textureName) + ", " + uv + ")"
            : "u_" + Sanitize(textureName) + ".Sample(u_" + Sanitize(textureName) + "Sampler, " + uv + ")";
        break;
    }
    case ShaderGraphNodeKind::Output:
        expression = input(pin.c_str(), "vec4(0.0)");
        if (expression == "vec4(0.0)" && language == ShaderGraphLanguage::HLSL)
            expression = "float4(0.0, 0.0, 0.0, 0.0)";
        break;
    }
    visiting.pop_back();
    return expression;
}

ea::string ShaderGraph::TypeName(ShaderGraphValueType type, ShaderGraphLanguage language)
{
    if (language == ShaderGraphLanguage::GLSL)
    {
        switch (type)
        {
        case ShaderGraphValueType::Float: return "float";
        case ShaderGraphValueType::Vector2: return "vec2";
        case ShaderGraphValueType::Vector3: return "vec3";
        case ShaderGraphValueType::Vector4: case ShaderGraphValueType::Color: return "vec4";
        case ShaderGraphValueType::Bool: return "bool";
        case ShaderGraphValueType::Texture2D: return "sampler2D";
        }
    }
    switch (type)
    {
    case ShaderGraphValueType::Float: return "float";
    case ShaderGraphValueType::Vector2: return "float2";
    case ShaderGraphValueType::Vector3: return "float3";
    case ShaderGraphValueType::Vector4: case ShaderGraphValueType::Color: return "float4";
    case ShaderGraphValueType::Bool: return "bool";
    case ShaderGraphValueType::Texture2D: return "Texture2D";
    }
    return "float";
}

ea::string ShaderGraph::Literal(const Variant& value, ShaderGraphValueType type, ShaderGraphLanguage language)
{
    const ea::string prefix = language == ShaderGraphLanguage::GLSL ? "vec" : "float";
    std::ostringstream stream;
    stream.precision(9);
    switch (type)
    {
    case ShaderGraphValueType::Float:
        stream << value.GetFloat();
        return stream.str().c_str();
    case ShaderGraphValueType::Vector2:
    {
        const Vector2 v = value.GetVector2();
        stream << prefix.c_str() << "2(" << v.x_ << ", " << v.y_ << ")";
        return stream.str().c_str();
    }
    case ShaderGraphValueType::Vector3:
    {
        const Vector3 v = value.GetVector3();
        stream << prefix.c_str() << "3(" << v.x_ << ", " << v.y_ << ", " << v.z_ << ")";
        return stream.str().c_str();
    }
    case ShaderGraphValueType::Vector4:
    case ShaderGraphValueType::Color:
    {
        const Color color = value.GetColor();
        const Vector4 v = type == ShaderGraphValueType::Color ? Vector4(color.r_, color.g_, color.b_, color.a_) : value.GetVector4();
        stream << prefix.c_str() << "4(" << v.x_ << ", " << v.y_ << ", " << v.z_ << ", " << v.w_ << ")";
        return stream.str().c_str();
    }
    case ShaderGraphValueType::Bool:
        return value.GetBool() ? "true" : "false";
    case ShaderGraphValueType::Texture2D:
        return "0";
    }
    return "0.0";
}

ea::string ShaderGraph::Sanitize(const ea::string& value)
{
    ea::string result;
    for (char character : value)
        result += std::isalnum(static_cast<unsigned char>(character)) || character == '_' ? character : '_';
    if (result.empty() || std::isdigit(static_cast<unsigned char>(result.front())))
        result = "p_" + result;
    return result;
}

} // namespace Urho3D
