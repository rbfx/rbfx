/*
 *  Copyright 2019-2022 Diligent Graphics LLC
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

#include "FileSystem.hpp"

#include "gtest/gtest.h"

using namespace Diligent;

namespace
{

TEST(Platforms_FileSystem, SimplifyPath)
{
    EXPECT_STREQ(FileSystem::SimplifyPath("", '/').c_str(), "");
    EXPECT_STREQ(FileSystem::SimplifyPath("", '\\').c_str(), "");
    EXPECT_STREQ(FileSystem::SimplifyPath("a", '/').c_str(), "a");
    EXPECT_STREQ(FileSystem::SimplifyPath("a", '\\').c_str(), "a");

    EXPECT_STREQ(FileSystem::SimplifyPath("/", '/').c_str(), "");
    EXPECT_STREQ(FileSystem::SimplifyPath("/", '\\').c_str(), "");
    EXPECT_STREQ(FileSystem::SimplifyPath("\\", '/').c_str(), "");
    EXPECT_STREQ(FileSystem::SimplifyPath("\\", '\\').c_str(), "");

    EXPECT_STREQ(FileSystem::SimplifyPath("a/", '/').c_str(), "a");
    EXPECT_STREQ(FileSystem::SimplifyPath("a/", '\\').c_str(), "a");
    EXPECT_STREQ(FileSystem::SimplifyPath("a\\", '/').c_str(), "a");
    EXPECT_STREQ(FileSystem::SimplifyPath("a\\", '\\').c_str(), "a");

    EXPECT_STREQ(FileSystem::SimplifyPath("/a", '/').c_str(), "a");
    EXPECT_STREQ(FileSystem::SimplifyPath("/a", '\\').c_str(), "a");
    EXPECT_STREQ(FileSystem::SimplifyPath("\\a", '/').c_str(), "a");
    EXPECT_STREQ(FileSystem::SimplifyPath("\\a", '\\').c_str(), "a");

    EXPECT_STREQ(FileSystem::SimplifyPath("/a/", '/').c_str(), "a");
    EXPECT_STREQ(FileSystem::SimplifyPath("/a/", '\\').c_str(), "a");
    EXPECT_STREQ(FileSystem::SimplifyPath("\\a/", '/').c_str(), "a");
    EXPECT_STREQ(FileSystem::SimplifyPath("\\a/", '\\').c_str(), "a");

    EXPECT_STREQ(FileSystem::SimplifyPath("a/b", '/').c_str(), "a/b");
    EXPECT_STREQ(FileSystem::SimplifyPath("a\\b", '/').c_str(), "a/b");
    EXPECT_STREQ(FileSystem::SimplifyPath("a/b", '\\').c_str(), "a\\b");
    EXPECT_STREQ(FileSystem::SimplifyPath("a\\b", '\\').c_str(), "a\\b");
    EXPECT_STREQ(FileSystem::SimplifyPath("a//b", '/').c_str(), "a/b");
    EXPECT_STREQ(FileSystem::SimplifyPath("a\\\\b", '/').c_str(), "a/b");
    EXPECT_STREQ(FileSystem::SimplifyPath("a//b", '\\').c_str(), "a\\b");
    EXPECT_STREQ(FileSystem::SimplifyPath("a\\\\b", '\\').c_str(), "a\\b");

    EXPECT_STREQ(FileSystem::SimplifyPath("a/./b", '/').c_str(), "a/b");
    EXPECT_STREQ(FileSystem::SimplifyPath("a\\.\\b", '/').c_str(), "a/b");
    EXPECT_STREQ(FileSystem::SimplifyPath("a/./b", '\\').c_str(), "a\\b");
    EXPECT_STREQ(FileSystem::SimplifyPath("a\\.\\b", '\\').c_str(), "a\\b");

    EXPECT_STREQ(FileSystem::SimplifyPath("./a", '/').c_str(), "a");
    EXPECT_STREQ(FileSystem::SimplifyPath(".\\a", '/').c_str(), "a");
    EXPECT_STREQ(FileSystem::SimplifyPath("./a", '\\').c_str(), "a");
    EXPECT_STREQ(FileSystem::SimplifyPath(".\\a", '\\').c_str(), "a");

    EXPECT_STREQ(FileSystem::SimplifyPath("a/.", '/').c_str(), "a");
    EXPECT_STREQ(FileSystem::SimplifyPath("a\\.", '/').c_str(), "a");
    EXPECT_STREQ(FileSystem::SimplifyPath("a/.", '\\').c_str(), "a");
    EXPECT_STREQ(FileSystem::SimplifyPath("a\\.", '\\').c_str(), "a");

    EXPECT_STREQ(FileSystem::SimplifyPath("a./b", '/').c_str(), "a./b");
    EXPECT_STREQ(FileSystem::SimplifyPath("a.\\b", '/').c_str(), "a./b");
    EXPECT_STREQ(FileSystem::SimplifyPath("a./b", '\\').c_str(), "a.\\b");
    EXPECT_STREQ(FileSystem::SimplifyPath("a.\\b", '\\').c_str(), "a.\\b");

    EXPECT_STREQ(FileSystem::SimplifyPath("a/.b", '/').c_str(), "a/.b");
    EXPECT_STREQ(FileSystem::SimplifyPath("a\\.b", '/').c_str(), "a/.b");
    EXPECT_STREQ(FileSystem::SimplifyPath("a/.b", '\\').c_str(), "a\\.b");
    EXPECT_STREQ(FileSystem::SimplifyPath("a\\.b", '\\').c_str(), "a\\.b");

    EXPECT_STREQ(FileSystem::SimplifyPath("a.b/c", '/').c_str(), "a.b/c");
    EXPECT_STREQ(FileSystem::SimplifyPath("a.b\\c", '/').c_str(), "a.b/c");
    EXPECT_STREQ(FileSystem::SimplifyPath("a.b/c", '\\').c_str(), "a.b\\c");
    EXPECT_STREQ(FileSystem::SimplifyPath("a.b\\c", '\\').c_str(), "a.b\\c");

    EXPECT_STREQ(FileSystem::SimplifyPath("..", '/').c_str(), "..");
    EXPECT_STREQ(FileSystem::SimplifyPath("..", '\\').c_str(), "..");
    EXPECT_STREQ(FileSystem::SimplifyPath("../a", '/').c_str(), "../a");
    EXPECT_STREQ(FileSystem::SimplifyPath("../a", '\\').c_str(), "..\\a");
    EXPECT_STREQ(FileSystem::SimplifyPath("..\\a", '/').c_str(), "../a");
    EXPECT_STREQ(FileSystem::SimplifyPath("..\\a", '\\').c_str(), "..\\a");

    EXPECT_STREQ(FileSystem::SimplifyPath("a/..", '/').c_str(), "");
    EXPECT_STREQ(FileSystem::SimplifyPath("a/..", '\\').c_str(), "");
    EXPECT_STREQ(FileSystem::SimplifyPath("a\\..", '/').c_str(), "");
    EXPECT_STREQ(FileSystem::SimplifyPath("a\\..", '\\').c_str(), "");

    EXPECT_STREQ(FileSystem::SimplifyPath("a/b/../c", '/').c_str(), "a/c");
    EXPECT_STREQ(FileSystem::SimplifyPath("a/b/../c", '\\').c_str(), "a\\c");
    EXPECT_STREQ(FileSystem::SimplifyPath("a\\b\\..\\c", '/').c_str(), "a/c");
    EXPECT_STREQ(FileSystem::SimplifyPath("a\\b\\..\\c", '\\').c_str(), "a\\c");

    EXPECT_STREQ(FileSystem::SimplifyPath("a../b", '/').c_str(), "a../b");
    EXPECT_STREQ(FileSystem::SimplifyPath("a..\\b", '/').c_str(), "a../b");
    EXPECT_STREQ(FileSystem::SimplifyPath("a../b", '\\').c_str(), "a..\\b");
    EXPECT_STREQ(FileSystem::SimplifyPath("a..\\b", '\\').c_str(), "a..\\b");

    EXPECT_STREQ(FileSystem::SimplifyPath("a/..b", '/').c_str(), "a/..b");
    EXPECT_STREQ(FileSystem::SimplifyPath("a\\..b", '/').c_str(), "a/..b");
    EXPECT_STREQ(FileSystem::SimplifyPath("a/..b", '\\').c_str(), "a\\..b");
    EXPECT_STREQ(FileSystem::SimplifyPath("a\\..b", '\\').c_str(), "a\\..b");

    EXPECT_STREQ(FileSystem::SimplifyPath("a..b/c", '/').c_str(), "a..b/c");
    EXPECT_STREQ(FileSystem::SimplifyPath("a..b\\c", '/').c_str(), "a..b/c");
    EXPECT_STREQ(FileSystem::SimplifyPath("a..b/c", '\\').c_str(), "a..b\\c");
    EXPECT_STREQ(FileSystem::SimplifyPath("a..b\\c", '\\').c_str(), "a..b\\c");

    EXPECT_STREQ(FileSystem::SimplifyPath("../..", '/').c_str(), "../..");
    EXPECT_STREQ(FileSystem::SimplifyPath("..\\..", '/').c_str(), "../..");
    EXPECT_STREQ(FileSystem::SimplifyPath("../..", '\\').c_str(), "..\\..");
    EXPECT_STREQ(FileSystem::SimplifyPath("..\\..", '\\').c_str(), "..\\..");
}

TEST(Platforms_FileSystem, SplitPathList)
{
    auto TestPaths = [](const char* PathList, std::vector<std::string> Expected) {
        std::vector<std::string> Paths;
        FileSystem::SplitPathList(PathList,
                                  [&Paths](const char* Path, size_t Len) //
                                  {
                                      Paths.emplace_back(std::string{Path, Len});
                                      return true;
                                  });
        EXPECT_EQ(Paths.size(), Expected.size());
        if (Paths.size() != Expected.size())
            return;

        for (size_t i = 0; i < Expected.size(); ++i)
        {
            EXPECT_EQ(Paths[i], Expected[i]);
        }
    };
    TestPaths("", {});
    TestPaths(";", {});
    TestPaths(";;", {});
    TestPaths("path", {{"path"}});
    TestPaths(";path", {{"path"}});
    TestPaths("path;", {{"path"}});
    TestPaths("path;;", {{"path"}});
    TestPaths(";;path;;", {{"path"}});
    TestPaths("path1;path2", {{"path1"}, {"path2"}});
    TestPaths("path1;;path2", {{"path1"}, {"path2"}});
    TestPaths("path1;;path2;", {{"path1"}, {"path2"}});
    TestPaths("path1;;path2;", {{"path1"}, {"path2"}});
    TestPaths(";;path1;;path2;", {{"path1"}, {"path2"}});
    TestPaths("c:\\windows\\path1;c:\\windows\\path2", {{"c:\\windows\\path1"}, {"c:\\windows\\path2"}});
    TestPaths("/unix/path1;/unix/path2", {{"/unix/path1"}, {"/unix/path2"}});
}

} // namespace
