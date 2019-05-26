//
// Copyright (c) 2008-2019 the Urho3D project.
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

// #include <EASTL/optional.h>
// #include <EASTL/hash_set.h>

namespace Urho3D
{

/// Return whether the block type should be serialized as JSON array.
inline bool IsArchiveBlockJSONArray(ArchiveBlockType type) { return type == ArchiveBlockType::Array || type == ArchiveBlockType::Sequential; }

/// Return whether the block type should be serialized as JSON object.
inline bool IsArchiveBlockJSONObject(ArchiveBlockType type) { return type == ArchiveBlockType::Map || type == ArchiveBlockType::Unordered; }

/// Base archive for JSON serialization.
template <class T>
class JSONArchiveBase : public ArchiveBase
{
public:
    /// Construct.
    explicit JSONArchiveBase(SharedPtr<JSONFile> jsonFile, bool preferStrings = true)
        : jsonFile_(jsonFile)
        , preferStrings_(preferStrings)
    {}

    /// Get context.
    Context* GetContext() final { return jsonFile_->GetContext(); }
    /// Whether the archive is binary.
    bool IsBinary() const final { return false; }
    /// Whether the Unordered blocks are supported.
    bool IsUnorderedSupported() const final { return true; }

    /// Serialize string key. Used with Map block only.
    void SetStringKey(ea::string* key) final { stringKey_ = key; }
    /// Serialize unsigned integer key. Used with Map block only.
    void SetUnsignedKey(unsigned* key) final { uintKey_ = key; }

protected:
    /// Block type.
    using Block = T;

    /// Get current block.
    Block& GetCurrentBlock() { return stack_.back(); }
    /// Reset keys.
    void ResetKeys()
    {
        stringKey_ = nullptr;
        uintKey_ = nullptr;
    }

    /// JSON file.
    SharedPtr<JSONFile> jsonFile_;
    /// Whether to prefer string format.
    bool preferStrings_{};
    /// Blocks stack.
    ea::vector<Block> stack_;

    /// String key.
    ea::string* stringKey_{};
    /// UInt key.
    unsigned* uintKey_{};
};

/// JSON output archive block. Internal.
struct JSONOutputArchiveBlock
{
public:
    /// Construct.
    JSONOutputArchiveBlock(ArchiveBlockType type, const ea::string& key, unsigned expectedElementCount);
    /// Return whether the child with given name and key may be safely added.
    bool CanCreateChild(const char* name, const ea::string* stringKey, const unsigned* uintKey) const;
    /// Get child key string.
    ea::string GetChildKey(const char* name, const ea::string* stringKey, const unsigned* uintKey);
    /// Create child. Checks should be performed first!
    void CreateChild(const ea::string& key, JSONValue childValue);
    /// Is block complete and can be closed?
    bool IsComplete() const { return expectedElementCount_ == M_MAX_UNSIGNED || numElements_ == expectedElementCount_; }
    /// Get key.
    const ea::string& GetKey() const { return key_; }
    /// Get value (mutable, so it can be moved away).
    JSONValue& GetValue() { return blockValue_; }

private:
    /// Block type.
    ArchiveBlockType type_{};
    /// Block key (when parent block is JSON object).
    ea::string key_;
    /// Expected block size (for arrays and maps).
    unsigned expectedElementCount_{ M_MAX_UNSIGNED };

    /// Block value.
    JSONValue blockValue_;
    /// Number of elements in block.
    unsigned numElements_{};
};

class URHO3D_API JSONOutputArchive : public JSONArchiveBase<JSONOutputArchiveBlock>
{
public:
    using JSONArchiveBase<JSONOutputArchiveBlock>::JSONArchiveBase;

    /// Whether the archive is in input mode.
    bool IsInput() const final { return false; }

    /// Begin archive block.
    bool BeginBlock(const char* name, unsigned& sizeHint, ArchiveBlockType type) final;
    /// End archive block.
    bool EndBlock() final;

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

    /// Serialize float array. Size is not encoded and should be provided externally!
    bool SerializeFloatArray(const char* name, float* values, unsigned size) final;
    /// Serialize integer array. Size is not encoded and should be provided externally!
    bool SerializeIntArray(const char* name, int* values, unsigned size) final;
    /// Serialize bytes. Size is not encoded and should be provided externally!
    bool SerializeBytes(const char* name, void* bytes, unsigned size) final;
    /// Serialize Variable Length Encoded unsigned integer, up to 29 significant bits.
    bool SerializeVLE(const char* name, unsigned& value) final;

private:
    /// Serialize any JSON value.
    bool SerializeJSONValue(const char* name, const JSONValue& value);
    /// Temporary string.
    ea::string tempString_;
};

/// Archive stack frame helper.
struct JSONInputArchiveBlock
{
public:
    /// Construct valid.
    JSONInputArchiveBlock(ArchiveBlockType type, const JSONValue* value);
    /// Return whether the frame valid.
    bool IsValid() const;
    /// Return size hint.
    unsigned GetSizeHint() const { return value_->Size(); }
    /// Return current child. Name may be used for ArchiveBlockType::Unordered blocks. Key is filled for ArchiveBlockType::Map frames.
    const JSONValue* GetCurrentElement(const char* name, ea::string* stringKey, unsigned* uintKey);
    /// Move stack frame to next element.
    void NextElement();

private:
    /// Frame type.
    ArchiveBlockType type_{};
    /// Frame base value.
    const JSONValue* value_{};
    /// Next array index (for array frames).
    unsigned nextElementIndex_{};
    /// Next map element iterator (for map frames).
    JSONObject::const_iterator nextMapElementIterator_{};
};

class URHO3D_API JSONInputArchive : public JSONArchiveBase<JSONInputArchiveBlock>
{
public:
    using JSONArchiveBase<JSONInputArchiveBlock>::JSONArchiveBase;

    /// Whether the archive is in input mode.
    bool IsInput() const final { return true; }

    /// Begin archive block.
    bool BeginBlock(const char* name, unsigned& sizeHint, ArchiveBlockType type) final;
    /// End archive block.
    bool EndBlock() final;

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

    /// Serialize float array. Size is not encoded and should be provided externally!
    bool SerializeFloatArray(const char* name, float* values, unsigned size) final;
    /// Serialize integer array. Size is not encoded and should be provided externally!
    bool SerializeIntArray(const char* name, int* values, unsigned size) final;
    /// Serialize bytes. Size is not encoded and should be provided externally!
    bool SerializeBytes(const char* name, void* bytes, unsigned size) final;
    /// Serialize Variable Length Encoded unsigned integer, up to 29 significant bits.
    bool SerializeVLE(const char* name, unsigned& value) final;

private:
    /// Deserialize JSONValue.
    const JSONValue* DeserializeJSONValue(const char* name);
    /// Temporary buffer.
    ea::vector<unsigned char> tempBuffer_;
};

}
