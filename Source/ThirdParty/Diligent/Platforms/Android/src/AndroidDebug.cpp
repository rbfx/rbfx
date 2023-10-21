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

#include "AndroidDebug.hpp"
#include "FormatString.hpp"
#include "../../../Common/interface/StringTools.hpp"
#include <android/log.h>
#include <csignal>

namespace Diligent
{

void AndroidDebug::AssertionFailed(const Char* Message, const char* Function, const char* File, int Line)
{
    auto AssertionFailedMessage = FormatAssertionFailedMessage(Message, Function, File, Line);
    OutputDebugMessage(DEBUG_MESSAGE_SEVERITY_ERROR, AssertionFailedMessage.c_str(), nullptr, nullptr, 0);

    raise(SIGTRAP);
};

void AndroidDebug::OutputDebugMessage(DEBUG_MESSAGE_SEVERITY Severity,
                                      const Char*            Message,
                                      const char*            Function,
                                      const char*            File,
                                      int                    Line,
                                      TextColor              Color)
{
    auto Msg = FormatDebugMessage(Severity, Message, Function, File, Line);

    static constexpr android_LogPriority Priorities[]    = {ANDROID_LOG_INFO, ANDROID_LOG_WARN, ANDROID_LOG_ERROR, ANDROID_LOG_FATAL};
    static constexpr size_t              LogcatMsgMaxLen = 1024;
    const auto                           Priority        = Priorities[static_cast<int>(Severity)];

    SplitLongString(Msg.begin(), Msg.end(), LogcatMsgMaxLen, 80,
                    [&](std::string::iterator Start, std::string::iterator End) {
                        char tmp = '\0';
                        if (End != Msg.end())
                        {
                            // Temporarily break the string
                            tmp  = *End;
                            *End = '\0';
                        }

                        __android_log_print(Priority, "Diligent Engine", "%s", &*Start);

                        if (tmp != '\0')
                            *End = tmp;
                    });
}

void DebugAssertionFailed(const Char* Message, const char* Function, const char* File, int Line)
{
    AndroidDebug::AssertionFailed(Message, Function, File, Line);
}

static void OutputDebugMessage(DEBUG_MESSAGE_SEVERITY Severity,
                               const Char*            Message,
                               const char*            Function,
                               const char*            File,
                               int                    Line)
{
    return AndroidDebug::OutputDebugMessage(Severity, Message, Function, File, Line, TextColor::Auto);
}

DebugMessageCallbackType DebugMessageCallback = OutputDebugMessage;

} // namespace Diligent
