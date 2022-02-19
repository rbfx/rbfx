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

#include "../Core/Profiler.h"
#include "../Core/Context.h"
#include "../IO/ArchiveSerialization.h"
#include "../IO/Deserializer.h"
#include "../IO/Log.h"
#include "../IO/MemoryBuffer.h"
#include "../Resource/JSONArchive.h"
#include "../Resource/JSONFile.h"
#include "../Resource/ResourceCache.h"

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/prettywriter.h>

#include "../DebugNew.h"

using namespace rapidjson;

namespace Urho3D
{

JSONFile::JSONFile(Context* context) :
    Resource(context)
{
}

JSONFile::~JSONFile() = default;

void JSONFile::RegisterObject(Context* context)
{
    context->RegisterFactory<JSONFile>();
}

// Convert rapidjson value to JSON value.
static void ToJSONValue(JSONValue& jsonValue, const rapidjson::Value& rapidjsonValue)
{
    switch (rapidjsonValue.GetType())
    {
    case kNullType:
        // Reset to null type
        jsonValue.SetType(JSON_NULL);
        break;

    case kFalseType:
        jsonValue = false;
        break;

    case kTrueType:
        jsonValue = true;
        break;

    case kNumberType:
        if (rapidjsonValue.IsInt())
            jsonValue = rapidjsonValue.GetInt();
        else if (rapidjsonValue.IsUint())
            jsonValue = rapidjsonValue.GetUint();
        else
            jsonValue = rapidjsonValue.GetDouble();
        break;

    case kStringType:
        jsonValue = rapidjsonValue.GetString();
        break;

    case kArrayType:
        {
            jsonValue.Resize(rapidjsonValue.Size());
            for (unsigned i = 0; i < rapidjsonValue.Size(); ++i)
            {
                ToJSONValue(jsonValue[i], rapidjsonValue[i]);
            }
        }
        break;

    case kObjectType:
        {
            jsonValue.SetType(JSON_OBJECT);
            for (rapidjson::Value::ConstMemberIterator i = rapidjsonValue.MemberBegin(); i != rapidjsonValue.MemberEnd(); ++i)
            {
                JSONValue& value = jsonValue[ea::string(i->name.GetString())];
                ToJSONValue(value, i->value);
            }
        }
        break;

    default:
        break;
    }
}

bool JSONFile::BeginLoad(Deserializer& source)
{
    unsigned dataSize = source.GetSize();
    if (!dataSize && !source.GetName().empty())
    {
        URHO3D_LOGERROR("Zero sized JSON data in " + source.GetName());
        return false;
    }

    ea::shared_array<char> buffer(new char[dataSize + 1]);
    if (source.Read(buffer.get(), dataSize) != dataSize)
        return false;
    buffer[dataSize] = '\0';

    rapidjson::Document document;
    if (document.Parse<kParseCommentsFlag | kParseTrailingCommasFlag>(buffer.get()).HasParseError())
    {
        URHO3D_LOGERROR("Could not parse JSON data from " + source.GetName());
        return false;
    }

    ToJSONValue(root_, document);

    SetMemoryUse(dataSize);

    return true;
}

static void ToRapidjsonValue(rapidjson::Value& rapidjsonValue, const JSONValue& jsonValue, rapidjson::MemoryPoolAllocator<>& allocator)
{
    switch (jsonValue.GetValueType())
    {
    case JSON_NULL:
        rapidjsonValue.SetNull();
        break;

    case JSON_BOOL:
        rapidjsonValue.SetBool(jsonValue.GetBool());
        break;

    case JSON_NUMBER:
        {
            switch (jsonValue.GetNumberType())
            {
            case JSONNT_INT:
                rapidjsonValue.SetInt(jsonValue.GetInt());
                break;

            case JSONNT_UINT:
                rapidjsonValue.SetUint(jsonValue.GetUInt());
                break;

            default:
                rapidjsonValue.SetDouble(jsonValue.GetDouble());
                break;
            }
        }
        break;

    case JSON_STRING:
        rapidjsonValue.SetString(jsonValue.GetCString(), allocator);
        break;

    case JSON_ARRAY:
        {
            const JSONArray& jsonArray = jsonValue.GetArray();

            rapidjsonValue.SetArray();
            rapidjsonValue.Reserve(jsonArray.size(), allocator);

            for (unsigned i = 0; i < jsonArray.size(); ++i)
            {
                rapidjson::Value value;
                ToRapidjsonValue(value, jsonArray[i], allocator);
                rapidjsonValue.PushBack(value, allocator);
            }
        }
        break;

    case JSON_OBJECT:
        {
            const JSONObject& jsonObject = jsonValue.GetObject();

            rapidjsonValue.SetObject();
            for (auto i = jsonObject.begin(); i != jsonObject.end(); ++i)
            {
                const char* name = i->first.c_str();
                rapidjson::Value value;
                ToRapidjsonValue(value, i->second, allocator);
                rapidjsonValue.AddMember(StringRef(name), value, allocator);
            }
        }
        break;

    default:
        break;
    }
}

bool JSONFile::Save(Serializer& dest) const
{
    return Save(dest, "\t");
}

bool JSONFile::Save(Serializer& dest, const ea::string& indendation) const
{
    rapidjson::Document document;
    ToRapidjsonValue(document, root_, document.GetAllocator());

    StringBuffer buffer;
    PrettyWriter<StringBuffer> writer(buffer);
    writer.SetIndent(!indendation.empty() ? indendation.front() : '\0', indendation.length());

    document.Accept(writer);
    auto size = (unsigned)buffer.GetSize();
    return dest.Write(buffer.GetString(), size) == size;
}

bool JSONFile::SaveObjectCallback(const ea::function<void(Archive&)> serializeValue)
{
    try
    {
        root_.Clear();
        JSONOutputArchive archive{this};
        serializeValue(archive);
        return true;
    }
    catch (const ArchiveException& e)
    {
        root_.Clear();
        URHO3D_LOGERROR("Failed to save object to JSON: {}", e.what());
        return false;
    }
}

bool JSONFile::LoadObjectCallback(const ea::function<void(Archive&)> serializeValue) const
{
    try
    {
        JSONInputArchive archive{this};
        serializeValue(archive);
        return true;
    }
    catch (const ArchiveException& e)
    {
        URHO3D_LOGERROR("Failed to load object from JSON: {}", e.what());
        return false;
    }
}

bool JSONFile::FromString(const ea::string & source)
{
    if (source.empty())
        return false;

    MemoryBuffer buffer(source.c_str(), source.length());
    return Load(buffer);
}

bool JSONFile::ParseJSON(const ea::string& json, JSONValue& value, bool reportError)
{
    rapidjson::Document document;
    if (document.Parse<0>(json.c_str()).HasParseError())
    {
        if (reportError)
            URHO3D_LOGERRORF("Could not parse JSON data from string with error: %s", document.GetParseError());

        return false;
    }
    ToJSONValue(value, document);
    return true;
}

ea::string JSONFile::ToString(const ea::string& indendation) const
{
    rapidjson::Document document;
    ToRapidjsonValue(document, root_, document.GetAllocator());

    StringBuffer buffer;
    PrettyWriter<StringBuffer> writer(buffer);
    writer.SetIndent(!indendation.empty() ? indendation.front() : ' ', indendation.length());

    document.Accept(writer);
    return buffer.GetString();
}

}
