//
// Copyright (c) 2008-2017 the Urho3D project.
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

#include <IconFontCppHeaders/IconsFontAwesome.h>
#include <Urho3D/Urho3DAll.h>
#include "ContentUtilities.h"


namespace Urho3D
{

const Vector<String> archiveExtensions_{".rar", ".zip", ".tar", ".gz", ".xz", ".7z", ".pak"};
const Vector<String> wordExtensions_{".doc", ".docx", ".odt"};
const Vector<String> codeExtensions_{".c", ".cpp", ".h", ".hpp", ".hxx", ".py", ".py3", ".js", ".cs"};
const Vector<String> imagesExtensions_{".png", ".jpg", ".jpeg", ".gif", ".ttf", ".dds", ".psd"};
const Vector<String> textExtensions_{".xml", ".json", ".txt"};
const Vector<String> audioExtensions_{".waw", ".ogg", ".mp3"};

FileType GetFileType(const String& fileName)
{
    auto extension = GetExtension(fileName).ToLower();
    if (archiveExtensions_.Contains(extension))
        return FTYPE_ARCHIVE;
    if (wordExtensions_.Contains(extension))
        return FTYPE_WORD;
    if (codeExtensions_.Contains(extension))
        return FTYPE_CODE;
    if (imagesExtensions_.Contains(extension))
        return FTYPE_IMAGE;
    if (textExtensions_.Contains(extension))
        return FTYPE_TEXT;
    if (audioExtensions_.Contains(extension))
        return FTYPE_AUDIO;
    if (extension == "pdf")
        return FTYPE_PDF;
    return FTYPE_FILE;
}

String GetFileIcon(const String& fileName)
{
    switch (GetFileType(fileName))
    {
    case FTYPE_ARCHIVE:
        return ICON_FA_FILE_ARCHIVE_O;
    case FTYPE_WORD:
        return ICON_FA_FILE_WORD_O;
    case FTYPE_CODE:
        return ICON_FA_FILE_CODE_O;
    case FTYPE_IMAGE:
        return ICON_FA_FILE_IMAGE_O;
    case FTYPE_PDF:
        return ICON_FA_FILE_PDF_O;
    case FTYPE_VIDEO:
        return ICON_FA_FILE_VIDEO_O;
    case FTYPE_POWERPOINT:
        return ICON_FA_FILE_POWERPOINT_O;
    case FTYPE_TEXT:
        return ICON_FA_FILE_TEXT_O;
    case FTYPE_FILM:
        return ICON_FA_FILE_VIDEO_O;
    case FTYPE_AUDIO:
        return ICON_FA_FILE_AUDIO_O;
    case FTYPE_EXCEL:
        return ICON_FA_FILE_EXCEL_O;
    default:
        return ICON_FA_FILE;
    }
}

ContentType GetContentType(const String& resourcePath)
{
    auto extension = GetExtension(resourcePath).ToLower();
    if (extension == ".xml")
    {
        SharedPtr<XMLFile> xml(Context::GetContext()->GetCache()->GetResource<XMLFile>(resourcePath));
        auto rootElementName = xml->GetRoot().GetName();
        if (rootElementName == "scene")
            return CTYPE_SCENE;
        if (rootElementName == "node")
            return CTYPE_SCENEOBJECT;
        if (rootElementName == "elements")
            return CTYPE_UISTYLE;
        if (rootElementName == "element")
            return CTYPE_UILAYOUT;
        if (rootElementName == "material")
            return CTYPE_MATERIAL;
        if (rootElementName == "particleeffect")
            return CTYPE_PARTICLE;
        if (rootElementName == "renderpath")
            return CTYPE_RENDERPATH;
        if (rootElementName == "texture")
            return CTYPE_TEXTUREXML;
    }

    if (extension == ".mdl")
        return CTYPE_MODEL;
    if (extension == ".ani")
        return CTYPE_ANIMATION;
    if (audioExtensions_.Contains(extension))
        return CTYPE_SOUND;
    if (imagesExtensions_.Contains(extension))
        return CTYPE_TEXTURE;

    return CTYPE_UNKNOWN;
}

}
