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

#include <nativefiledialog/nfd.h>
#include "FileDialog.h"


namespace Urho3D
{

FileDialogResult OpenDialog(const ea::string& filterList, const ea::string& defaultPath, ea::string& outPath)
{
    nfdchar_t* output = nullptr;
    auto result = NFD_OpenDialog(filterList.c_str(), defaultPath.c_str(), &output, nullptr);
    if (output != nullptr)
    {
        outPath = output;
        NFD_FreePath(output);
    }
    return static_cast<FileDialogResult>(result);
}

FileDialogResult OpenDialogMultiple(const ea::string& filterList, const ea::string& defaultPath, ea::vector<ea::string>& outPaths)
{
    nfdpathset_t output{};
    auto result = NFD_OpenDialogMultiple(filterList.c_str(), defaultPath.c_str(), &output);
    if (result == NFD_OKAY)
    {
        for (size_t i = 0, end = NFD_PathSet_GetCount(&output); i < end; i++)
            outPaths.push_back(NFD_PathSet_GetPath(&output, i));
        NFD_PathSet_Free(&output);
    }
    return static_cast<FileDialogResult>(result);
}

FileDialogResult SaveDialog(const ea::string& filterList, const ea::string& defaultPath, ea::string& outPath)
{
    nfdchar_t* output = nullptr;
    auto result = NFD_SaveDialog(filterList.c_str(), defaultPath.c_str(), &output, nullptr);
    if (output != nullptr)
    {
        outPath = output;
        NFD_FreePath(output);
    }
    return static_cast<FileDialogResult>(result);
}

FileDialogResult PickFolder(const ea::string& defaultPath, ea::string& outPath)
{
    nfdchar_t* output = nullptr;
    auto result = NFD_PickFolder(defaultPath.c_str(), &output);
    if (output != nullptr)
    {
        outPath = output;
        NFD_FreePath(output);
    }
    return static_cast<FileDialogResult>(result);
}

}
