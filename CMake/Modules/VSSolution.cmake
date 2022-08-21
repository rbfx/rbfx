
function (vs_generate_sln SLN_PATH)
    # This is a dumb VS solution generator for grouping C# projects with non-VS generators. It makes several assumptions
    # that projects should satisfy:
    #  * Project should only contain 'Any CPU' configuration. Despite misleading name, this configuration will build as
    #    for a specific platform that CMake is configured to build. Platform type will be passed down from CMake and
    #    configured in Directory.Build.Props in engine root directory.
    #  * Project should contain Release, MinSizeRel, RelWithDebInfo and Debug configurations only. This matches CMake
    #    build configurations. If additional flexibility is needed, project should be configured via CMake parameters
    #    with MSBUILD_ prefix, these variables will be passed to MSBuild.
    #
    # Usage:
    #   vs_generate_sln(${CMAKE_BINARY_DIR}/rbfx.sln                                            # sln path
    #           ${rbfx_SOURCE_DIR}/Source/Urho3D/CSharp/Urho3DNet.csproj                        # ungrouped projects
    #           ${rbfx_SOURCE_DIR}/Source/Player/ManagedHost/Player.csproj
    #           ${rbfx_SOURCE_DIR}/Source/Urho3D/CSharp/Urho3DNet.Scripts.csproj
    #       Tools                                                                               # define project group
    #           ${rbfx_SOURCE_DIR}/Source/Tools/Editor/ManagedHost/Editor.csproj                # projects grouped under Tools
    #           ${rbfx_SOURCE_DIR}/Source/Tools/ScriptPlayer/ScriptPlayer.csproj
    #       Samples                                                                             # define another project group
    #           ${rbfx_SOURCE_DIR}/Source/Samples/102_CSharpProject/102_CSharpProject.csproj    # projects grouped under Samples
    #           ${rbfx_SOURCE_DIR}/Source/Samples/104_CSharpPlugin/104_CSharpPlugin.csproj
    #   )
    #
    set (GUID_FOLDER 2150E333-8FDC-42A3-9474-1A3956D46DE8)
    set (GUID_CSHARP FAE04EC0-301F-11D3-BF4B-00C04F79EFBC)

    # Header
    file(WRITE "${SLN_PATH}" "\r\nMicrosoft Visual Studio Solution File, Format Version 12.00\r\n")

    # Projects
    foreach (ARG ${ARGN})
        string(FIND "${ARG}" ".csproj" is_csproj)
        if (is_csproj EQUAL -1)
            set (ns "${GUID_FOLDER}")
        else ()
            set (ns "${GUID_CSHARP}")
        endif ()
        string(UUID arg_guid NAMESPACE "${ns}" NAME "${ARG}" TYPE SHA1 UPPER)
        file(APPEND "${SLN_PATH}" "Project(\"{${ns}}\") = \"${ARG}\", \"${ARG}\", \"{${arg_guid}}\"\r\n")
        file(APPEND "${SLN_PATH}" "EndProject\r\n")
    endforeach ()

    # Global section
    file(APPEND "${SLN_PATH}" "Global\r\n")
    file(APPEND "${SLN_PATH}" "\tGlobalSection(SolutionConfigurationPlatforms) = preSolution\r\n")
    file(APPEND "${SLN_PATH}" "\t\tRelease|${URHO3D_PLATFORM} = Release|${URHO3D_PLATFORM}\r\n")
    file(APPEND "${SLN_PATH}" "\t\tMinSizeRel|${URHO3D_PLATFORM} = MinSizeRel|${URHO3D_PLATFORM}\r\n")
    file(APPEND "${SLN_PATH}" "\t\tRelWithDebInfo|${URHO3D_PLATFORM} = RelWithDebInfo|${URHO3D_PLATFORM}\r\n")
    file(APPEND "${SLN_PATH}" "\t\tDebug|${URHO3D_PLATFORM} = Debug|${URHO3D_PLATFORM}\r\n")
    file(APPEND "${SLN_PATH}" "\tEndGlobalSection\r\n")
    file(APPEND "${SLN_PATH}" "\tGlobalSection(ProjectConfigurationPlatforms) = postSolution\r\n")

    # Configurations
    foreach (ARG ${ARGN})
        string(FIND "${ARG}" ".csproj" is_csproj)
        if (is_csproj GREATER -1)
            string(UUID arg_guid NAMESPACE "${GUID_CSHARP}" NAME "${ARG}" TYPE SHA1 UPPER)
            file(APPEND "${SLN_PATH}" "\t\t{${arg_guid}}.Release|${URHO3D_PLATFORM}.ActiveCfg = Release|${URHO3D_PLATFORM}\r\n")
            file(APPEND "${SLN_PATH}" "\t\t{${arg_guid}}.Release|${URHO3D_PLATFORM}.Build.0 = Release|${URHO3D_PLATFORM}\r\n")
            file(APPEND "${SLN_PATH}" "\t\t{${arg_guid}}.MinSizeRel|${URHO3D_PLATFORM}.ActiveCfg = MinSizeRel|${URHO3D_PLATFORM}\r\n")
            file(APPEND "${SLN_PATH}" "\t\t{${arg_guid}}.MinSizeRel|${URHO3D_PLATFORM}.Build.0 = MinSizeRel|${URHO3D_PLATFORM}\r\n")
            file(APPEND "${SLN_PATH}" "\t\t{${arg_guid}}.RelWithDebInfo|${URHO3D_PLATFORM}.ActiveCfg = RelWithDebInfo|${URHO3D_PLATFORM}\r\n")
            file(APPEND "${SLN_PATH}" "\t\t{${arg_guid}}.RelWithDebInfo|${URHO3D_PLATFORM}.Build.0 = RelWithDebInfo|${URHO3D_PLATFORM}\r\n")
            file(APPEND "${SLN_PATH}" "\t\t{${arg_guid}}.Debug|${URHO3D_PLATFORM}.ActiveCfg = Debug|${URHO3D_PLATFORM}\r\n")
            file(APPEND "${SLN_PATH}" "\t\t{${arg_guid}}.Debug|${URHO3D_PLATFORM}.Build.0 = Debug|${URHO3D_PLATFORM}\r\n")
        endif ()
    endforeach ()
    file(APPEND "${SLN_PATH}" "\tEndGlobalSection\r\n")
    file(APPEND "${SLN_PATH}" "\tGlobalSection(NestedProjects) = preSolution\r\n")

    # Folders
    set (CURRENT_FOLDER OFF)
    foreach (ARG ${ARGN})
        string(FIND "${ARG}" ".csproj" is_csproj)
        if (is_csproj EQUAL -1)
            string(UUID CURRENT_FOLDER NAMESPACE "${GUID_FOLDER}" NAME "${ARG}" TYPE SHA1 UPPER)
        elseif (CURRENT_FOLDER)
            string(UUID PROJECT_GUID NAMESPACE "${GUID_CSHARP}" NAME "${ARG}" TYPE SHA1 UPPER)
            file(APPEND "${SLN_PATH}" "\t\t{${PROJECT_GUID}} = {${CURRENT_FOLDER}}\r\n")
        endif ()
    endforeach ()

    # Footer
    file(APPEND "${SLN_PATH}" "\tEndGlobalSection\r\n")
    file(APPEND "${SLN_PATH}" "EndGlobal\r\n")
endfunction ()
