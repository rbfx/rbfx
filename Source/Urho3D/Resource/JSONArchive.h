//
// Copyright (c) 2017-2020 the rbfx project.
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

#include "../IO/ArchiveBase.h"
#include "../Resource/JSONFile.h"
#include "../Resource/JSONValue.h"

#include <EASTL/optional.h>

namespace Urho3D
{

/// Base archive for JSON serialization.
template <class BlockType, bool IsInputBool>
class JSONArchiveBase : public ArchiveBaseT<BlockType, IsInputBool, true>
{
public:
    /// @name Archive implementation
    /// @{
    ea::string_view GetName() const { return jsonFile_ ? jsonFile_->GetName() : ""; }
    /// @}

protected:
    explicit JSONArchiveBase(Context* context, const JSONFile* jsonFile)
        : ArchiveBaseT<BlockType, IsInputBool, true>(context)
        , jsonFile_(jsonFile)
    {
    }

private:
    const JSONFile* jsonFile_{};
};

/// JSON output archive block.
class URHO3D_API JSONOutputArchiveBlock : public ArchiveBlockBase
{
public:
    JSONOutputArchiveBlock(const char* name, ArchiveBlockType type, JSONValue* blockValue, unsigned sizeHint);
    JSONValue& CreateElement(ArchiveBase& archive, const char* elementName);

    bool IsUnorderedAccessSupported() const { return type_ == ArchiveBlockType::Unordered; }
    bool HasElementOrBlock(const char* name) const { return false; }
    void Close(ArchiveBase& archive);

private:
    /// Block value.
    JSONValue* blockValue_;

    /// Expected block size (for arrays).
    unsigned expectedElementCount_{ M_MAX_UNSIGNED };
    /// Number of elements in block.
    unsigned numElements_{};
};

/// JSON output archive.
class URHO3D_API JSONOutputArchive : public JSONArchiveBase<JSONOutputArchiveBlock, false>
{
public:
    /// Construct from element.
    JSONOutputArchive(Context* context, JSONValue& value, JSONFile* jsonFile = nullptr)
        : JSONArchiveBase(context, jsonFile)
        , rootValue_(value)
    {
    }
    /// Construct from file.
    explicit JSONOutputArchive(JSONFile* jsonFile)
        : JSONArchiveBase(jsonFile->GetContext(), jsonFile)
        , rootValue_(jsonFile->GetRoot())
    {}

    /// @name Archive implementation
    /// @{
    void BeginBlock(const char* name, unsigned& sizeHint, bool safe, ArchiveBlockType type) final;

    void Serialize(const char* name, bool& value) final;
    void Serialize(const char* name, signed char& value) final;
    void Serialize(const char* name, unsigned char& value) final;
    void Serialize(const char* name, short& value) final;
    void Serialize(const char* name, unsigned short& value) final;
    void Serialize(const char* name, int& value) final;
    void Serialize(const char* name, unsigned int& value) final;
    void Serialize(const char* name, long long& value) final;
    void Serialize(const char* name, unsigned long long& value) final;
    void Serialize(const char* name, float& value) final;
    void Serialize(const char* name, double& value) final;
    void Serialize(const char* name, ea::string& value) final;

    void SerializeBytes(const char* name, void* bytes, unsigned size) final;
    void SerializeVLE(const char* name, unsigned& value) final;
    /// @}

private:
    void CreateElement(const char* name, const JSONValue& value);

    JSONValue& rootValue_;
    ea::string tempString_;
};

/// JSON input archive block.
class URHO3D_API JSONInputArchiveBlock : public ArchiveBlockBase
{
public:
    JSONInputArchiveBlock(const char* name, ArchiveBlockType type, const JSONValue* value);
    /// Return size hint.
    unsigned GetSizeHint() const { return value_->Size(); }
    /// Read current child and move to the next one.
    const JSONValue& ReadElement(ArchiveBase& archive, const char* elementName, const ArchiveBlockType* elementBlockType);

    bool IsUnorderedAccessSupported() const { return type_ == ArchiveBlockType::Unordered; }
    bool HasElementOrBlock(const char* name) const { return value_ && value_->GetObject().contains(name); }
    void Close(ArchiveBase& archive) {}

private:
    const JSONValue* value_{};

    /// Next array index (for sequential and array blocks).
    unsigned nextElementIndex_{};
};

/// JSON input archive.
class URHO3D_API JSONInputArchive : public JSONArchiveBase<JSONInputArchiveBlock, true>
{
public:
    /// Construct from element.
    JSONInputArchive(Context* context, const JSONValue& value, const JSONFile* jsonFile = nullptr)
        : JSONArchiveBase(context, jsonFile)
        , rootValue_(value)
    {
    }
    /// Construct from file.
    explicit JSONInputArchive(const JSONFile* jsonFile)
        : JSONArchiveBase(jsonFile->GetContext(), jsonFile)
        , rootValue_(jsonFile->GetRoot())
    {}

    /// @name Archive implementation
    /// @{
    void BeginBlock(const char* name, unsigned& sizeHint, bool safe, ArchiveBlockType type) final;

    void Serialize(const char* name, bool& value) final;
    void Serialize(const char* name, signed char& value) final;
    void Serialize(const char* name, unsigned char& value) final;
    void Serialize(const char* name, short& value) final;
    void Serialize(const char* name, unsigned short& value) final;
    void Serialize(const char* name, int& value) final;
    void Serialize(const char* name, unsigned int& value) final;
    void Serialize(const char* name, long long& value) final;
    void Serialize(const char* name, unsigned long long& value) final;
    void Serialize(const char* name, float& value) final;
    void Serialize(const char* name, double& value) final;
    void Serialize(const char* name, ea::string& value) final;

    void SerializeBytes(const char* name, void* bytes, unsigned size) final;
    void SerializeVLE(const char* name, unsigned& value) final;
    /// @}

private:
    const JSONValue& ReadElement(const char* name);
    void CheckType(const char* name, const JSONValue& value, JSONValueType type) const;

    const JSONValue& rootValue_;
};

/// Save object to JSON string.
template <class T> ea::optional<ea::string> ToJSONString(T& object)
{
    ea::optional<ea::string> result;
    ConsumeArchiveException(
        [&]
    {
        JSONFile jsonFile{Context::GetInstance()};
        JSONOutputArchive archive(&jsonFile);
        SerializeValue(archive, "object", object);
        result = jsonFile.ToString();
    });
    return result;
}

/// Load object from JSON string.
template <class T> ea::optional<T> FromJSONString(const ea::string& jsonString)
{
    ea::optional<T> result;
    ConsumeArchiveException(
        [&]
    {
        JSONFile jsonFile{Context::GetInstance()};
        if (!jsonFile.FromString(jsonString))
            throw ArchiveException("Failed to parse JSON string");

        JSONInputArchive archive(&jsonFile);
        T resultObject;
        SerializeValue(archive, "object", resultObject);
        result = ea::move(resultObject);
    });
    return result;
}

} // namespace Urho3D
