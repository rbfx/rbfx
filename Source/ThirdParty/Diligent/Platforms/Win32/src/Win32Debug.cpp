/*
 *  Copyright 2019-2024 Diligent Graphics LLC
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

#include "Win32Debug.hpp"
#include "FormatString.hpp"
#include <csignal>
#include <iostream>
#include <Windows.h>

namespace Diligent
{

namespace
{

class ConsoleSetUpHelper
{
public:
    ConsoleSetUpHelper()
    {
        // Set proper console mode to ensure colored output (required ENABLE_VIRTUAL_TERMINAL_PROCESSING flag is not set
        // by default for stdout when starting an app from Windows terminal).
        for (auto StdHandle : {GetStdHandle(STD_OUTPUT_HANDLE), GetStdHandle(STD_ERROR_HANDLE)})
        {
            DWORD Mode = 0;
            // https://docs.microsoft.com/en-us/windows/console/setconsolemode
            if (GetConsoleMode(StdHandle, &Mode))
            {
                // Characters written by the WriteFile or WriteConsole function or echoed by the ReadFile or
                // ReadConsole function are parsed for ASCII control sequences, and the correct action is performed.
                // Backspace, tab, bell, carriage return, and line feed characters are processed. It should be
                // enabled when using control sequences or when ENABLE_VIRTUAL_TERMINAL_PROCESSING is set.
                Mode |= ENABLE_PROCESSED_OUTPUT;

                // When writing with WriteFile or WriteConsole, characters are parsed for VT100 and similar
                // control character sequences that control cursor movement, color/font mode, and other operations
                // that can also be performed via the existing Console APIs.
                Mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;

                SetConsoleMode(StdHandle, Mode);
            }
        }
    }
};

} // namespace

void WindowsDebug::AssertionFailed(const Char* Message, const char* Function, const char* File, int Line)
{
    String AssertionFailedMessage = FormatAssertionFailedMessage(Message, Function, File, Line);
    if (DebugMessageCallback)
    {
        DebugMessageCallback(DEBUG_MESSAGE_SEVERITY_ERROR, AssertionFailedMessage.c_str(), nullptr, nullptr, 0);
    }
    else
    {
        OutputDebugMessage(DEBUG_MESSAGE_SEVERITY_ERROR, AssertionFailedMessage.c_str(), nullptr, nullptr, 0);
    }

    if (GetBreakOnError())
    {
        int nCode = MessageBoxA(NULL,
                                AssertionFailedMessage.c_str(),
                                "Runtime assertion failed",
                                MB_TASKMODAL | MB_ICONHAND | MB_ABORTRETRYIGNORE | MB_SETFOREGROUND);

        // Abort: abort the program
        if (nCode == IDABORT)
        {
            // raise abort signal
            raise(SIGABRT);

            // We usually won't get here, but it's possible that
            //  SIGABRT was ignored.  So exit the program anyway.
            exit(3);
        }

        // Retry: call the debugger
        if (nCode == IDRETRY)
        {
            DebugBreak();
            // return to user code
            return;
        }

        // Ignore: continue execution
        if (nCode == IDIGNORE)
            return;
    }
}

void WindowsDebug::OutputDebugMessage(DEBUG_MESSAGE_SEVERITY Severity,
                                      const Char*            Message,
                                      const char*            Function,
                                      const char*            File,
                                      int                    Line,
                                      TextColor              Color)
{
    static ConsoleSetUpHelper SetUpConsole;

    auto msg = FormatDebugMessage(Severity, Message, Function, File, Line);
    OutputDebugStringA(msg.c_str());

    const auto* ColorCode = TextColorToTextColorCode(Severity, Color);
    std::cout << ColorCode << msg << TextColorCode::Default;
}

void DebugAssertionFailed(const Char* Message, const char* Function, const char* File, int Line)
{
    WindowsDebug::AssertionFailed(Message, Function, File, Line);
}

static void OutputDebugMessage(DEBUG_MESSAGE_SEVERITY Severity,
                               const Char*            Message,
                               const char*            Function,
                               const char*            File,
                               int                    Line)
{
    return WindowsDebug::OutputDebugMessage(Severity, Message, Function, File, Line, TextColor::Auto);
}

DebugMessageCallbackType DebugMessageCallback = OutputDebugMessage;

} // namespace Diligent
