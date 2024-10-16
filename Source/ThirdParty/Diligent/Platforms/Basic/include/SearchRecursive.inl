/*
 *  Copyright 2023 Diligent Graphics LLC
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

#pragma once

#include <string>
#include <vector>

namespace Diligent
{

template <typename FileSystem>
void SearchRecursiveImpl(const std::string& BaseDir, const std::string& SubDir, const Char* SearchPattern, std::vector<FindFileData>& Res)
{
    std::string Dir = BaseDir + SubDir;

    auto Files = FileSystem::Search((Dir + SearchPattern).c_str());
    Res.reserve(Res.size() + Files.size());
    for (auto& File : Files)
    {
        File.Name = SubDir + File.Name;
        Res.emplace_back(std::move(File));
    }

    auto AllFiles = FileSystem::Search((Dir + '*').c_str());
    for (const auto& File : AllFiles)
    {
        if (File.IsDirectory)
        {
            SearchRecursiveImpl<FileSystem>(BaseDir, SubDir + File.Name + FileSystem::SlashSymbol, SearchPattern, Res);
        }
    }
}

template <typename FileSystem>
typename FileSystem::SearchFilesResult SearchRecursive(const Char* Dir, const Char* SearchPattern)
{
    if (Dir == nullptr || Dir[0] == '\0')
    {
        UNEXPECTED("Directory must not be null or empty");
        return {};
    }
    if (SearchPattern == nullptr || SearchPattern[0] == '\0')
    {
        UNEXPECTED("Search pattern must not be null or empty");
        return {};
    }

    std::string BaseDir = Dir;
    if (BaseDir.back() != FileSystem::SlashSymbol)
        BaseDir += FileSystem::SlashSymbol;

    typename FileSystem::SearchFilesResult Res;
    SearchRecursiveImpl<FileSystem>(BaseDir, "", SearchPattern, Res);
    return Res;
}

} // namespace Diligent
