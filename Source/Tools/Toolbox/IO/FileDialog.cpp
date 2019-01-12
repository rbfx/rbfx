//
// Copyright (c) 2017-2019 Rokas Kupstys.
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

#include <nativefiledialog/nfd.h>
#include "FileDialog.h"


namespace Urho3D
{

FileDialogResult OpenDialog(const String& filterList, const String& defaultPath, String& outPath)
{
    nfdchar_t* output = nullptr;
    auto result = NFD_OpenDialog(filterList.CString(), defaultPath.CString(), &output);
    if (output != nullptr)
    {
        outPath = output;
        NFD_FreePath(output);
    }
    return static_cast<FileDialogResult>(result);
}

FileDialogResult OpenDialogMultiple(const String& filterList, const String& defaultPath, Vector<String>& outPaths)
{
    nfdpathset_t output{};
    auto result = NFD_OpenDialogMultiple(filterList.CString(), defaultPath.CString(), &output);
    if (result == NFD_OKAY)
    {
        for (size_t i = 0, end = NFD_PathSet_GetCount(&output); i < end; i++)
            outPaths.Push(NFD_PathSet_GetPath(&output, i));
        NFD_PathSet_Free(&output);
    }
    return static_cast<FileDialogResult>(result);
}

FileDialogResult SaveDialog(const String& filterList, const String& defaultPath, String& outPath)
{
    nfdchar_t* output = nullptr;
    auto result = NFD_SaveDialog(filterList.CString(), defaultPath.CString(), &output);
    if (output != nullptr)
    {
        outPath = output;
        NFD_FreePath(output);
    }
    return static_cast<FileDialogResult>(result);
}

FileDialogResult PickFolder(const String& defaultPath, String& outPath)
{
    nfdchar_t* output = nullptr;
    auto result = NFD_PickFolder(defaultPath.CString(), &output);
    if (output != nullptr)
    {
        outPath = output;
        NFD_FreePath(output);
    }
    return static_cast<FileDialogResult>(result);
}

}
