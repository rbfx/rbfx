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

#include "UWPDebug.hpp"
#include "FormatString.hpp"
#include <csignal>

#include "WinHPreface.h"
#include <windows.h>
#include "WinHPostface.h"

namespace Diligent
{

void WindowsStoreDebug::AssertionFailed(const Char* Message, const char* Function, const char* File, int Line)
{
    auto AssertionFailedMessage = FormatAssertionFailedMessage(Message, Function, File, Line);
    OutputDebugMessage(DEBUG_MESSAGE_SEVERITY_ERROR, AssertionFailedMessage.c_str(), nullptr, nullptr, 0);

    __debugbreak();
    //int nCode = MessageBoxA(NULL,
    //                        FullMsg.c_str(),
    //                        "Runtime assertion failed",
    //                        MB_TASKMODAL|MB_ICONHAND|MB_ABORTRETRYIGNORE|MB_SETFOREGROUND);

    //// Abort: abort the program
    //if (nCode == IDABORT)
    //{
    //    // raise abort signal
    //    raise(SIGABRT);

    //    // We usually won't get here, but it's possible that
    //    //  SIGABRT was ignored.  So exit the program anyway.
    //    exit(3);
    //}

    //// Retry: call the debugger
    //if (nCode == IDRETRY)
    //{
    //    DebugBreak();
    //    /* return to user code */
    //    return;
    //}

    //// Ignore: continue execution
    //if (nCode == IDIGNORE)
    //    return;
};

void WindowsStoreDebug::OutputDebugMessage(DEBUG_MESSAGE_SEVERITY Severity,
                                           const Char*            Message,
                                           const char*            Function,
                                           const char*            File,
                                           int                    Line,
                                           TextColor              Color)
{
    auto msg = FormatDebugMessage(Severity, Message, Function, File, Line);
    OutputDebugStringA(msg.c_str());
}

void DebugAssertionFailed(const Char* Message, const char* Function, const char* File, int Line)
{
    WindowsStoreDebug ::AssertionFailed(Message, Function, File, Line);
}

static void OutputDebugMessage(DEBUG_MESSAGE_SEVERITY Severity,
                               const Char*            Message,
                               const char*            Function,
                               const char*            File,
                               int                    Line)
{
    return WindowsStoreDebug::OutputDebugMessage(Severity, Message, Function, File, Line, TextColor::Auto);
}

DebugMessageCallbackType DebugMessageCallback = OutputDebugMessage;

} // namespace Diligent
