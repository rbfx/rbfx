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

#include "TestingEnvironment.hpp"
#include "PlatformDebug.hpp"
#include "DebugUtilities.hpp"

namespace Diligent
{

namespace Testing
{

TestingEnvironment* TestingEnvironment::m_pTheEnvironment = nullptr;
std::atomic_int     TestingEnvironment::m_NumAllowedErrors{0};

std::vector<std::string> TestingEnvironment::m_ExpectedErrorSubstrings;
std::mutex               TestingEnvironment::m_ExpectedErrorSubstringsMtx{};

TestingEnvironment::TestingEnvironment()
{
    VERIFY(m_pTheEnvironment == nullptr, "Testing environment object has already been initialized!");
    m_pTheEnvironment = this;
    SetDebugMessageCallback(MessageCallback);
}

void TestingEnvironment::MessageCallback(DEBUG_MESSAGE_SEVERITY Severity,
                                         const Char*            Message,
                                         const char*            Function,
                                         const char*            File,
                                         int                    Line)
{
    TextColor MsgColor = TextColor::Auto;
    if (Severity == DEBUG_MESSAGE_SEVERITY_ERROR || Severity == DEBUG_MESSAGE_SEVERITY_FATAL_ERROR)
    {
        if (m_NumAllowedErrors == 0)
        {
            ADD_FAILURE() << "Unexpected error";
        }
        else
        {
            m_NumAllowedErrors--;
            std::unique_lock<std::mutex> Lock{m_ExpectedErrorSubstringsMtx};
            if (!m_ExpectedErrorSubstrings.empty())
            {
                const auto& ErrorSubstring = m_ExpectedErrorSubstrings.back();
                if (strstr(Message, ErrorSubstring.c_str()) == nullptr)
                {
                    ADD_FAILURE() << "Expected error substring '" << ErrorSubstring << "' was not found in the error message";
                }
                m_ExpectedErrorSubstrings.pop_back();
            }
        }
        MsgColor = TextColor::DarkRed;
    }

    PlatformDebug::OutputDebugMessage(Severity, Message, Function, File, Line, MsgColor);
}

void TestingEnvironment::SetErrorAllowance(int NumErrorsToAllow, const char* InfoMessage)
{
    m_NumAllowedErrors = NumErrorsToAllow;
    if (InfoMessage != nullptr)
    {
        std::cout << TextColorCode::Cyan << InfoMessage << TextColorCode::Default;
    }
    if (m_NumAllowedErrors == 0)
    {
        m_ExpectedErrorSubstrings.clear();
    }
}

void TestingEnvironment::PushExpectedErrorSubstring(const char* Str, bool ClearStack)
{
    if (ClearStack)
        m_ExpectedErrorSubstrings.clear();
    VERIFY_EXPR(Str != nullptr && Str[0] != '\0');
    m_ExpectedErrorSubstrings.push_back(Str);
}


const char* TestingEnvironment::GetCurrentTestStatusString()
{
    static constexpr char TestFailedString[] = "\033[0;91m"
                                               "[  FAILED  ]"
                                               "\033[0;0m";
    static constexpr char TestPassedString[] = "\033[0;92m"
                                               "[  PASSED  ]"
                                               "\033[0;0m";
    return testing::Test::HasFailure() ? TestFailedString : TestPassedString;
}

const char* TestingEnvironment::GetTestSkippedString()
{
    return "\033[0;32m"
           "[  SKIPPED ]"
           "\033[0;0m";
}

} // namespace Testing

} // namespace Diligent
