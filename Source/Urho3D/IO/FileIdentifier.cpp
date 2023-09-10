//
// Copyright (c) 2022-2022 the Urho3D project.
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

#include "Urho3D/IO/FileIdentifier.h"

#include "Urho3D/Container/Str.h"

namespace Urho3D
{

const FileIdentifier FileIdentifier::Empty{};

FileIdentifier::FileIdentifier(ea::string_view scheme, ea::string_view fileName)
    : scheme_(scheme)
    , fileName_(fileName)
{
}

FileIdentifier FileIdentifier::FromUri(ea::string_view uri)
{
    // Special case: absolute path
    if (uri.starts_with('/') || (uri.length() >= 3 && uri[1] == ':' && (uri[2] == '/' || uri[2] == '\\')))
        return {"file", SanitizeFileName(uri)};

    const auto schemePos = uri.find(":");

    // Special case: empty scheme
    if (schemePos == ea::string_view::npos)
        return {EMPTY_STRING, SanitizeFileName(uri)};

    const auto scheme = uri.substr(0, schemePos);
    const auto path = uri.substr(schemePos + 1);

    const auto isSlash = [](char c) { return c == '/'; };
    const unsigned numSlashes = ea::find_if_not(path.begin(), path.end(), isSlash) - path.begin();

    // Special case: file scheme
    if (scheme == "file")
    {
        if (numSlashes == 0 || numSlashes > 3)
            return FileIdentifier::Empty;

        // Keep one leading slash
        const auto localPath = path.substr(numSlashes - 1);

        // Windows-like path, e.g. /c:/path/to/file
        if (localPath.size() >= 3 && localPath[2] == ':')
            return {scheme, localPath.substr(1)};

        // Unix-like path, e.h. /path/to/file
        return {scheme, localPath};
    }

    // Trim up to two leading slashes for other schemes
    return {scheme, path.substr(ea::min(numSlashes, 2u))};
}

ea::string FileIdentifier::ToUri() const
{
    // Special case: empty scheme
    if (scheme_.empty())
        return fileName_;

    // Special case: file scheme
    if (scheme_ == "file")
    {
        if (fileName_.empty())
            return ea::string{};
        else if (fileName_.front() == '/')
            return "file://" + fileName_;
        else
            return "file:///" + fileName_;
    }

    // Use scheme://path/to/file format by default
    return scheme_ + "://" + fileName_;
}

void FileIdentifier::AppendPath(ea::string_view path)
{
    if (path.empty())
        return;

    if (fileName_.empty())
    {
        fileName_ = path;
        return;
    }

    if (fileName_.back() != '/' && path.front() != '/')
        fileName_.push_back('/');
    if (fileName_.back() == '/' && path.front() == '/')
        path = path.substr(1);

    fileName_.append(path.data(), path.length());
}

ea::string FileIdentifier::SanitizeFileName(ea::string_view fileName)
{
    ea::string sanitizedName;
    sanitizedName.reserve(fileName.length());

    size_t segmentStartIndex = 0;
    for (auto c: fileName)
    {
        if (c == '\\' || c == '/')
        {
            if (sanitizedName.size() - segmentStartIndex <= 2)
            {
                auto segment = sanitizedName.substr(segmentStartIndex);
                if (segment == ".")
                {
                    sanitizedName.resize(segmentStartIndex);
                    continue;
                }
                if (segment == "..")
                {
                    // If there is a posibility of parent path...
                    if (segmentStartIndex > 1)
                    {
                        // Find where parent path starts...
                        segmentStartIndex = sanitizedName.find_last_of('/', segmentStartIndex - 2);
                        // Find where parent path starts and set segment start right after / symbol.
                        segmentStartIndex = (segmentStartIndex == ea::string::npos) ? 0 : segmentStartIndex + 1;
                    }
                    else
                    {
                        // If there is no way the parent path has parent of it's own then reset full path to empty.
                        segmentStartIndex = 0;
                    }
                    // Reset sanitized name to position right after last known / or at the start.
                    sanitizedName.resize(segmentStartIndex);
                    continue;
                }
            }
            sanitizedName.push_back('/');
            segmentStartIndex = sanitizedName.size();
        }
        else
        {
            sanitizedName.push_back(c);
        }
    }
    sanitizedName.trim();
    return sanitizedName;
}

} // namespace Urho3D
