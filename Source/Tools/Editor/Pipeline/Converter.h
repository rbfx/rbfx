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

#pragma once


#include <Urho3D/Scene/Serializable.h>

namespace Urho3D
{

enum ConverterKind
{
    /// Converter will not be executed.
    CONVERTER_DISABLED,
    /// converter will only run when explicitly invoked.
    CONVERTER_OFFLINE,
    /// Converter will run only when user is working in the editor.
    CONVERTER_ONLINE,
    /// Converter will run only when user is working in the editor.
    CONVERTER_ALWAYS,
};
URHO3D_FLAGSET(ConverterKind, ConverterKinds);

static const char* converterKindNames[] = {
    "disabled",
    "offline",
    "online",
    "always",
    nullptr
};

class Converter : public Serializable
{
    URHO3D_OBJECT(Converter, Serializable);
public:
    ///
    explicit Converter(Context* context);
    ///
    static void RegisterObject(Context* context);
    ///
    bool LoadJSON(const JSONValue& source) override;
    ///
    virtual void Execute(const StringVector& input);
    ///
    static StringHash GetSerializedType(const JSONValue& source);
    ///
    ConverterKind GetKind() const { return kind_; }

protected:
    ///
    ea::string comment_{};
    ///
    ConverterKind kind_ = CONVERTER_OFFLINE;
    ///
    ea::vector<SharedPtr<Converter>> converters_;
};

}
