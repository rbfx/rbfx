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

const char* ArchiveBase::keyElementName_ = "<Map key>";
const char* ArchiveBase::blockElementName_ = "<Block>";
const char* ArchiveBase::checkedBlockGuardElementName_ = "<Checked block guard>";

const ea::string ArchiveBase::fatalRootBlockNotOpened_elementName = "Fatal: Root block must be opened before serializing element '{0}'";
const ea::string ArchiveBase::fatalUnexpectedEndBlock = "Fatal: Unexpected call to EndBlock";
const ea::string ArchiveBase::fatalMissingElementName_blockName = "Fatal: Missing element name in Unordered block '{0}";
const ea::string ArchiveBase::fatalMissingKeySerialization_blockName = "Fatal: Missing key serialization in Map block '{0}'";
const ea::string ArchiveBase::fatalDuplicateKeySerialization_blockName = "Fatal: Duplicate key serialization in Map block '{0}'";
const ea::string ArchiveBase::fatalUnexpectedKeySerialization_blockName = "Fatal: Unexpected key serialization in non-Map block '{0}'";

const ea::string ArchiveBase::errorCannotReadData_blockName_elementName = "Cannot read data";
const ea::string ArchiveBase::errorReadEOF_blockName_elementName = "End of file before reading element or block '{1}' within block '{0}'";
const ea::string ArchiveBase::errorElementNotFound_blockName_elementName = "Element or block '{1}' is not found in block '{0}'";
const ea::string ArchiveBase::errorUnexpectedBlockType_blockName = "Block '{0}' has unexpected type";
const ea::string ArchiveBase::errorMissingMapKey_blockName = "Map key for block '{0}' is missing";

const ea::string ArchiveBase::errorCannotWriteData_blockName_elementName = "Cannot write data";
const ea::string ArchiveBase::errorWriteEOF_blockName_elementName = "End of file before writing element or block '{1}' within block '{0}'";
const ea::string ArchiveBase::errorDuplicateElement_blockName_elementName = "Duplicate element or block '{1}' in block '{0}'";
const ea::string ArchiveBase::fatalBlockOverflow_blockName = "Fatal: Array or Map block '{0}' overflow";
const ea::string ArchiveBase::fatalBlockUnderflow_blockName = "Fatal: Array or Map block '{0}' underflow";

}
