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

#include "../IO/Archive.h"

#include <cassert>

namespace Urho3D
{

ArchiveBlock::~ArchiveBlock()
{
    if (archive_)
    {
        const bool success = archive_->EndBlock();
        assert(success);
    }
}

bool Archive::ValidateName(ea::string_view name)
{
    // Empty names are allowed
    if (name.empty())
        return true;

    // Name must start with letter or underscore.
    if (!isalpha(name[0]) && name[0] != '_')
        return false;

    // Name must contain only letters, digits or underscores.
    for (const char ch : name)
    {
        if (!isalnum(ch) && ch != '_')
            return false;
    }

    // Name must not be reserved
    static const ea::string keywords[] = { "key" };
    for (ea::string_view keyword : keywords)
    {
        if (name == keyword)
            return false;
    }

    return true;
}

const char* ArchiveBase::versionElementName_ = "Version";
const char* ArchiveBase::keyElementName_ = "<Map key>";
const char* ArchiveBase::blockElementName_ = "<Block>";

const ea::string ArchiveBase::fatalRootBlockNotOpened_elementName = "Fatal: Root block must be opened before serializing element '{0}'";
const ea::string ArchiveBase::fatalUnexpectedEndBlock = "Fatal: Unexpected call to EndBlock";
const ea::string ArchiveBase::fatalMissingElementName = "Fatal: Missing element name in Unordered block";
const ea::string ArchiveBase::fatalMissingKeySerialization = "Fatal: Missing key serialization in Map block";
const ea::string ArchiveBase::fatalDuplicateKeySerialization = "Fatal: Duplicate key serialization in Map block";
const ea::string ArchiveBase::fatalUnexpectedKeySerialization = "Fatal: Unexpected key serialization in non-Map block";
const ea::string ArchiveBase::fatalInvalidName = "Fatal: Invalid element or block name '{0}'";
const ea::string ArchiveBase::errorEOF_elementName = "End of file before element or block '{0}'";
const ea::string ArchiveBase::errorUnspecifiedFailure_elementName = "Unspecified I/O failure before element or block '{0}'";

const ea::string ArchiveBase::errorElementNotFound_elementName = "Element or block '{0}' is not found";
const ea::string ArchiveBase::errorUnexpectedBlockType_blockName = "Block '{0}' has unexpected type";
const ea::string ArchiveBase::errorMissingMapKey = "Map key is missing";

const ea::string ArchiveBase::errorDuplicateElement_elementName = "Duplicate element or block '{0}'";
const ea::string ArchiveBase::fatalBlockOverflow = "Fatal: Array or Map block overflow";
const ea::string ArchiveBase::fatalBlockUnderflow = "Fatal: Array or Map block underflow";

}
