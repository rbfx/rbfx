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

#include "../IO/Archive.h"
#include "../Resource/JSONFile.h"
#include "../Resource/JSONValue.h"

namespace Urho3D
{

/// Return whether the block type should be serialized as JSON array.
inline bool IsArchiveBlockJSONArray(ArchiveBlockType type) { return type == ArchiveBlockType::Array || type == ArchiveBlockType::Sequential; }

/// Return whether the block type should be serialized as JSON object.
inline bool IsArchiveBlockJSONObject(ArchiveBlockType type) { return type == ArchiveBlockType::Map || type == ArchiveBlockType::Unordered; }

/// Return whether the block type matches JSONValue type.
inline bool IsArchiveBlockTypeMatching(const JSONValue& value, ArchiveBlockType type)
{
    return IsArchiveBlockJSONArray(type) && (value.IsArray() || value.IsNull())
        || IsArchiveBlockJSONObject(type) && (value.IsObject() || value.IsNull());
}

/// Base archive for JSON serialization.
template <class T, bool IsInputBool>
class JSONArchiveBase : public ArchiveBaseT<IsInputBool, true>
{
public:
    /// Get context.
    Context* GetContext() final { return context_; }
    /// Return name of the archive.
    ea::string_view GetName() const final { return jsonFile_ ? jsonFile_->GetName() : ""; }

    /// Whether the unordered element access is supported for Unordered blocks.
    bool IsUnorderedSupportedNow() const final { return !stack_.empty() && stack_.back().GetType() == ArchiveBlockType::Unordered; }

    /// Return current string stack.
    ea::string GetCurrentStackString() final
    {
        ea::string result;
        for (const Block& block : stack_)
        {
            if (!result.empty())
                result += "/";
            result += ea::string{ block.GetName() };
        }
        return result;
    }

protected:
    /// Construct from context and optional file.
    explicit JSONArchiveBase(Context* context, const JSONFile* jsonFile)
        : context_(context)
        , jsonFile_(jsonFile)
    {
    }

    /// Block type.
    using Block = T;

    /// Get current block.
    Block& GetCurrentBlock() { return stack_.back(); }

    /// Context.
    Context* context_{};
    /// Blocks stack.
    ea::vector<Block> stack_;

private:
    /// JSON file.
    const JSONFile* jsonFile_{};
};

/// JSON output archive block. Internal.
struct JSONOutputArchiveBlock
{
public:
    /// Construct.
    JSONOutputArchiveBlock(const char* name, ArchiveBlockType type, JSONValue* blockValue, unsigned sizeHint);
    /// Get block name.
    ea::string_view GetName() const { return name_; }
    /// Return block type.
    ArchiveBlockType GetType() const { return type_; }
    /// Set element key.
    bool SetElementKey(ArchiveBase& archive, ea::string key);
    /// Create element in the block.
    JSONValue* CreateElement(ArchiveBase& archive, const char* elementName);
    /// Close block.
    bool Close(ArchiveBase& archive);

private:
    /// Block name.
    ea::string_view name_{};
    /// Block type.
    ArchiveBlockType type_{};
    /// Block value.
    JSONValue* blockValue_;

    /// Expected block size (for arrays and maps).
    unsigned expectedElementCount_{ M_MAX_UNSIGNED };
    /// Number of elements in block.
    unsigned numElements_{};

    /// Key of the next created element (for Map blocks).
    ea::string elementKey_;
    /// Whether the key is set.
    bool keySet_{};
};

class URHO3D_API JSONOutputArchive : public JSONArchiveBase<JSONOutputArchiveBlock, false>
{
public:
    /// Base type.
    using Base = JSONArchiveBase<JSONOutputArchiveBlock, false>;

    /// Construct from element.
    JSONOutputArchive(Context* context, JSONValue& value, JSONFile* jsonFile = nullptr)
        : Base(context, jsonFile)
        , rootValue_(value)
    {
    }
    /// Construct from file.
    explicit JSONOutputArchive(JSONFile* jsonFile)
        : Base(jsonFile->GetContext(), jsonFile)
        , rootValue_(jsonFile->GetRoot())
    {}

    /// Begin archive block.
    bool BeginBlock(const char* name, unsigned& sizeHint, bool safe, ArchiveBlockType type) final;
    /// End archive block.
    bool EndBlock() final;

    /// Serialize string key. Used with Map block only.
    bool SerializeKey(ea::string& key) final;
    /// Serialize unsigned integer key. Used with Map block only.
    bool SerializeKey(unsigned& key) final;

    /// Serialize bool.
    bool Serialize(const char* name, bool& value) final;
    /// Serialize signed char.
    bool Serialize(const char* name, signed char& value) final;
    /// Serialize unsigned char.
    bool Serialize(const char* name, unsigned char& value) final;
    /// Serialize signed short.
    bool Serialize(const char* name, short& value) final;
    /// Serialize unsigned short.
    bool Serialize(const char* name, unsigned short& value) final;
    /// Serialize signed int.
    bool Serialize(const char* name, int& value) final;
    /// Serialize unsigned int.
    bool Serialize(const char* name, unsigned int& value) final;
    /// Serialize signed long.
    bool Serialize(const char* name, long long& value) final;
    /// Serialize unsigned long.
    bool Serialize(const char* name, unsigned long long& value) final;
    /// Serialize float.
    bool Serialize(const char* name, float& value) final;
    /// Serialize double.
    bool Serialize(const char* name, double& value) final;
    /// Serialize string.
    bool Serialize(const char* name, ea::string& value) final;

