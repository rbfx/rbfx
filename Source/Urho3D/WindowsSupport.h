// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#ifdef _WIN32
#   ifndef WIN32_LEAN_AND_MEAN
#       define WIN32_LEAN_AND_MEAN
#endif
#   include <windows.h>
#   ifdef TRANSPARENT
#       undef TRANSPARENT
#   endif
#   ifdef SendMessage
#       undef SendMessage
#   endif
#   ifdef GetMessage
#       undef GetMessage
#   endif
#   ifdef GetObject
#       undef GetObject
#   endif
#   ifdef min
#       undef min
#   endif
#   ifdef max
#       undef max
#   endif
#endif
