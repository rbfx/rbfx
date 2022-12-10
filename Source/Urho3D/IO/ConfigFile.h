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

#include "../Scene/Serializable.h"
#include "../IO/AbstractFile.h"
#include "../Core/Variant.h"

namespace Urho3D
{
class XMLFile;
class JSONFile;

class URHO3D_API ConfigFileBase : public Serializable
{
    URHO3D_OBJECT(ConfigFileBase, Serializable)
public:
    /// Construct.
    explicit ConfigFileBase(Context* context);
    /// Destruct.
    ~ConfigFileBase() override;

    /// Config file serialization.
    virtual void SerializeInBlock(Archive& archive);

    /// Load all config files and merge the result. Return true if successful.
    virtual bool MergeFile(const ea::string& fileName);
    /// Load from file at app preferences location. Return true if successful.
    bool LoadFile(const ea::string& fileName) override;
    /// Save to file at app preferences location. Return true if successful.
    virtual bool SaveFile(const ea::string& fileName);

    /// Load from binary data. Return true if successful.
    bool Load(Deserializer& source) override;
    /// Save as binary data. Return true if successful.
    bool Save(Serializer& dest) const override;
    /// Load from XML data. Return true if successful.
    bool LoadXML(const XMLElement& source) override;
    /// Save as XML data. Return true if successful.
    bool SaveXML(XMLElement& dest) const override;
    /// Load from JSON data. Return true if successful.
    bool LoadJSON(const JSONValue& source) override;
    /// Save as JSON data. Return true if successful.
    bool SaveJSON(JSONValue& dest) const override;
    /// Load from binary resource.
    bool Load(const ea::string& resourceName) override;
    /// Load from XML resource.
    bool LoadXML(const ea::string& resourceName) override;
    /// Load from JSON resource.
    bool LoadJSON(const ea::string& resourceName) override;
protected:
    /// Load from file. Return true if successful.
    bool LoadImpl(Deserializer& source);
};

class URHO3D_API ConfigFile : public ConfigFileBase
{
    URHO3D_OBJECT(ConfigFile, ConfigFileBase)
public:
    /// Construct.
    explicit ConfigFile(Context* context);
    /// Destruct.
    ~ConfigFile() override;

    /// Reset values to default.
    void Clear();
    /// Config file serialization.
    void SerializeInBlock(Archive& archive) override;
    /// Save difference between current config and resources into file.
    bool SaveDiffFile(const ea::string& fileName);

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
