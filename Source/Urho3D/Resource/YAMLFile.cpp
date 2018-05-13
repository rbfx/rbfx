//
// Copyright (c) 2008-2018 the Urho3D project.
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

#include "../Container/ArrayPtr.h"
#include "../Core/Profiler.h"
#include "../Core/Context.h"
#include "../IO/Deserializer.h"
#include "../IO/Log.h"
#include "../IO/MemoryBuffer.h"
#include "../Resource/YAMLFile.h"
#include "../Resource/ResourceCache.h"

#include <regex>
#include <yaml-cpp/yaml.h>

#include "../DebugNew.h"

namespace YAML
{

template<>
struct convert<Urho3D::String>
{
    static Node encode(const Urho3D::String& rhs)
    {
        return Node(rhs.CString());
    }

    static bool decode(const Node& node, Urho3D::String& rhs)
    {
        if(!node.IsScalar())
            return false;

        rhs = node.Scalar().c_str();
        return true;
    }
};

}

namespace Urho3D
{

YAMLFile::YAMLFile(Context* context)
    : Resource(context)
{

}

void YAMLFile::RegisterObject(Context* context)
{
    context->RegisterFactory<YAMLFile>();
}

static std::regex isNumeric("-?[0-9]+(\\.[0-9]+)?");

// Convert rapidjson value to JSON value.
static void ToJSONValue(JSONValue& jsonValue, const YAML::Node& yamlValue)
{
    switch (yamlValue.Type())
    {
    case YAML::NodeType::Null:
        // Reset to null type
        jsonValue.SetType(JSON_NULL);
        break;

    case YAML::NodeType::Scalar:
    {
        // Yaml does not make a distinction between data types. Instead of storing all values as strings we make a best
        // guess here.

        // `true` and `false` strings are treated as booleans
        String value = yamlValue.Scalar();
        if (value == "true")
            jsonValue = true;
        else if (value == "false")
            jsonValue = false;
        else
        {
            // All numbers are treated as doubles
            const auto& scalar = yamlValue.Scalar();

            if (std::regex_match(scalar, isNumeric))
                jsonValue = yamlValue.as<double>();
            else
                jsonValue = scalar.c_str();
        }

        break;
    }

    case YAML::NodeType::Sequence:
    {
        jsonValue.Resize(yamlValue.size());
        for (unsigned i = 0; i < yamlValue.size(); ++i)
        {
            ToJSONValue(jsonValue[i], yamlValue[i]);
        }
        break;
    }

    case YAML::NodeType::Map:
    {
        jsonValue.SetType(JSON_OBJECT);
        for (auto i = yamlValue.begin(); i != yamlValue.end(); ++i)
        {
            JSONValue& value = jsonValue[i->first.as<String>()];
            ToJSONValue(value, i->second);
        }
        break;
    }

    default:
        break;
    }
}
bool YAMLFile::BeginLoad(Deserializer& source)
{
    unsigned dataSize = source.GetSize();
    if (!dataSize && !source.GetName().Empty())
    {
        URHO3D_LOGERROR("Zero sized JSON data in " + source.GetName());
        return false;
    }

    SharedArrayPtr<char> buffer(new char[dataSize + 1]);
    if (source.Read(buffer.Get(), dataSize) != dataSize)
        return false;
    buffer[dataSize] = '\0';

    auto document = YAML::Load(buffer.Get());

    if (!document.IsDefined())
    {
        URHO3D_LOGERROR("Could not parse JSON data from " + source.GetName());
        return false;
    }

    ToJSONValue(root_, document);

    SetMemoryUse(dataSize);

    return true;
}

static void ToYAMLValue(YAML::Node& yamlValue, const JSONValue& jsonValue)
{
    switch (jsonValue.GetValueType())
    {
    case JSON_NULL:
        yamlValue = YAML::Node(YAML::NodeType::Null);
        break;

    case JSON_BOOL:
        yamlValue = jsonValue.GetBool() ? "true" : "false";
        break;

    case JSON_NUMBER:
    {
        switch (jsonValue.GetNumberType())
        {
        case JSONNT_INT:
            yamlValue = jsonValue.GetInt();
            break;

        case JSONNT_UINT:
            yamlValue = jsonValue.GetUInt();
            break;

        default:
            yamlValue = jsonValue.GetDouble();
            break;
        }
        break;
    }

    case JSON_STRING:
        yamlValue = jsonValue.GetString();
        break;

    case JSON_ARRAY:
    {
        const JSONArray& jsonArray = jsonValue.GetArray();

        yamlValue = YAML::Node(YAML::NodeType::Sequence);

        for (unsigned i = 0; i < jsonArray.Size(); ++i)
        {
            YAML::Node value;
            ToYAMLValue(value, jsonArray[i]);
            yamlValue.push_back(value);
        }
        break;
    }

    case JSON_OBJECT:
    {
        const JSONObject& jsonObject = jsonValue.GetObject();

        yamlValue = YAML::Node(YAML::NodeType::Map);
        for (JSONObject::ConstIterator i = jsonObject.Begin(); i != jsonObject.End(); ++i)
        {
            YAML::Node value;
            ToYAMLValue(value, i->second_);
            yamlValue[i->first_] = value;
        }
        break;
    }

    default:
        break;
    }
}

bool YAMLFile::Save(Serializer& dest) const
{
    return Save(dest, 2);
}

bool YAMLFile::Save(Serializer& dest, int indendation) const
{
    YAML::Node document;
    ToYAMLValue(document, root_);

    YAML::Emitter out;
    out << YAML::Indent(indendation);
    out << document;

    auto size = (unsigned)strlen(out.c_str());
    return dest.Write(out.c_str(), size) == size;
}

bool YAMLFile::FromString(const String& source)
{
    return ParseYAML(source, root_, false);
}

bool YAMLFile::ParseYAML(const String& yaml, JSONValue& value, bool reportError)
{
    if (yaml.Empty())
        return false;

    auto yamlNode = YAML::Load(yaml.CString());
    if (!yamlNode.IsDefined())
    {
        if (reportError)
            URHO3D_LOGERRORF("Could not parse YAML data from string");
        return false;
    }

    ToJSONValue(value, yamlNode);
    return true;
}

}
