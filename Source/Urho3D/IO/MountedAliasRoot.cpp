// Copyright (c) 2023-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/IO/MountedAliasRoot.h"

#include "Urho3D/IO/Log.h"

namespace Urho3D
{

namespace
{

static const ea::string aliasSeparator = ":/";

ea::string_view StripFileName(ea::string_view fileName, ea::string_view alias)
{
    const unsigned offset = alias.size() + aliasSeparator.size();
    return offset < fileName.size() ? fileName.substr(offset) : ea::string_view{};
}

FileIdentifier StripFileIdentifier(const FileIdentifier& fileName, ea::string_view alias, ea::string_view scheme)
{
    return {scheme, StripFileName(fileName.fileName_, alias)};
}

} // namespace

MountedAliasRoot::MountedAliasRoot(Context* context)
    : MountPoint(context)
{
}

MountedAliasRoot::~MountedAliasRoot() = default;

void MountedAliasRoot::AddAlias(const ea::string& path, const ea::string& scheme, MountPoint* mountPoint)
{
    aliases_[path] = {WeakPtr<MountPoint>(mountPoint), scheme};
}

void MountedAliasRoot::RemoveAliases(MountPoint* mountPoint)
{
    ea::erase_if(aliases_, [mountPoint](const auto& pair) { return pair.second.first == mountPoint; });
}

ea::tuple<MountPoint*, ea::string_view, ea::string_view> MountedAliasRoot::FindMountPoint(
    ea::string_view fileName) const
{
    const auto separatorPos = fileName.find(aliasSeparator);
    if (separatorPos == ea::string_view::npos)
        return {};

    const ea::string_view alias = fileName.substr(0, separatorPos);
    const auto iter = aliases_.find_as(alias);
    if (iter == aliases_.end() || !iter->second.first)
        return {};

    return {iter->second.first.Get(), iter->first, iter->second.second};
}

bool MountedAliasRoot::AcceptsScheme(const ea::string& scheme) const
{
    return scheme.comparei("alias") == 0;
}

bool MountedAliasRoot::Exists(const FileIdentifier& fileName) const
{
    if (!AcceptsScheme(fileName.scheme_))
        return false;

    const auto [mountPoint, alias, scheme] = FindMountPoint(fileName.fileName_);
    if (!mountPoint)
        return false;

    const FileIdentifier resolvedFileName = StripFileIdentifier(fileName, alias, scheme);
    return mountPoint->Exists(resolvedFileName);
}

AbstractFilePtr MountedAliasRoot::OpenFile(const FileIdentifier& fileName, FileMode mode)
{
    if (!AcceptsScheme(fileName.scheme_))
        return nullptr;

    const auto [mountPoint, alias, scheme] = FindMountPoint(fileName.fileName_);
    if (!mountPoint)
        return nullptr;

    const FileIdentifier resolvedFileName = StripFileIdentifier(fileName, alias, scheme);
    return mountPoint->OpenFile(resolvedFileName, mode);
}

ea::optional<FileTime> MountedAliasRoot::GetLastModifiedTime(
    const FileIdentifier& fileName, bool creationIsModification) const
{
    if (!AcceptsScheme(fileName.scheme_))
        return ea::nullopt;

    const auto [mountPoint, alias, scheme] = FindMountPoint(fileName.fileName_);
    if (!mountPoint)
        return ea::nullopt;

    const FileIdentifier resolvedFileName = StripFileIdentifier(fileName, alias, scheme);
    return mountPoint->GetLastModifiedTime(resolvedFileName, creationIsModification);
}

const ea::string& MountedAliasRoot::GetName() const
{
    static const ea::string name = "alias://";
    return name;
}

ea::string MountedAliasRoot::GetAbsoluteNameFromIdentifier(const FileIdentifier& fileName) const
{
    if (!AcceptsScheme(fileName.scheme_))
        return EMPTY_STRING;

    const auto [mountPoint, alias, scheme] = FindMountPoint(fileName.fileName_);
    if (!mountPoint)
        return EMPTY_STRING;

    const FileIdentifier resolvedFileName = StripFileIdentifier(fileName, alias, scheme);
    return mountPoint->GetAbsoluteNameFromIdentifier(resolvedFileName);
}

FileIdentifier MountedAliasRoot::GetIdentifierFromAbsoluteName(const ea::string& absoluteFileName) const
{
    // This operation is not supported, actual mount points should do this work.
    return FileIdentifier{};
}

void MountedAliasRoot::Scan(
    ea::vector<ea::string>& result, const ea::string& pathName, const ea::string& filter, ScanFlags flags) const
{
    // Scanning is not supported for aliases.
}

} // namespace Urho3D
