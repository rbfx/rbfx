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

static HashMap<StringHash, String> unknownTypeToName;
static String letters("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");

static String GenerateNameFromType(StringHash typeHash)
{
    if (unknownTypeToName.Contains(typeHash))
        return unknownTypeToName[typeHash];

    String test;

    // Begin brute-force search
    unsigned numLetters = letters.Length();
    unsigned combinations = numLetters;
    bool found = false;

    for (unsigned i = 1; i < 6; ++i)
    {
        test.Resize(i);

        for (unsigned j = 0; j < combinations; ++j)
        {
            unsigned current = j;

            for (unsigned k = 0; k < i; ++k)
            {
                test[k] = letters[current % numLetters];
                current /= numLetters;
            }

            if (StringHash(test) == typeHash)
            {
                found = true;
                break;
            }
        }

        if (found)
            break;

        combinations *= numLetters;
    }

    unknownTypeToName[typeHash] = test;
    return test;
}

UnknownComponent::UnknownComponent(Context* context) :
    Component(context)
{
}

void UnknownComponent::RegisterObject(Context* context)
{
    context->RegisterFactory<UnknownComponent>();
}

bool UnknownComponent::Load(Deserializer& source)
{
	typeNameStored_ = source.ReadString();
	return Component::Load(source);
}

bool UnknownComponent::LoadXML(const XMLElement& source)
{
	if (source.HasAttribute("storedTypeName"))
		typeNameStored_ = source.GetAttribute("storedTypeName");
	else
		return false;


	return Component::LoadXML(source);
}


bool UnknownComponent::LoadJSON(const JSONValue& source)
{
	if (source.Contains("storedTypeName"))
		typeNameStored_ = source.Get("storedTypeName").GetString();
	else
		return false;

	return Component::LoadJSON(source);
}


bool UnknownComponent::Save(Serializer& dest) const
{
	// Write the stored typename
	dest.WriteString(typeNameStored_);


	return Component::Save(dest);
}

bool UnknownComponent::SaveXML(XMLElement& dest) const
{
	// Write the stored typename
	if (!dest.SetString("storedTypeName", typeNameStored_))
		return false;

	return Component::SaveXML(dest);
}

bool UnknownComponent::SaveJSON(JSONValue& dest) const
{
	dest.Set("storedTypeName", typeNameStored_);
	return Component::SaveJSON(dest);
}

void UnknownComponent::SetStoredTypeName(const String& typeName)
{
    typeNameStored_ = typeName;
    typeHashStored_ = typeName;
}

void UnknownComponent::SetStoredType(StringHash typeHash)
{
    typeNameStored_ = GenerateNameFromType(typeHash);
    typeHashStored_ = typeHash;
}

}
