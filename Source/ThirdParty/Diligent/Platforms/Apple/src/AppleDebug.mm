/*
 *  Copyright 2019-2024 Diligent Graphics LLC
 *  Copyright 2015-2019 Egor Yusov
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF ANY PROPRIETARY RIGHTS.
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

#include <csignal>
#include <iostream>

#import <Foundation/Foundation.h>

#include "AppleDebug.hpp"
#include "FormatString.hpp"

namespace Diligent 
{

void AppleDebug::AssertionFailed(const Char *Message, const char *Function, const char *File, int Line)
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
        raise(SIGTRAP);
    }
}

bool AppleDebug::ColoredTextSupported()
{
    static const bool StartedFromXCode =
        []() -> bool
        {
            NSDictionary* environment = [[NSProcessInfo processInfo] environment];
            if (NSString* DtMode = [environment objectForKey:@"IDE_DISABLED_OS_ACTIVITY_DT_MODE"]) // Since XCode 15
            {
                return [DtMode boolValue];
            }
            else if (NSString* DtMode = [environment objectForKey:@"OS_ACTIVITY_DT_MODE"]) // Before XCode 15
            {
                return [DtMode boolValue];
            }
            else
            {
                return false;
            }
        }();
    // XCode does not support colored console
    return !StartedFromXCode;
}

void AppleDebug::OutputDebugMessage(DEBUG_MESSAGE_SEVERITY Severity,
                                    const Char*            Message,
                                    const char*            Function,
                                    const char*            File,
                                    int                    Line,
                                    TextColor              Color)
{
    auto msg = FormatDebugMessage(Severity, Message, Function, File, Line);
    
    if (ColoredTextSupported())
    {
        const auto* ColorCode = TextColorToTextColorCode(Severity, Color);
        printf("%s%s%s\n", ColorCode, msg.c_str(), TextColorCode::Default);
        // NSLog truncates the log at 1024 symbols
        //NSLog(@"%s", str.c_str());
    }
    else
    {
        printf("%s\n",msg.c_str());
    }
}

void DebugAssertionFailed(const Char* Message, const char* Function, const char* File, int Line)
{
    AppleDebug::AssertionFailed( Message, Function, File, Line );
}

static void OutputDebugMessage(DEBUG_MESSAGE_SEVERITY Severity,
                               const Char*            Message,
                               const char*            Function,
                               const char*            File,
                               int                    Line)
{
    return AppleDebug::OutputDebugMessage(Severity, Message, Function, File, Line, TextColor::Auto);
}

DebugMessageCallbackType DebugMessageCallback = OutputDebugMessage;

} // namespace Diligent
