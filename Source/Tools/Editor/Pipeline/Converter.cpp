//
// Copyright (c) 2017-2019 the rbfx project.
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

#include <Urho3D/Core/Context.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Resource/JSONValue.h>
#include "Converter.h"


Urho3D::Converter::Converter(Urho3D::Context* context)
    : Serializable(context)
{
}

void Urho3D::Converter::RegisterObject(Context* context)
{
    URHO3D_ATTRIBUTE("comment", ea::string, comment_, EMPTY_STRING, AM_DEFAULT);
    URHO3D_ENUM_ATTRIBUTE("kind", kind_, converterKindNames, CONVERTER_OFFLINE, AM_DEFAULT);
}

bool Urho3D::Converter::LoadJSON(const Urho3D::JSONValue& source)
{
    if (!Serializable::LoadJSON(source))
        return false;

    if (source.Contains("output"))
    {
        auto loadConverter = [&](const JSONValue& value) -> bool {
            if (!value.IsObject())
                return false;

            StringHash type = GetSerializedType(value);
            SharedPtr<Converter> converter = DynamicCast<Converter>(context_->CreateObject(type));
            if (!converter)
                return false;

            if (!converter->LoadJSON(value))
                return false;

            converters_.push_back(converter);
            return true;
        };

        const JSONValue& output = source["output"];
        if (output.IsObject())
        {
            if (!loadConverter(output))
                return false;
        }
        else if (output.GetValueType() == JSON_ARRAY)
        {
            for (auto i = 0U; i < output.Size(); i++)
            {
                if (!loadConverter(output[i]))
                    return false;
            }
        }
        else
            return false;
    }

    return true;
}

void Urho3D::Converter::Execute(const StringVector& input)
{
    for (SharedPtr<Converter>& converter : converters_)
        converter->Execute(input);
}

Urho3D::StringHash Urho3D::Converter::GetSerializedType(const Urho3D::JSONValue& source)
{
    if (!source.IsObject())
    {
        URHO3D_LOGERROR("Source is not an object.");
        return StringHash::ZERO;
    }

    if (!source.Contains("type"))
    {
        URHO3D_LOGERROR("Source does not contain 'Type' field.");
        return StringHash::ZERO;
    }

    return source["type"].GetString();
}
