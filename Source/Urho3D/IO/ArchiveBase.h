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

#include "../Core/Assert.h"
#include "../Core/Context.h"
#include "../Core/NonCopyable.h"
#include "../IO/Archive.h"

namespace Urho3D
{

/// Archive implementation helper. Provides default Archive implementation for most cases.
class URHO3D_API ArchiveBase
    : public Archive
    , public MovableNonCopyable
{
public:
    /// @name Archive implementation
    /// @{
    Context* GetContext() final { return context_; }

    ea::string_view GetName() const override { return {}; }

    unsigned GetChecksum() override { return 0; }

    bool IsEOF() const final { return eof_; }

    void Flush() final { FlushDelayedException(); }

    unsigned SerializeVersion(unsigned version) final
    {
        SerializeVLE(versionElementName, version);
        return version;
    }
    /// @}

    /// @name Common exception factories
    /// @{
    ArchiveException IOFailureException(ea::string_view elementName) const
    {
        return ArchiveException("Unspecified I/O failure before '{}/{}'", GetCurrentBlockPath(), elementName);
    }

    ArchiveException DuplicateElementException(ea::string_view elementName) const
    {
        return ArchiveException("'{}/{}' is serialized several times", GetCurrentBlockPath(), elementName);
    }

    ArchiveException ElementNotFoundException(ea::string_view elementName) const
    {
        return ArchiveException("'{}/{}' is not found", GetCurrentBlockPath(), elementName);
    }

    ArchiveException ElementNotFoundException(ea::string_view elementName, unsigned elementIndex) const
    {
        return ArchiveException("'{}/{}#{}' is not found", GetCurrentBlockPath(), elementName, elementIndex);
    }

    ArchiveException UnexpectedElementValueException(ea::string_view elementName) const
    {
        return ArchiveException("'{}/{}' has unexpected type", GetCurrentBlockPath(), elementName);
    }

    ArchiveException UnexpectedEOFException(ea::string_view elementName) const
    {
        return ArchiveException("Unexpected end of file before '{}/{}'", GetCurrentBlockPath(), elementName);
    }
    /// @}

protected:
    ArchiveBase(Context* context)
        : context_(context)
    {
    }

    ~ArchiveBase() override
    {
        URHO3D_ASSERT(!delayedException_, "Archive::Flush was not called while having delayed exception");
    }

    static constexpr const char* rootBlockName = "Root";
    static constexpr const char* versionElementName = "Version";

    void SetDelayedException(std::exception_ptr ptr)
    {
        if (!delayedException_)
            delayedException_ = ptr;
    }

    void FlushDelayedException()
    {
        if (delayedException_)
        {
            std::exception_ptr ptr = delayedException_;
            delayedException_ = nullptr;
            std::rethrow_exception(ptr);
        }
    }

    void CheckIfNotEOF(ea::string_view elementName) const
    {
        if (eof_)
            throw UnexpectedEOFException(elementName);
    }

    void CheckBlockOrElementName(ea::string_view elementName) const
    {
        URHO3D_ASSERT(ValidateName(elementName));
    }

    void CloseArchive() { eof_ = true; }

    void ReadBytesFromHexString(ea::string_view elementName, const ea::string& string, void* bytes, unsigned size);

private:
    Context* context_{};
    std::exception_ptr delayedException_;
    bool eof_{};
};

/// Base implementation of ArchiveBlock. May contain inline blocks.
class ArchiveBlockBase
{
public:
    ArchiveBlockBase(const char* name, ArchiveBlockType type)
        : name_(name)
        , type_(type)
    {
    }

    ea::string_view GetName() const { return name_; }
    ArchiveBlockType GetType() const { return type_; }

    /// @name Manage inline blocks
    /// @{
    void OpenInlineBlock() { ++inlineBlockDepth_; }
    void CloseInlineBlock()
    {
        URHO3D_ASSERT(inlineBlockDepth_ > 0);
        --inlineBlockDepth_;
    }
    bool HasOpenInlineBlock() const { return inlineBlockDepth_; }
    /// @}

    /// @name To be implemented
    /// @{
    bool IsUnorderedAccessSupported() const = delete;
    bool HasElementOrBlock(const char* name) const = delete;
    void Close() = delete;
    /// @}

protected:
    const ea::string_view name_;
    const ArchiveBlockType type_{};

    unsigned inlineBlockDepth_{};
};

/// Archive implementation helper (template). Provides default block-dependent Archive implementation for most cases.
template <class BlockType, bool IsInputBool, bool IsHumanReadableBool>
class ArchiveBaseT : public ArchiveBase
{
    friend BlockType;

public:
    /// @name Archive implementation
    /// @{
    bool IsInput() const final { return IsInputBool; }

    bool IsHumanReadable() const final { return IsHumanReadableBool; }

    bool IsUnorderedAccessSupportedInCurrentBlock() const final
    {
        return !stack_.empty() && GetCurrentBlock().IsUnorderedAccessSupported();
    }

    bool HasElementOrBlock(const char* name) const final
    {
        if constexpr (IsInputBool)
        {
            CheckIfRootBlockOpen();
            return GetCurrentBlock().HasElementOrBlock(name);
        }
        else
        {
            URHO3D_ASSERT(0, "Archive::HasElementOrBlock is not supported for Output Archive");
            return false;
        }
    }

    ea::string GetCurrentBlockPath() const final
    {
        ea::string result;
        for (const Block& block : stack_)
        {
            if (!result.empty())
                result += "/";
            const auto& blockName = block.GetName();
            result.append(blockName.begin(), blockName.end());
            if (block.HasOpenInlineBlock())
                result += "/?";
        }
        return result;
    }

    void EndBlock() noexcept override
    {
        URHO3D_ASSERT(!stack_.empty());

        // Close inline block if possible
        Block& currentBlock = GetCurrentBlock();
        if (currentBlock.HasOpenInlineBlock())
        {
            currentBlock.CloseInlineBlock();
            return;
        }

        // Close block normally
        try
        {
            currentBlock.Close(*this);
        }
        catch (const ArchiveException&)
        {
            SetDelayedException(std::current_exception());
        }

        stack_.pop_back();
        if (stack_.empty())
            CloseArchive();
    }
    /// @}

protected:
    using ArchiveBase::ArchiveBase;

    using Block = BlockType;

    Block& GetCurrentBlock() { return stack_.back(); }

    const Block& GetCurrentBlock() const { return stack_.back(); }

    void CheckIfRootBlockOpen() const { URHO3D_ASSERT(!stack_.empty(), "Root block must be opened before serialization"); }

    void CheckBeforeBlock(const char* elementName)
    {
        FlushDelayedException();
        CheckIfNotEOF(elementName);
    }

    void CheckBeforeElement(const char* elementName)
    {
        FlushDelayedException();
        CheckIfNotEOF(elementName);
        CheckIfRootBlockOpen();
    }

    ea::vector<Block> stack_;
};

}
