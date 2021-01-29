#
# Copyright (c) 2017-2020 the rbfx project.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#

file (GLOB_RECURSE SOURCE_FILES *.h *.cpp Source)
add_library(SLikeNet STATIC ${SOURCE_FILES})

if (BUILD_SHARED_LIBS)
    target_compile_definitions(SLikeNet PRIVATE -D_RAKNET_DLL=1)
endif ()

target_compile_definitions(SLikeNet PRIVATE
    -DRAKNET_ENABLE_STATIC
    -D_RAKNET_SUPPORT_NatTypeDetectionServer=0
    -D_RAKNET_SUPPORT_UDPForwarder=0
    -D_RAKNET_SUPPORT_TwoWayAuthentication=0
    -D_RAKNET_SUPPORT_CloudClient=0
    -D_RAKNET_SUPPORT_CloudServer=0
    -D_RAKNET_SUPPORT_ConnectionGraph2=0
    -D_RAKNET_SUPPORT_NatPunchthroughServer=0
    -D_RAKNET_SUPPORT_RelayPlugin=0
    -D_RAKNET_SUPPORT_LibVoice=0
    -D_RAKNET_SUPPORT_DynDNS=0
    -D_RAKNET_SUPPORT_HTTPConnection2=0
    -D_RAKNET_SUPPORT_HTTPConnection=0
    -D_RAKNET_SUPPORT_EmailSender=0
    -D_RAKNET_SUPPORT_UDPProxyClient=0
    -D_RAKNET_SUPPORT_UDPProxyCoordinator=0
    -D_RAKNET_SUPPORT_TeamManager=0
    -D_RAKNET_SUPPORT_TeamBalancer=0
    -D_RAKNET_SUPPORT_NatTypeDetectionClient=0
    -D_RAKNET_SUPPORT_FullyConnectedMesh2=0
    -D_RAKNET_SUPPORT_ConnectionGraph2=0
    -D_RAKNET_SUPPORT_FullyConnectedMesh=0
    -D_RAKNET_SUPPORT_TelnetTransport=0
    -D_RAKNET_SUPPORT_PacketLogger=0
    -D_RAKNET_SUPPORT_DirectoryDeltaTransfer=0
    -D_RAKNET_SUPPORT_FileOperations=0
    -D_RAKNET_SUPPORT_UDPProxyServer=0
    -D_RAKNET_SUPPORT_ConsoleServer=0
    -D_RAKNET_SUPPORT_RakNetTransport=0
    -D_RAKNET_SUPPORT_TCPInterface=0
    -D_RAKNET_SUPPORT_LogCommandParser=0
    -D_RAKNET_SUPPORT_RakNetCommandParser=0
    -D_RAKNET_SUPPORT_Rackspace=0
    -D_RAKNET_SUPPORT_PacketizedTCP=0
    -D_RAKNET_SUPPORT_RPC4Plugin=0
    -D_RAKNET_SUPPORT_Router2=0
    -D_RAKNET_SUPPORT_ReplicaManager3=0
    -D_RAKNET_SUPPORT_ReadyEvent=0
    -D_RAKNET_SUPPORT_MessageFilter=0
    -D_RAKNET_SUPPORT_FileListTransfer=0
    -D_WINSOCK_DEPRECATED_NO_WARNINGS=1
)

if (WIN32)
    target_compile_definitions(SLikeNet PRIVATE -DINET_FUNCTIONS_MISSING=1)
endif()

# Check specific function availability which may be missing from different MinGW versions
check_cxx_source_compiles ("#include <stdio.h>\nint main() { char s[80];\nint x = 1;\nsprintf_s(s, 80, \"Test:%d\", x); }" SPRINTFS_FUNCTION_EXISTS)

if (NOT SPRINTFS_FUNCTION_EXISTS)
    target_compile_definitions(SLikeNet PRIVATE -DSPRINTFS_FUNCTION_MISSING=1)
endif()

# Check specific function availability which may be missing from different MinGW versions
check_cxx_source_compiles ("#include <stdio.h>\nint main() {\nchar s[80];\nint x = 1;\nva_list args = {};\nvsnprintf_s(s, 80, \"Test:%d\", args);}" VSNPRINTFS_FUNCTION_EXISTS)

if (NOT VSNPRINTFS_FUNCTION_EXISTS)
    target_compile_definitions(SLikeNet PRIVATE -DVSNPRINTFS_FUNCTION_MISSING=1)
endif()

target_include_directories(SLikeNet
    SYSTEM PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Source/include>
)

if (WIN32)
    target_link_libraries(SLikeNet PUBLIC iphlpapi ws2_32)
endif ()

if (NOT MINI_URHO)
    install (DIRECTORY Source/include/ DESTINATION ${DEST_THIRDPARTY_HEADERS_DIR}/ FILES_MATCHING PATTERN *.h)

    if (NOT URHO3D_MERGE_STATIC_LIBS)
        install(TARGETS SLikeNet EXPORT Urho3D ARCHIVE DESTINATION ${DEST_ARCHIVE_DIR_CONFIG})
    endif ()
endif ()
