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

#pragma once

#include "../Scene/Component.h"

namespace Urho3D
{

/// component that stored data nessecary to recreate another component.
class URHO3D_API UnknownComponent : public Component
{
	URHO3D_OBJECT(UnknownComponent, Component)
public:

    /// Construct.
    explicit UnknownComponent(Context* context);

    /// Register object factory.
    static void RegisterObject(Context* context);
   
    /// Initialize the type name. Called by Node when loading.
    void SetStoredTypeName(const String& typeName);
    /// Initialize the type hash only when type name not known. Called by Node when loading.
    void SetStoredType(StringHash typeHash);
    /// Return type of the stored component.
    StringHash GetStoredType() const  { return typeHashStored_; }
    /// Return type name of the stored component.
    const String& GetStoredTypeName() const  { return typeNameStored_; }


private:
    /// Type of stored component.
    StringHash typeHashStored_;
    /// Type name of the stored component.
    String typeNameStored_;
};

}
