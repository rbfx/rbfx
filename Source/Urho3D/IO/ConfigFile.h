//
// Copyright (c) 2022-2022 the Urho3D project.
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

#include "../IO/AbstractFile.h"
#include "../Core/Variant.h"

namespace Urho3D
{
class XMLFile;
class JSONFile;

class URHO3D_API ConfigFileBase
{
public:
    virtual ~ConfigFileBase() = default;

    /// Config file serialization.
    virtual void SerializeInBlock(Archive& archive);

    /// Load all config files and merge the result. Return true if successful.
    bool Merge(Context* context, const ea::string& fileName);
    /// Load from file at app preferences location. Return true if successful.
    bool Load(Context* context, const ea::string& fileName);
    /// Save to file at app preferences location. Return true if successful.
    bool Save(Context* context, const ea::string& fileName);

    /// Load from file. Return true if successful.
    bool LoadFile(Context* context, const AbstractFilePtr& file);
    /// Save to file. Return true if successful.
    bool SaveFile(Context* context, const AbstractFilePtr& file);
    /// Load from XML file. Return true if successful.
    bool LoadXML(const XMLFile* xmlFile);
    /// Save to XML file. Return true if successful.
    bool SaveXML(XMLFile* xmlFile);
    /// Load from JSON file. Return true if successful.
    bool LoadJSON(const JSONFile* jsonFile);
    /// Save to JSON file. Return true if successful.
    bool SaveJSON(JSONFile* jsonFile);
};

class URHO3D_API ConfigFile : public ConfigFileBase
{
public:
    /// Reset values to default.
    void Clear();
    /// Config file serialization.
    void SerializeInBlock(Archive& archive) override;

    /// Set default value.
    void SetDefaultValue(const ea::string key, const Variant& vault);
    /// Set value.
    bool SetValue(const ea::string& name, const Variant& value);
    /// Get value.
    const Variant& GetValue(const ea::string& name) const;
    /// Get all defined values.
    const StringVariantMap& GetValues() const { return values_; }

private:
    // Active values.
    StringVariantMap values_;
    // Default value.
    StringVariantMap default_;
};

}
