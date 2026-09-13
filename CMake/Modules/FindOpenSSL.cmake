# Copyright (c) 2017-2026 the rbfx project.
# This work is licensed under the terms of the MIT license.
# For a copy, see <https:#opensource.org/licenses/MIT> or the accompanying LICENSE file.

# A dummy script to help dependencies of libdatachannel to find OpenSSL.
if (TARGET ssl)
    add_library(OpenSSL::SSL ALIAS ssl)
endif ()
if (TARGET crypto)
    add_library(OpenSSL::Crypto ALIAS crypto)
endif ()