    /// Serialize bytes. Size is not encoded and should be provided externally!
    bool SerializeBytes(const char* name, void* bytes, unsigned size) final;
    /// Serialize Variable Length Encoded unsigned integer, up to 29 significant bits.
    bool SerializeVLE(const char* name, unsigned& value) final;

private:
    /// Check EOF.
    bool CheckEOF(const char* elementName, const char* debugName);
    /// Check EOF and root block.
    bool CheckEOFAndRoot(const char* elementName, const char* debugName);
    /// Serialize any JSON value.
    bool CreateElement(const char* name, const JSONValue& value);
    /// Temporary string.
    ea::string tempString_;

    /// Root value.
    JSONValue& rootValue_;
};

/// Archive stack frame helper.
struct JSONInputArchiveBlock
{
public:
    /// Construct valid.
    JSONInputArchiveBlock(const char* name, ArchiveBlockType type, const JSONValue* value);
    /// Return name.
    const ea::string_view GetName() const { return name_; }
    /// Return block type.
    ArchiveBlockType GetType() const { return type_; }
    /// Return size hint.
    unsigned GetSizeHint() const { return value_->Size(); }
    /// Return current child's key.
    bool ReadCurrentKey(ArchiveBase& archive, ea::string& key);
    /// Read current child and move to the next one.
    const JSONValue* ReadElement(ArchiveBase& archive, const char* elementName, const ArchiveBlockType* elementBlockType);

private:
    /// Debug block name.
    ea::string_view name_{};
    /// Frame type.
    ArchiveBlockType type_{};
    /// Frame base value.
    const JSONValue* value_{};
    /// Next array index (for array frames).
    unsigned nextElementIndex_{};
    /// Next map element iterator (for map frames).
    JSONObject::const_iterator nextMapElementIterator_{};
    /// Whether the key was read.
    bool keyRead_{};
};

class URHO3D_API JSONInputArchive : public JSONArchiveBase<JSONInputArchiveBlock, true>
{
public:
    /// Base type.
    using Base = JSONArchiveBase<JSONInputArchiveBlock, true>;

    /// Construct from element.
    JSONInputArchive(Context* context, const JSONValue& value, const JSONFile* jsonFile = nullptr)
        : Base(context, jsonFile)
        , rootValue_(value)
    {
    }
    /// Construct from file.
    explicit JSONInputArchive(const JSONFile* jsonFile)
        : Base(jsonFile->GetContext(), jsonFile)
        , rootValue_(jsonFile->GetRoot())
    {}

    /// Begin archive block.
    bool BeginBlock(const char* name, unsigned& sizeHint, bool safe, ArchiveBlockType type) final;
    /// End archive block.
    bool EndBlock() final;

    /// Serialize string key. Used with Map block only.
    bool SerializeKey(ea::string& key) final;
    /// Serialize unsigned integer key. Used with Map block only.
    bool SerializeKey(unsigned& key) final;

    /// Serialize bool.
    bool Serialize(const char* name, bool& value) final;
    /// Serialize signed char.
    bool Serialize(const char* name, signed char& value) final;
    /// Serialize unsigned char.
    bool Serialize(const char* name, unsigned char& value) final;
    /// Serialize signed short.
    bool Serialize(const char* name, short& value) final;
    /// Serialize unsigned short.
    bool Serialize(const char* name, unsigned short& value) final;
    /// Serialize signed int.
    bool Serialize(const char* name, int& value) final;
    /// Serialize unsigned int.
    bool Serialize(const char* name, unsigned int& value) final;
    /// Serialize signed long.
    bool Serialize(const char* name, long long& value) final;
    /// Serialize unsigned long.
    bool Serialize(const char* name, unsigned long long& value) final;
    /// Serialize float.
    bool Serialize(const char* name, float& value) final;
    /// Serialize double.
    bool Serialize(const char* name, double& value) final;
    /// Serialize string.
    bool Serialize(const char* name, ea::string& value) final;

    /// Serialize bytes. Size is not encoded and should be provided externally!
    bool SerializeBytes(const char* name, void* bytes, unsigned size) final;
    /// Serialize Variable Length Encoded unsigned integer, up to 29 significant bits.
    bool SerializeVLE(const char* name, unsigned& value) final;

private:
    /// Check EOF.
    bool CheckEOF(const char* elementName, const char* debugName);
    /// Check EOF and root block.
    bool CheckEOFAndRoot(const char* elementName, const char* debugName);
    /// Deserialize JSONValue.
    const JSONValue* ReadElement(const char* name);
    /// Temporary buffer.
    ea::vector<unsigned char> tempBuffer_;
    /// Root value.
    const JSONValue& rootValue_;
};

}
