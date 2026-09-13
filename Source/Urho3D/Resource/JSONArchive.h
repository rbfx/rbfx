// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/IO/ArchiveBase.h"
#include "Urho3D/Resource/JSONFile.h"
#include "Urho3D/Resource/JSONValue.h"

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
