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

#include "../Core/Context.h"
#include "../IO/Deserializer.h"
#include "../IO/Log.h"
#include "../IO/Serializer.h"
#include "../Resource/XMLElement.h"
#include "../Resource/JSONValue.h"
#include "../Scene/UnknownComponent.h"

#include "../DebugNew.h"

namespace Urho3D
{

UnknownComponent::UnknownComponent(Context* context) :
    Component(context),
    useXML_(false)
{
}

void UnknownComponent::RegisterObject(Context* context)
{
    context->RegisterFactory<UnknownComponent>();
}

bool UnknownComponent::Load(Deserializer& source)
{
    useXML_ = false;
    xmlAttributes_.clear();
    xmlAttributeInfos_.clear();

    // Assume we are reading from a component data buffer, and the type has already been read
    unsigned dataSize = source.GetSize() - source.GetPosition();
    binaryAttributes_.resize(dataSize);
    return dataSize ? source.Read(&binaryAttributes_[0], dataSize) == dataSize : true;
}

bool UnknownComponent::LoadXML(const XMLElement& source)
{
    useXML_ = true;
    xmlAttributes_.clear();
    xmlAttributeInfos_.clear();
    binaryAttributes_.clear();

    XMLElement attrElem = source.GetChild("attribute");
    while (attrElem)
    {
        AttributeInfo attr;
        attr.mode_ = AM_FILE;
        attr.name_ = attrElem.GetAttribute("name");
        attr.type_ = VAR_STRING;

        if (!attr.name_.empty())
        {
            ea::string attrValue = attrElem.GetAttribute("value");
            attr.defaultValue_ = EMPTY_STRING;
            xmlAttributeInfos_.push_back(attr);
            xmlAttributes_.push_back(attrValue);
        }

        attrElem = attrElem.GetNext("attribute");
    }

    // Fix up pointers to the attributes after all have been read
    for (unsigned i = 0; i < xmlAttributeInfos_.size(); ++i)
        xmlAttributeInfos_[i].ptr_ = &xmlAttributes_[i];

    return true;
}


bool UnknownComponent::LoadJSON(const JSONValue& source)
{
    useXML_ = true;
    xmlAttributes_.clear();
    xmlAttributeInfos_.clear();
    binaryAttributes_.clear();

    JSONArray attributesArray = source.Get("attributes").GetArray();
    for (unsigned i = 0; i < attributesArray.size(); i++)
    {
        const JSONValue& attrVal = attributesArray.at(i);

        AttributeInfo attr;
        attr.mode_ = AM_FILE;
        attr.name_ = attrVal.Get("name").GetString();
        attr.type_ = VAR_STRING;

        if (!attr.name_.empty())
        {
            ea::string attrValue = attrVal.Get("value").GetString();
            attr.defaultValue_ = EMPTY_STRING;
            xmlAttributeInfos_.push_back(attr);
            xmlAttributes_.push_back(attrValue);
        }
    }

    // Fix up pointers to the attributes after all have been read
    for (unsigned i = 0; i < xmlAttributeInfos_.size(); ++i)
        xmlAttributeInfos_[i].ptr_ = &xmlAttributes_[i];

    return true;
}


bool UnknownComponent::Save(Serializer& dest) const
{
    if (useXML_)
        URHO3D_LOGWARNING("UnknownComponent loaded in XML mode, attributes will be empty for binary save");

    // Write type and ID
    if (!dest.WriteStringHash(GetType()))
        return false;
    if (!dest.WriteUInt(id_))
        return false;

    if (!binaryAttributes_.size())
        return true;
    else
        return dest.Write(&binaryAttributes_[0], binaryAttributes_.size()) == binaryAttributes_.size();
}

bool UnknownComponent::SaveXML(XMLElement& dest) const
{
    if (dest.IsNull())
    {
        URHO3D_LOGERROR("Could not save " + GetTypeName() + ", null destination element");
        return false;
    }

    if (!useXML_)
        URHO3D_LOGWARNING("UnknownComponent loaded in binary or JSON mode, attributes will be empty for XML save");

    // Write type and ID
    if (!dest.SetString("type", GetTypeName()))
        return false;
    if (!dest.SetInt("id", id_))
        return false;

    for (unsigned i = 0; i < xmlAttributeInfos_.size(); ++i)
    {
        XMLElement attrElem = dest.CreateChild("attribute");
        attrElem.SetAttribute("name", xmlAttributeInfos_[i].name_);
        attrElem.SetAttribute("value", xmlAttributes_[i]);
    }

    return true;
}

bool UnknownComponent::SaveJSON(JSONValue& dest) const
{
    if (!useXML_)
        URHO3D_LOGWARNING("UnknownComponent loaded in binary mode, attributes will be empty for JSON save");

    // Write type and ID
    dest.Set("type", GetTypeName());
    dest.Set("id", (int) id_);

    JSONArray attributesArray;
    attributesArray.reserve(xmlAttributeInfos_.size());
    for (unsigned i = 0; i < xmlAttributeInfos_.size(); ++i)
    {
        JSONValue attrVal;
        attrVal.Set("name", xmlAttributeInfos_[i].name_);
        attrVal.Set("value", xmlAttributes_[i]);
        attributesArray.push_back(attrVal);
    }
    dest.Set("attributes", attributesArray);

    return true;
}

void UnknownComponent::SetTypeName(const ea::string& typeName)
{
    typeName_ = typeName;
    typeHash_ = typeName;
}

void UnknownComponent::SetType(StringHash typeHash)
{
#if URHO3D_HASH_DEBUG
    StringHashRegister* registry = StringHash::GetGlobalStringHashRegister();
    if (registry->Contains(typeHash))
        typeName_ = registry->GetString(typeHash);
#endif

    if (typeName_.empty())
        typeName_ = typeHash.ToString();

    typeHash_ = typeHash;
}

}
