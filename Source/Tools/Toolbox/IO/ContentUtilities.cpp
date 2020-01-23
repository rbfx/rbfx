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

#include <Urho3D/Audio/Sound.h>
#include <Urho3D/Core/Context.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/Animation.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Texture2D.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/XMLFile.h>
#include <Urho3D/SystemUI/SystemUI.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/Node.h>

#include <IconFontCppHeaders/IconsFontAwesome5.h>
#include "ContentUtilities.h"


namespace Urho3D
{

const ea::vector<ea::string> archiveExtensions_{".rar", ".zip", ".tar", ".gz", ".xz", ".7z", ".pak"};
const ea::vector<ea::string> wordExtensions_{".doc", ".docx", ".odt"};
const ea::vector<ea::string> codeExtensions_{".c", ".cpp", ".h", ".hpp", ".hxx", ".py", ".py3", ".js", ".cs"};
const ea::vector<ea::string> imagesExtensions_{".png", ".jpg", ".jpeg", ".gif", ".ttf", ".dds", ".psd"};
const ea::vector<ea::string> textExtensions_{".xml", ".json", ".txt", ".yml", ".scene", ".material", ".ui", ".uistyle", ".node", ".particle"};
const ea::vector<ea::string> audioExtensions_{".waw", ".ogg", ".mp3"};

FileType GetFileType(const ea::string& fileName)
{
    auto extension = GetExtension(fileName).to_lower();
    if (archiveExtensions_.contains(extension))
        return FTYPE_ARCHIVE;
    if (wordExtensions_.contains(extension))
        return FTYPE_WORD;
    if (codeExtensions_.contains(extension))
        return FTYPE_CODE;
    if (imagesExtensions_.contains(extension))
        return FTYPE_IMAGE;
    if (textExtensions_.contains(extension))
        return FTYPE_TEXT;
    if (audioExtensions_.contains(extension))
        return FTYPE_AUDIO;
    if (extension == "pdf")
        return FTYPE_PDF;
    return FTYPE_FILE;
}

ea::string GetFileIcon(const ea::string& fileName)
{
    switch (GetFileType(fileName))
    {
    case FTYPE_ARCHIVE:
        return ICON_FA_FILE_ARCHIVE;
    case FTYPE_WORD:
        return ICON_FA_FILE_WORD;
    case FTYPE_CODE:
        return ICON_FA_FILE_CODE;
    case FTYPE_IMAGE:
        return ICON_FA_FILE_IMAGE;
    case FTYPE_PDF:
        return ICON_FA_FILE_PDF;
    case FTYPE_VIDEO:
        return ICON_FA_FILE_VIDEO;
    case FTYPE_POWERPOINT:
        return ICON_FA_FILE_POWERPOINT;
    case FTYPE_TEXT:
        return ICON_FA_FILE_ALT;
    case FTYPE_FILM:
        return ICON_FA_FILE_VIDEO;
    case FTYPE_AUDIO:
        return ICON_FA_FILE_AUDIO;
    case FTYPE_EXCEL:
        return ICON_FA_FILE_EXCEL;
    default:
        return ICON_FA_FILE;
    }
}

ContentType GetContentType(Context* context, const ea::string& resourcePath)
{
    ResourceCache* cache = context->GetCache();
    FileSystem* fs = context->GetFileSystem();

    if (IsAbsolutePath(resourcePath))
    {
        if (fs->DirExists(resourcePath))
            return CTYPE_FOLDER;
    }
    else
    {
        for (const ea::string& resourceDir : cache->GetResourceDirs())
        {
            if (fs->DirExists(resourceDir + resourcePath))
                return CTYPE_FOLDER;
        }
    }

    auto extension = GetExtension(resourcePath).to_lower();
    if (extension == ".xml")
    {
        SharedPtr<XMLFile> xml;

        if (IsAbsolutePath(resourcePath))
        {
            xml = SharedPtr<XMLFile>(new XMLFile(context));
            if (!xml->LoadFile(resourcePath))
                return CTYPE_UNKNOWN;
        }
        else
            xml = context->GetCache()->GetResource<XMLFile>(resourcePath);

        if (!xml)
            return CTYPE_UNKNOWN;

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
    if (extension == ".scene")
        return CTYPE_SCENE;
    if (extension == ".ui")
        return CTYPE_UILAYOUT;
    if (extension == ".style")
        return CTYPE_UISTYLE;
    if (extension == ".material")
        return CTYPE_MATERIAL;
    if (extension == ".particle")
        return CTYPE_PARTICLE;
    if (extension == ".node")
        return CTYPE_SCENEOBJECT;
    if (audioExtensions_.contains(extension))
        return CTYPE_SOUND;
    if (imagesExtensions_.contains(extension))
        return CTYPE_TEXTURE;

    return CTYPE_UNKNOWN;
}

StringHash GetContentResourceType(Context* context, const ea::string& resourcePath)
{
    switch (GetContentType(context, resourcePath))
    {
    case CTYPE_SCENEOBJECT:
        return Node::GetTypeStatic();
    case CTYPE_UILAYOUT:
        return XMLFile::GetTypeStatic();
    case CTYPE_UISTYLE:
        return XMLFile::GetTypeStatic();
    case CTYPE_MODEL:
        return Model::GetTypeStatic();
    case CTYPE_ANIMATION:
        return Animation::GetTypeStatic();
    case CTYPE_MATERIAL:
        return Material::GetTypeStatic();
    case CTYPE_PARTICLE:
        return XMLFile::GetTypeStatic();
    case CTYPE_RENDERPATH:
        return XMLFile::GetTypeStatic();
    case CTYPE_SOUND:
        return Sound::GetTypeStatic();
    case CTYPE_TEXTURE:
        return Texture2D::GetTypeStatic();
    case CTYPE_TEXTUREXML:
        return XMLFile::GetTypeStatic();
    default:
        break;
    }
    return StringHash();
}

}
