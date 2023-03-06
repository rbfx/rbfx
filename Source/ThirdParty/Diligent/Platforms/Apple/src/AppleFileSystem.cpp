/*
 *  Copyright 2019-2022 Diligent Graphics LLC
 *  Copyright 2015-2019 Egor Yusov
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  In no event and under no legal theory, whether in tort (including negligence),
 *  contract, or otherwise, unless required by applicable law (such as deliberate
 *  and grossly negligent acts) or agreed to in writing, shall any Contributor be
 *  liable for any damages, including any direct, indirect, special, incidental,
 *  or consequential damages of any character arising as a result of this License or
 *  out of the use or inability to use the software (including but not limited to damages
 *  for loss of goodwill, work stoppage, computer failure or malfunction, or any and
 *  all other commercial damages or losses), even if such Contributor has been advised
 *  of the possibility of such damages.
 */

#include <stdio.h>
#include <unistd.h>
#include <cstdio>
#include <sys/stat.h>
#include <sys/types.h>
#include <CoreFoundation/CoreFoundation.h>

#include "CFObjectWrapper.hpp"

#include "AppleFileSystem.hpp"
#include "Errors.hpp"
#include "DebugUtilities.hpp"

namespace
{

std::string FindResource(const std::string& FilePath)
{
    std::string dir, name;
    Diligent::BasicFileSystem::GetPathComponents(FilePath, &dir, &name);
    auto        dotPos = name.find(".");
    std::string type   = (dotPos != std::string::npos) ? name.substr(dotPos + 1) : "";
    if (dotPos != std::string::npos)
        name.erase(dotPos);

    // Naming convention established by Core Foundation library:
    // * If a function name contains the word "Create" or "Copy", you own the object.
    // * If a function name contains the word "Get", you do not own the object.
    // If you own an object, it is your responsibility to relinquish ownership
    // (using CFRelease) when you have finished with it.
    // https://developer.apple.com/library/content/documentation/CoreFoundation/Conceptual/CFMemoryMgmt/Concepts/Ownership.html

    // get bundle and CFStrings
    CFBundleRef               mainBundle       = CFBundleGetMainBundle();
    Diligent::CFStringWrapper cf_resource_path = CFStringCreateWithCString(NULL, dir.c_str(), kCFStringEncodingUTF8);
    Diligent::CFStringWrapper cf_filename      = CFStringCreateWithCString(NULL, name.c_str(), kCFStringEncodingUTF8);
    Diligent::CFStringWrapper cf_file_type     = CFStringCreateWithCString(NULL, type.c_str(), kCFStringEncodingUTF8);
    Diligent::CFURLWrapper    cf_url_resource  = CFBundleCopyResourceURL(mainBundle, cf_filename, cf_file_type, cf_resource_path);
    std::string               resource_path;
    if (cf_url_resource != NULL)
    {
        Diligent::CFStringWrapper cf_url_string = CFURLCopyFileSystemPath(cf_url_resource, kCFURLPOSIXPathStyle);
        const char*               url_string    = CFStringGetCStringPtr(cf_url_string, kCFStringEncodingUTF8);
        resource_path                           = url_string;
    }
    return resource_path;
}

} // namespace

namespace Diligent
{

AppleFile* AppleFileSystem::OpenFile(const FileOpenAttribs& OpenAttribs)
{
    // Try to find the file in the bundle first
    std::string path(OpenAttribs.strFilePath);
    CorrectSlashes(path);
    auto resource_path = FindResource(path);

    AppleFile* pFile = nullptr;
    if (!resource_path.empty())
    {
        try
        {
            FileOpenAttribs BundleResourceOpenAttribs = OpenAttribs;
            BundleResourceOpenAttribs.strFilePath     = resource_path.c_str();
            pFile                                     = new AppleFile{BundleResourceOpenAttribs};
        }
        catch (const std::runtime_error& err)
        {
        }
    }

    if (pFile == nullptr)
    {
        try
        {
            pFile = new AppleFile{OpenAttribs};
        }
        catch (const std::runtime_error& err)
        {
        }
    }
    return pFile;
}

bool AppleFileSystem::FileExists(const Char* strFilePath)
{
    if (LinuxFileSystem::FileExists(strFilePath))
        return true;

    // Try to find the file in bundle resources
    std::string path{strFilePath};
    CorrectSlashes(path);
    const auto resource_path = FindResource(path);

    return !resource_path.empty();
}

} // namespace Diligent
