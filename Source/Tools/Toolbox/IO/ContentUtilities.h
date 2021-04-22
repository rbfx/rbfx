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


#include "ToolboxAPI.h"
#include <Urho3D/Container/Str.h>
#include <Urho3D/Precompiled.h>

#include <EASTL/fixed_vector.h>


namespace Urho3D
{

enum FileType : unsigned
{
    FTYPE_FILE,
    FTYPE_ARCHIVE,
    FTYPE_WORD,
    FTYPE_CODE,
    FTYPE_IMAGE,
    FTYPE_PDF,
    FTYPE_VIDEO,
    FTYPE_POWERPOINT,
    FTYPE_TEXT,
    FTYPE_FILM,
    FTYPE_AUDIO,
    FTYPE_EXCEL,
    FTYPE_FONT,
};

enum ContentType : unsigned
{
    CTYPE_BINARY,
    CTYPE_SCENE,
    CTYPE_SCENEOBJECT,
    CTYPE_UILAYOUT,
    CTYPE_UISTYLE,
    CTYPE_MODEL,
    CTYPE_ANIMATION,
    CTYPE_MATERIAL,
    CTYPE_PARTICLE,
    CTYPE_RENDERPATH,
    CTYPE_SOUND,
    CTYPE_TEXTURE,
    CTYPE_TEXTURECUBE,
    CTYPE_TEXTUREXML,
    CTYPE_FOLDER,
    CTYPE_FONT,
};

using ResourceContentTypes = ea::fixed_vector<StringHash, 2>;

/// Return file type based on extension of file name.
URHO3D_TOOLBOX_API FileType GetFileType(const ea::string& fileName);
/// Return icon from icon font based on extension of file name.
URHO3D_TOOLBOX_API ea::string GetFileIcon(const ea::string& fileName);

/// Return content type by inspecting file contents.
URHO3D_TOOLBOX_API ContentType GetContentType(Context* context, const ea::string& resourcePath);
/// Return resource type by inspecting file contents.
URHO3D_TOOLBOX_API bool GetContentResourceType(Context* context, const ea::string& resourcePath, ResourceContentTypes& types);

}
