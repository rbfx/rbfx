#!/usr/bin/env bash

# ci_build.sh <action> [options] [-- extra cmake args]
#
# Actions: dependencies|generate|generate-detect-thirdparty|build|build-swig|
#          install|test|cstest|apk|publish-to-itch|release-mobile-artifacts|
#          test-project|build-project|build-android-project|download-release|
#          download-cached-sdk|download-nuget-sdks|sanitize-cached-sdk|
#          copy-cached-sdk|setup-package-tool|setup-emsdk|setup-environment|
#          stage-project-artifacts|upload-release-archive|release-sdk|
#          refresh-latest-tag
#
# Environment variables (used for defaults):
#   ci_platform:      windows|linux|macos|android|ios|web|uwp
#   ci_arch:          x86|x64|arm|arm64|wasm|universal
#   ci_compiler:      msvc|gcc|clang|emscripten|mingw
#   ci_lib_type:      lib|dll
#   ci_platform_tag:  platform-compiler-arch-libtype (e.g., android-clang-arm64-dll)
#   ci_workspace_dir: actions workspace directory
#   ci_source_dir:    source code directory
#   ci_build_dir:     cmake cache directory
#   ci_sdk_dir:       sdk installation directory

# Parse action (required positional argument)
if [ $# -eq 0 ];
then
    echo "Usage: $0 <action> [options] [-- extra_args]"
    echo ""
    echo "Actions:"
    echo "  dependencies              Install build dependencies"
    echo "  generate                  Generate build files with CMake"
    echo "  generate-detect-thirdparty Generate build files and report ThirdParty cache usage"
    echo "  build                     Build the project"
    echo "  build-swig                Build host SWIG tool for cross builds"
    echo "  install                   Install build artifacts"
    echo "  test                      Run native tests"
    echo "  cstest                    Run C# tests"
    echo "  apk                       Build Android APK"
    echo "  publish-to-itch           Publish to itch.io"
    echo "  release-mobile-artifacts  Release mobile artifacts"
    echo "  test-project              Test external project"
    echo "  build-project             Build downstream project"
    echo "  build-android-project     Build downstream Android project"
    echo "  download-release          Download and optionally verify release"
    echo "  download-cached-sdk       Download and verify cached ThirdParty SDK"
    echo "  download-nuget-sdks       Download SDKs for NuGet"
    echo "  sanitize-cached-sdk       Remove engine package exports from cached SDK"
    echo "  copy-cached-sdk           Copy cached SDK files"
    echo "  setup-package-tool        Configure PackageTool executable"
    echo "  setup-emsdk              Activate emsdk and export environment"
    echo "  setup-environment         Gather build information"
    echo "  stage-project-artifacts   Stage downstream build outputs for publishing"
    echo "  upload-release-archive    Archive a staged output directory and upload it"
    echo "  release-sdk              Archive and upload SDK directory contents"
    echo "  refresh-latest-tag        Move the latest tag to the current commit"
    echo ""
    echo "Options:"
    echo "  --build-type TYPE         Build type: dbg or rel (default: rel)"
    echo "  --sdk-layout BOOL         Install full SDK layout using ci_sdk_dir or --prefix"
    echo ""
    echo "Extra arguments after -- are passed to the action"
    exit 1
fi

ci_action=$1; shift;

# Parse all arguments into arg_* variables
# Long options (--name value) become arg_name=value
# Arguments after -- become arg_extra array
# Positional arguments become arg_positional array
declare -a arg_extra=()
declare -a arg_positional=()

parse_args() {
    while [[ $# -gt 0 ]]; do
        case "$1" in
            --)
                shift
                arg_extra=("$@")
                break
                ;;
            --*)
                # Extract option name (remove leading --)
                local opt_name="${1#--}"
                # Convert dashes to underscores for variable names
                local var_name="arg_${opt_name//-/_}"

                if [[ -n "$2" && "$2" != --* ]]; then
                    # Next argument is the value
                    declare -g "$var_name=$2"
                    shift 2
                else
                    echo "Error: $1 requires an argument"
                    exit 1
                fi
                ;;
            *)
                # Positional argument
                arg_positional+=("$1")
                shift
                ;;
        esac
    done
}

# Parse arguments
parse_args "$@"

# fix paths on windows by replacing \ with /.
normalize-path() {
    echo "$1" | tr "\\" "/" 2>/dev/null
}

write-github-output() {
    local key=$1
    local value=$2
    if [[ -n "${GITHUB_OUTPUT:-}" ]]; then
        printf '%s=%s\n' "$key" "$value" >> "$GITHUB_OUTPUT"
    fi
}

trim-whitespace() {
    local value=$1
    value="${value#"${value%%[![:space:]]*}"}"
    value="${value%"${value##*[![:space:]]}"}"
    printf '%s' "$value"
}

is-truthy() {
    case "$1" in
        1|true|TRUE|True|yes|YES|Yes|on|ON|On)
            return 0
            ;;
        *)
            return 1
            ;;
    esac
}

ci_source_dir=$(normalize-path "$ci_source_dir")
ci_build_dir=$(normalize-path "$ci_build_dir")
ci_sdk_dir=$(normalize-path "$ci_sdk_dir")
ci_source_dir=${ci_source_dir%/};   # remove trailing slash if any

echo "ci_action=$ci_action"
echo "ci_platform=$ci_platform"
echo "ci_arch=$ci_arch"
echo "ci_compiler=$ci_compiler"
echo "ci_lib_type=$ci_lib_type"
echo "ci_workspace_dir=$ci_workspace_dir"
echo "ci_source_dir=$ci_source_dir"
echo "ci_build_dir=$ci_build_dir"
echo "ci_sdk_dir=$ci_sdk_dir"

declare -A types=(
    [dbg]='Debug'
    [rel]='RelWithDebInfo'
)

# Web builds cannot handle RelWithDebInfo configuration.
if [[ "$ci_platform" == "web" ]];
then
    types[rel]='Release';
fi

# Determine number of processors
case "$ci_platform" in
    linux|android|web)
        ci_number_of_processors=$(nproc)
        ;;
    macos|ios)
        ci_number_of_processors=$(sysctl -n hw.ncpu)
        ;;
    windows|uwp)
        ci_number_of_processors=${NUMBER_OF_PROCESSORS:-1}
        ;;
esac

# Map simplified arch names to Android ABI names (used across multiple functions)
declare -A android_arch_map=(
    [arm]='armeabi-v7a'
    [arm64]='arm64-v8a'
    [x86]='x86'
    [x64]='x86_64'
)

get-sdk-share-suffix() {
    if [[ "$ci_platform" == "windows" || "$ci_platform" == "uwp" ]]; then
        echo '/share'
    fi
}

run-gradle-task() {
    local project_dir=$1
    shift

    cd "$project_dir"
    if [[ ! -f ./gradlew ]]; then
        gradle wrapper
    fi
    chmod +x ./gradlew
    ./gradlew "$@"
}

build-swig-executable() {
    local binary_dir="${ci_workspace_dir}/swig-build"
    local exe=''
    local swig_exe=''

    cmake -S "${ci_source_dir}/Source/ThirdParty/swig" -B "$binary_dir" \
        -DCMAKE_RUNTIME_OUTPUT_DIRECTORY="$binary_dir" \
        -DCMAKE_BUILD_TYPE=Release -DDESKTOP=ON -DURHO3D_CSHARP=ON
    cmake --build "$binary_dir" --config Release --parallel "$ci_number_of_processors"

    if [[ "$ci_platform" == 'uwp' ]]; then
        exe='.exe'
    fi

    if [[ -f "$binary_dir/swig$exe" ]]; then
        swig_exe="$binary_dir/swig$exe"
    else
        swig_exe="$binary_dir/Release/swig$exe"
    fi

    export SWIG_EXECUTABLE="$swig_exe"
    if [[ -n "${GITHUB_ENV:-}" ]]; then
        printf 'SWIG_EXECUTABLE=%s\n' "$swig_exe" >> "$GITHUB_ENV"
    fi
}

install-build-artifacts() {
    local build_type=$1
    shift
    local extra_args=("$@")
    local has_component=0

    if [[ "$ci_platform" == "android" ]]; then
        local android_abi="${android_arch_map[$ci_arch]:-$ci_arch}"
        local install_dirs=()
        shopt -s nullglob
        install_dirs=("$ci_source_dir/android/.cxx/${types[$build_type]}"/*/"$android_abi")
        shopt -u nullglob
        if [[ ${#install_dirs[@]} -ne 1 ]]; then
            echo "Error: expected exactly one Android install directory for ${types[$build_type]}, found ${#install_dirs[@]}"
            return 1
        fi
        cmake --install "${install_dirs[0]}" --config "${types[$build_type]}" "${extra_args[@]}"
    else
        cmake --install "$ci_build_dir" --config "${types[$build_type]}" "${extra_args[@]}"
    fi

    if [[ "$ci_platform" == "windows" ]]; then
        copy-runtime-libraries-for-executables "$ci_sdk_dir/bin"
    fi

    for arg in "${extra_args[@]}"; do
        if [[ "$arg" == '--component' ]]; then
            has_component=1
            break
        fi
    done

    if [[ "$ci_platform" == "web" && "$build_type" == "rel" && "$has_component" -eq 0 ]]; then
        mkdir -p "$ci_sdk_dir/deploy/"
        cp -r \
            "$ci_sdk_dir/bin/${types[$build_type]}/Resources.js" \
            "$ci_sdk_dir/bin/${types[$build_type]}/Resources.js.data" \
            "$ci_sdk_dir/bin/${types[$build_type]}/Samples.js" \
            "$ci_sdk_dir/bin/${types[$build_type]}/Samples.wasm" \
            "$ci_sdk_dir/bin/${types[$build_type]}/Samples.html" \
            "$ci_sdk_dir/deploy/"
    fi
}

setup-package-tool-executable() {
    local strategy="${1:-auto}"
    local package_tool_executable=''
    local venv=''
    local python_exe=''

    case "$strategy" in
        auto|python)
            ;;
        *)
            echo "Error: unsupported PackageTool strategy '$strategy'"
            return 1
            ;;
    esac

    venv="${ci_workspace_dir}/venv-PackageTool"
    python3 -m venv "$venv"
    python_exe=$([[ -f "$venv/bin/python" ]] && echo "$venv/bin/python" || echo "$venv/Scripts/python.exe")
    "$python_exe" -m pip install lz4
    package_tool_executable="$python_exe;${ci_source_dir}/Source/Tools/PackageTool/PackageTool.py"
    echo "Using Python PackageTool fallback: ${package_tool_executable}"
    export PACKAGE_TOOL_EXECUTABLE="$package_tool_executable"
    if [[ -n "${GITHUB_ENV:-}" ]]; then
        printf 'PACKAGE_TOOL_EXECUTABLE=%s\n' "$package_tool_executable" >> "$GITHUB_ENV"
    fi
}

copy-runtime-libraries-for-executables() {
    local dir=$1
    local executable_files=($(find "$dir" -type f -executable))
    for file in "${executable_files[@]}"; do
        echo "Copying dependencies for $file"
        copy-runtime-libraries-for-file "$file"
    done
}

copy-runtime-libraries-for-file() {
    local file=$1
    local dependencies=($(ldd "$file" | awk '{print $3}'))
    local dir=$(dirname "$file")
    local filename=$(basename "$file")
    shopt -s nocasematch
    for dep in "${dependencies[@]}"; do
        if [[ "$dep" =~ (vcruntime.+dll)|(msvcp.+dll)|(D3DCOMPILER.*dll) ]];
        then
            local depName=$(basename "$dep")
            if [ "$dep" != "$dir/$depName" ]  && [[ ! -f "$dir/$depName" ]];
            then
                echo "Depends on $dep, making a copy to $dir"
                cp "$dep" "$dir"
            fi
        fi
    done
}

prepare-project-search-paths() {
    local mode=$1
    local cached_sdk=''

    project_cmake_prefix_variable='CMAKE_PREFIX_PATH'
    project_cmake_root_value=''
    project_urho3d_dir=''
    project_sdk_suffix=$(get-sdk-share-suffix)

    cached_sdk="${ci_workspace_dir}/cached-sdk${project_sdk_suffix}"

    if [[ "$mode" == "sdk" ]];
    then
        project_cmake_prefix_value="${ci_sdk_dir}${project_sdk_suffix}"
        if [[ -n "$project_sdk_suffix" ]]; then
            project_urho3d_dir="${ci_sdk_dir}${project_sdk_suffix}/Urho3D/CMake"
        else
            project_urho3d_dir="${ci_sdk_dir}/share/Urho3D/CMake"
        fi
        if [[ "$ci_platform" == "web" ]];
        then
            project_cmake_root_value="$ci_sdk_dir"
        fi
    else
        project_urho3d_dir="${ci_source_dir}/CMake/Urho3D/share/Urho3D"
        project_cmake_prefix_value="${ci_source_dir}/CMake"

        if [[ -d "$cached_sdk" ]];
        then
            if [[ "$ci_platform" == "web" ]];
            then
                project_cmake_root_value="$cached_sdk"
            else
                project_cmake_prefix_value="${project_cmake_prefix_value};${cached_sdk}"
            fi
        fi
    fi

}

prepare-project-cmake-args() {
    local source_dir=$1
    local build_dir=$2
    local mode=$3

    local shared=$([[ "$ci_lib_type" == 'dll' ]] && echo ON || echo OFF)
    prepare-project-search-paths "$mode"

    project_cmake_args=(
        -S "$source_dir"
        -B "$build_dir"
        -DBUILD_SHARED_LIBS=$shared
        -DCMAKE_CONFIGURATION_TYPES="${types[dbg]};${types[rel]}"
        "-D${project_cmake_prefix_variable}=${project_cmake_prefix_value}"
    )

    if [[ -n "$project_urho3d_dir" ]];
    then
        project_cmake_args+=("-DUrho3D_DIR=${project_urho3d_dir}")
    fi

    if [[ "$ci_platform" == "web" || "$ci_platform" == "ios" ]];
    then
        project_cmake_args+=('-DURHO3D_PACKAGING=ON')
    fi

    if [[ "$ci_platform" == "web" && -n "$project_cmake_root_value" ]];
    then
        project_cmake_args+=("-DCMAKE_FIND_ROOT_PATH=${project_cmake_root_value}")
    fi

    case "$ci_platform" in
        windows|uwp)
            local arch=$([[ "$ci_arch" == "x86" ]] && echo Win32 || echo x64)
            project_cmake_args+=(-G 'Visual Studio 18 2026' -A "$arch")
            if [[ "$ci_platform" == "uwp" ]];
            then
                project_cmake_args+=('-DCMAKE_SYSTEM_NAME=WindowsStore' '-DCMAKE_SYSTEM_VERSION=10.0')
            fi
            ;;
        linux)
            case "$ci_compiler" in
                gcc)
                    export CC=gcc
                    export CXX=g++
                    ;;
                clang)
                    export CC=clang
                    export CXX=clang++
                    ;;
                *)
                    export CC=$ci_compiler
                    export CXX=${ci_compiler/gcc/g++}
                    ;;
            esac
            project_cmake_args+=(-G 'Ninja Multi-Config')
            project_cmake_args+=('-DCMAKE_C_COMPILER_LAUNCHER=ccache' '-DCMAKE_CXX_COMPILER_LAUNCHER=ccache')
            ;;
        web)
            project_cmake_args+=(-G 'Ninja Multi-Config')
            project_cmake_args+=("-DCMAKE_TOOLCHAIN_FILE=$EMSDK/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake")
            project_cmake_args+=("-DEMSCRIPTEN_ROOT_PATH=$EMSDK/upstream/emscripten/")
            project_cmake_args+=('-DCMAKE_C_COMPILER_LAUNCHER=ccache' '-DCMAKE_CXX_COMPILER_LAUNCHER=ccache')
            ;;
        ios)
            project_cmake_args+=(-G Xcode)
            project_cmake_args+=("-DCMAKE_TOOLCHAIN_FILE=${ci_source_dir}/CMake/Toolchains/IOS.cmake")
            project_cmake_args+=('-DDEPLOYMENT_TARGET=12')
            project_cmake_args+=('-DCMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED=NO')
            case "$ci_arch" in
                arm)
                    project_cmake_args+=('-DPLATFORM=OS')
                    ;;
                arm64)
                    project_cmake_args+=('-DPLATFORM=OS64')
                    ;;
                x64)
                    project_cmake_args+=('-DPLATFORM=SIMULATOR64')
                    ;;
                *)
                    echo "Error: unsupported iOS architecture '$ci_arch'"
                    exit 1
                    ;;
            esac
            ;;
        macos)
            project_cmake_args+=(-G Xcode)
            case "$ci_arch" in
                x64)
                    project_cmake_args+=('-DCMAKE_OSX_ARCHITECTURES=x86_64')
                    ;;
                arm64)
                    project_cmake_args+=('-DCMAKE_OSX_ARCHITECTURES=arm64')
                    ;;
                *)
                    ;;
            esac
            ;;
        *)
            ;;
    esac

    if [[ ${#arg_extra[@]} -gt 0 ]];
    then
        project_cmake_args+=("${arg_extra[@]}")
    fi
}

build-project-configurations() {
    local build_dir=$1
    local build_types=(dbg rel)

    if [[ -n "${arg_build_type:-}" ]];
    then
        build_types=("$arg_build_type")
    fi

    for build_type in "${build_types[@]}";
    do
        if [[ -z "${types[$build_type]:-}" ]];
        then
            echo "Error: unsupported build type '$build_type'"
            exit 1
        fi

        echo "Building configuration ${types[$build_type]}"
        cmake --build "$build_dir" --config "${types[$build_type]}" --parallel "$ci_number_of_processors"
    done
}

function action-dependencies() {
    # Arguments: none

    if [[ "$ci_platform" == "linux" ]];
    then
        # Linux dependencies
        dev_packages=(
            libgl1-mesa-dev libxcursor-dev libxi-dev libxinerama-dev libxrandr-dev libxrender-dev libxss-dev
            libasound2-dev libpulse-dev libibus-1.0-dev libdbus-1-dev libreadline6-dev libudev-dev uuid-dev libtbb-dev
        )

        if [[ "$ci_arch" != "x64" ]];
        then
            dev_packages[${#dev_packages[@]}]="binutils-multiarch"
            dev_packages[${#dev_packages[@]}]="binutils-multiarch-dev"
            dev_packages[${#dev_packages[@]}]="build-essential"
        fi

        # Per-arch compilers
        case "$ci_arch" in
            arm)
                sudo dpkg --add-architecture armhf
                dev_packages=("${dev_packages[@]/%/:armhf}")
                dev_packages[${#dev_packages[@]}]="crossbuild-essential-armhf"
                ;;
            arm64)
                sudo dpkg --add-architecture arm64
                sudo apt-get update
                dev_packages=("${dev_packages[@]/%/:arm64}")
                dev_packages[${#dev_packages[@]}]="crossbuild-essential-arm64"
                ;;
            x86)
                sudo dpkg --add-architecture i386
                dev_packages=("${dev_packages[@]/%/:i386}")
                dev_packages[${#dev_packages[@]}]="crossbuild-essential-i386"
                ;;
        esac

        # Common dependencies
        sudo apt-get update
        sudo apt-get install -y ninja-build ccache xvfb p7zip-full "${dev_packages[@]}"
    elif [[ "$ci_platform" == "web" ]];
    then
        # Web dependencies. Host-side SDK tools require libOpenGL at runtime.
        sudo apt-get update
        sudo apt-get install -y --no-install-recommends uuid-dev ninja-build ccache p7zip-full libopengl0
    elif [[ "$ci_platform" == "android" ]];
    then
        # Android dependencies
        sudo apt-get update
        sudo apt-get install -y --no-install-recommends uuid-dev ninja-build ccache p7zip-full
    elif [[ "$ci_platform" == "macos" || "$ci_platform" == "ios" ]];
    then
        # iOS/MacOS dependencies
        brew install ccache bash
    else
        # Windows/UWP dependencies
        choco install -y ccache 7zip
        if [[ "$ci_platform" == "uwp" ]];
        then
            choco install -y windows-sdk-10-version-2104-all
        fi
    fi
}

function action-generate() {
    # Arguments: -- <extra_cmake_args...>

    if [[ "${ci_platform_group:-}" != "desktop" && -z "${PACKAGE_TOOL_EXECUTABLE:-}" ]]; then
        setup-package-tool-executable auto || exit 1
    fi

    # Handle CMAKE_PREFIX_PATH override
    # Determine CMAKE_FIND_ROOT_PATH vs CMAKE_PREFIX_PATH based on platform
    local cmake_prefix_path='CMAKE_PREFIX_PATH'
    local sdk_suffix=''
    if [[ "$ci_platform" == "web" ]]; then
        cmake_prefix_path='CMAKE_FIND_ROOT_PATH'
    fi

    # TODO: Ideally this should not be necessary.
    sdk_suffix=$(get-sdk-share-suffix)
    sdk_suffix=${sdk_suffix#/}

    # Generate using CMake preset if available
    ci_cmake_params=(
        --preset "${ci_platform}-${ci_compiler}-${ci_arch}-${ci_lib_type}"
        -B "$ci_build_dir" -S "$ci_source_dir"
        "-DCMAKE_INSTALL_PREFIX=$ci_sdk_dir"
        "-D$cmake_prefix_path=${ci_workspace_dir}/cached-sdk/${sdk_suffix}"
        "-DTRACY_TIMER_FALLBACK=ON"
        $([[ "$ci_compiler" != "msvc" ]] && echo "-DCMAKE_C_COMPILER_LAUNCHER=ccache")
        $([[ "$ci_compiler" != "msvc" ]] && echo "-DCMAKE_CXX_COMPILER_LAUNCHER=ccache")
        ${BUTLER_API_KEY:+"-DURHO3D_COPY_DATA_DIRS=ON"}
        "${arg_extra[@]}"
    )

    printf 'cmake'
    printf ' %q' "${ci_cmake_params[@]}"
    printf '\n'
    cmake "${ci_cmake_params[@]}"
}

function action-generate-detect-thirdparty() {
    # Arguments: --auto-pch <bool> (optional), --cmake-args-csv <args> (optional)
    local generate_args=()
    local generate_log="${arg_log_file:-generate.log}"
    local raw_csv="${arg_cmake_args_csv:-${INPUT_RBFX_SOURCE_SDK_CMAKE_ARGS:-}}"

    if is-truthy "${arg_auto_pch:-false}"; then
        local pch=$([[ "$ci_platform" == 'linux' && "$ci_compiler" == 'clang' ]] && echo 'OFF' || echo 'ON')
        generate_args+=("-DURHO3D_PCH=$pch")
    fi

    if [[ -n "$raw_csv" ]]; then
        local old_ifs="$IFS"
        local csv_args=()
        IFS=',' read -r -a csv_args <<< "$raw_csv"
        IFS="$old_ifs"
        local csv_arg=''
        for csv_arg in "${csv_args[@]}"; do
            csv_arg=$(trim-whitespace "$csv_arg")
            if [[ -n "$csv_arg" ]]; then
                generate_args+=("$csv_arg")
            fi
        done
    fi

    if ! (set -o pipefail; "$0" generate -- "${generate_args[@]}" 2>&1 | tee "$generate_log"); then
        return 1
    fi

    if grep -q "Could NOT find Urho3DThirdParty" "$generate_log"; then
        echo "ThirdParty will be built from source"
        write-github-output thirdparty_used false
    else
        echo "Using cached Urho3DThirdParty"
        write-github-output thirdparty_used true
    fi
}

function action-build() {
    # Arguments: --build-type <dbg|rel>, -- <extra_args...>
    if [[ "$ci_compiler" == "msvc" ]];
    then
      # Custom compiler build paths used only on windows.
      ccache_path=$(realpath /c/ProgramData/chocolatey/lib/ccache/tools/ccache-*)
      cp $ccache_path/ccache.exe $ccache_path/cl.exe  # https://github.com/ccache/ccache/wiki/MS-Visual-Studio
      $ccache_path/ccache.exe -s
      cmake --build $ci_build_dir --config "${types[$arg_build_type]}" -- -r -maxcpucount:$ci_number_of_processors -p:TrackFileAccess=false -p:UseMultiToolTask=true -p:CLToolPath=$ccache_path \
        '-p:ObjectFileName=$(IntDir)%(FileName).obj' -p:DebugInformationFormat=OldStyle && \
      $ccache_path/ccache.exe -s
    elif [[ "$ci_platform" == "android" ]];
    then
      declare -A android_build_types=(
        [dbg]='buildCMakeDebug'
        [rel]='buildCMakeRelWithDebInfo'
      )
      android_abi="${android_arch_map[$ci_arch]:-$ci_arch}"
      # Custom platform build paths used only on android.
            if [[ -z "${PACKAGE_TOOL_EXECUTABLE:-}" ]]; then
                    setup-package-tool-executable auto || exit 1
            fi
      ccache -s
      run-gradle-task "$ci_source_dir/android" "${android_build_types[$arg_build_type]}[${android_abi}]" && \
      ccache -s
    else
      # Default build path using plain CMake.
      # ci_platform:     windows|linux|macos|android|ios|web
      ccache -s
      cmake --build $ci_build_dir --parallel $ci_number_of_processors --config "${types[$arg_build_type]}" && \
      ccache -s
    fi
}

function action-build-swig() {
    # Arguments: none
    build-swig-executable
}

function action-setup-package-tool() {
    # Arguments: none
    setup-package-tool-executable python
}

function action-apk() {
    # Arguments: --build-type <dbg|rel>

    declare -A android_apk_types=(
        [dbg]='assembleDebug'
        [rel]='assembleRelease'
    )

    run-gradle-task "$ci_source_dir/android" "${android_apk_types[$arg_build_type]}"
}

function action-install() {
    # Arguments: --build-type <dbg|rel>, --sdk-layout <bool>, --prefix <dir>, -- <extra_cmake_args...>
    if [[ -n "${arg_sdk_layout:-}" ]]; then
        case "${arg_sdk_layout,,}" in
            1|true|yes|on)
                local install_prefix="${arg_prefix:-$ci_sdk_dir}"
                install-build-artifacts dbg --component ThirdParty --prefix "$install_prefix"
                install-build-artifacts rel --component ThirdParty --prefix "$install_prefix"
                echo "$ci_hash_thirdparty" > "$install_prefix/thirdparty-id.txt"
                (cd "$install_prefix" && find . -type f | sed 's|^\./||' > thirdparty-files.txt)
                install-build-artifacts dbg --prefix "$install_prefix"
                install-build-artifacts rel --prefix "$install_prefix"
                return
                ;;
            0|false|no|off)
                ;;
            *)
                echo "Error: --sdk-layout must be one of true/false/yes/no/on/off/1/0"
                exit 1
                ;;
        esac
    fi

    if [[ -z "${arg_build_type:-}" ]]; then
        echo "Error: install requires --build-type unless --sdk-layout is enabled"
        exit 1
    fi

    install-build-artifacts "$arg_build_type" "${arg_extra[@]}"
}

function action-test() {
    # Arguments: --build-type <dbg|rel>
    cd $ci_build_dir
    ctest --output-on-failure -C "${types[$arg_build_type]}" -j $ci_number_of_processors
}

function action-cstest() {
    # Arguments: --build-type <dbg|rel>
    cd "$ci_build_dir/bin/${types[$arg_build_type]}"
    # We don't want to fail C# tests if build was without C# support.
    test_file="Urho3DNet.Tests.dll"
    if test -f "$test_file";
    then
        dotnet test $test_file
    fi
}

function action-publish-to-itch() {
    # Arguments: none
    if [[ -z "${BUTLER_API_KEY}" ]];
    then
        echo "No BUTLER_API_KEY detected. Can't publish to itch.io."
        return 0
    fi

    if [[ "$ci_platform" == "windows" ]];
    then
        copy-runtime-libraries-for-executables "$ci_build_dir/bin"
    fi

    butler push "$ci_build_dir/bin" "rebelfork/rebelfork:$ci_platform-$ci_arch-$ci_lib_type-$ci_compiler-$arg_build_type"
}

function action-release-mobile-artifacts() {
    # Arguments: none
    if [[ -z "${GH_TOKEN}" ]];
    then
        echo "No GH_TOKEN detected. Can't release artifacts."
        return 1
    fi

    # Determine file pattern based on platform
    local pattern
    case "$ci_platform" in
        android) pattern="*.apk" ;;
        ios)     pattern="*.app" ;;
        *)       echo "Warning: action-release-mobile-artifacts called for non-mobile platform: $ci_platform"
                 return 1        ;;
    esac

    # Find and release the artifact
    local artifact=$(find . -name "$pattern" $([[ "$ci_platform" == "ios" ]] && echo "-type d") | head -n 1)
    if [ -n "$artifact" ];
    then
        local archive_name="rebelfork-bin-${ci_platform_tag}-latest.7z"
        7z a -t7z -m0=lzma2 -mx=9 -mfb=64 -md=32m -ms=on "$archive_name" "$artifact"
        gh release upload latest "$archive_name" --repo "${GITHUB_REPOSITORY}" --clobber
        echo "Released $archive_name"
    else
        echo "Warning: No $pattern file found"
        return 1
    fi
}

function action-test-project() {
    # Arguments: <project_name> <mode>
    # project_name: empty-project or sample-project
    # mode: sdk or source

    if [ ${#arg_positional[@]} -ne 2 ]; then
        echo "Error: test-project requires exactly 2 arguments: <project_name> <mode>"
        exit 1
    fi

    local project_name="${arg_positional[0]}"
    local mode="${arg_positional[1]}"

    local source_dir="$ci_workspace_dir/$project_name"
    local build_dir="$ci_workspace_dir/${project_name}-${mode}-build"
    if [[ "$mode" != "sdk" && "$mode" != "source" ]]; then
        echo "Error: test-project mode must be either 'sdk' or 'source'"
        exit 1
    fi

    prepare-project-cmake-args "$source_dir" "$build_dir" "$mode"

    echo "Configuring $project_name with $mode mode..."
    printf 'cmake'
    printf ' %q' "${project_cmake_args[@]}"
    printf '\n'
    cmake "${project_cmake_args[@]}"

    # Only build for SDK mode
    if [[ "$mode" == "sdk" ]];
    then
        echo "Building $project_name..."
        cmake --build "$build_dir" --config "${types[dbg]}"
        cmake --build "$build_dir" --config "${types[rel]}"
    else
        echo "Skipping build for $project_name in source mode as it would take too long."
    fi
}

function action-build-project() {
    # Arguments: <project_dir> <mode>, --build-type <dbg|rel> (optional), -- <extra_cmake_args...>

    if [ ${#arg_positional[@]} -ne 2 ]; then
        echo "Error: build-project requires exactly 2 arguments: <project_dir> <mode>"
        exit 1
    fi

    if [[ "$ci_platform" == "android" ]]; then
        echo "Error: Android projects must be built with build-android-project"
        exit 1
    fi

    local source_dir="${arg_positional[0]}"
    local mode="${arg_positional[1]}"
    local build_dir="$ci_workspace_dir/project-build/$ci_platform_tag"

    source_dir=$(normalize-path "$source_dir")
    build_dir=$(normalize-path "$build_dir")

    if [[ ! -d "$source_dir" ]]; then
        echo "Error: project directory does not exist: $source_dir"
        exit 1
    fi

    if [[ "$mode" != "sdk" && "$mode" != "source" ]]; then
        echo "Error: build-project mode must be either 'sdk' or 'source'"
        exit 1
    fi

    prepare-project-cmake-args "$source_dir" "$build_dir" "$mode"

    echo "Configuring downstream project from $source_dir"
    printf 'cmake'
    printf ' %q' "${project_cmake_args[@]}"
    printf '\n'
    cmake "${project_cmake_args[@]}"

    build-project-configurations "$build_dir"
}

function action-build-android-project() {
    # Arguments: --gradle-task <task>, <android_dir> <mode>

    if [ ${#arg_positional[@]} -ne 2 ]; then
        echo "Error: build-android-project requires exactly 2 arguments: <android_dir> <mode>"
        exit 1
    fi

    if [[ "$ci_platform" != "android" ]]; then
        echo "Error: build-android-project can only be used for android platform"
        exit 1
    fi

    local android_dir="${arg_positional[0]}"
    local mode="${arg_positional[1]}"
    local gradle_task="${arg_gradle_task:-assembleRelease}"

    android_dir=$(normalize-path "$android_dir")

    if [[ ! -d "$android_dir" ]]; then
        echo "Error: android directory does not exist: $android_dir"
        exit 1
    fi

    if [[ "$mode" != "sdk" && "$mode" != "source" ]]; then
        echo "Error: build-android-project mode must be either 'sdk' or 'source'"
        exit 1
    fi

    prepare-project-search-paths "$mode"

    if [[ -z "${PACKAGE_TOOL_EXECUTABLE:-}" ]]; then
        setup-package-tool-executable auto || exit 1
    fi

    echo "Building downstream Android project from $android_dir"
    echo "Using ${project_cmake_prefix_variable}=${project_cmake_prefix_value}"
    export CMAKE_PREFIX_PATH="$project_cmake_prefix_value"
    if [[ -n "$project_urho3d_dir" ]]; then
        echo "Using Urho3D_DIR=${project_urho3d_dir}"
        export Urho3D_DIR="$project_urho3d_dir"
    else
        unset Urho3D_DIR
    fi
    if [[ -n "${PACKAGE_TOOL_EXECUTABLE:-}" ]]; then
        echo "Using PackageTool executable: ${PACKAGE_TOOL_EXECUTABLE}"
    fi
    run-gradle-task "$android_dir" "$gradle_task"
}

# Action to download and optionally verify a release from GitHub
# Usage: ci_build.sh download-release -- <url> <extract_dir> [<id_file_path> <expected_id>]
function action-download-release() {
    # Arguments: -- <url> <extract_dir> [<id_file_path> <expected_id>]
    if [ ${#arg_extra[@]} -ne 2 ] && [ ${#arg_extra[@]} -ne 4 ];
    then
        echo "Error: download-release requires 2 or 4 arguments after --: <url> <extract_dir> [<id_file_path> <expected_id>]"
        return 1
    fi

    local url="${arg_extra[0]}"
    local extract_dir="${arg_extra[1]}"
    local id_file_path=''
    local expected_id=''
    local temp_id_dir="temp-id-$$"

    if [ ${#arg_extra[@]} -eq 4 ]; then
        id_file_path="${arg_extra[2]}"
        expected_id="${arg_extra[3]}"
    fi

    # Set up automatic cleanup on function exit
    trap 'rm -rf "$temp_id_dir" download.7z' RETURN

    echo "Attempting to download: $url"

    # Download the archive
    if command -v gh >/dev/null 2>&1 && [[ -n "${GH_TOKEN:-}" ]] && [[ "$url" =~ ^https://github\.com/([^/]+/[^/]+)/releases/download/([^/]+)/([^/]+)$ ]]; then
        local github_repository="${BASH_REMATCH[1]}"
        local release_tag="${BASH_REMATCH[2]}"
        local asset_name="${BASH_REMATCH[3]}"
        if ! gh release download "$release_tag" --repo "$github_repository" --pattern "$asset_name" --dir .;
        then
            echo "Failed to download from GitHub releases using gh"
            return 1
        fi
        mv "$asset_name" download.7z
    else
        if ! curl -fsSL "$url" -o download.7z;
        then
            echo "Failed to download from releases"
            return 1
        fi
    fi

    echo "Downloaded successfully"

    if [[ -n "$id_file_path" ]]; then
        # Extract id file to verify version
        mkdir -p "$temp_id_dir"
        if ! 7z e -y "download.7z" "$id_file_path" -o"$temp_id_dir" >/dev/null 2>&1;
        then
            echo "Could not extract ID file: $id_file_path"
            return 1
        fi

        local id_filename=$(basename "$id_file_path")
        local cached_id=$(cat "$temp_id_dir/$id_filename")
        echo "Cached ID: $cached_id"
        echo "Expected ID: $expected_id"

        if [[ "$cached_id" != "$expected_id" ]];
        then
            echo "ID mismatch! Download is outdated."
            return 1
        fi

        echo "ID matches, extracting..."
    else
        echo "Extracting release archive..."
    fi

    if ! 7z x -y download.7z >/dev/null;
    then
        echo "Failed to extract archive"
        return 1
    fi

    # Extract the archive filename from URL and remove .7z extension to get the top-level directory name
    local archive_name=$(basename "$url")
    local top_level_dir="${archive_name%.7z}"
    mkdir -p "$(dirname "$extract_dir")"
    rm -rf "$extract_dir"
    mv "$top_level_dir" "$extract_dir"

    echo "Extraction successful"
    return 0
}

function action-download-cached-sdk() {
    # Arguments: --repository <repo> (optional), --extract-dir <dir> (optional)
    local repository="${arg_repository:-${DOWNLOAD_SDK_REPOSITORY:-${GITHUB_REPOSITORY:-}}}"
    local extract_dir="${arg_extract_dir:-${ci_workspace_dir}/cached-sdk}"

    if [[ -z "$repository" ]]; then
        echo "Error: download-cached-sdk requires --repository or GITHUB_REPOSITORY"
        return 1
    fi

    if "$0" download-release -- \
        "https://github.com/${repository}/releases/download/latest/rebelfork-sdk-${ci_platform_tag}-latest.7z" \
        "$extract_dir" \
        "rebelfork-sdk-${ci_platform_tag}-latest/thirdparty-id.txt" \
        "$ci_hash_thirdparty"; then
        write-github-output sdk_cached true
    else
        write-github-output sdk_cached false
        return 1
    fi
}

# Action to download multiple SDKs for NuGet packaging
# Usage: ci_build.sh download-nuget-sdks -- <github_repository>
function action-download-nuget-sdks() {
    # Arguments: -- <github_repository>
    if [ ${#arg_extra[@]} -ne 2 ];
    then
        echo "Error: download-nuget-sdks requires 1 argument after --: <github_repository> <output_dir>"
        return 1
    fi

    local github_repository="${arg_extra[0]}"
    local output_dir="${arg_extra[1]}"

    # Define all SDK configurations that need to be downloaded for NuGet packaging
    local platforms=(
        "windows-msvc-x64-dll"
        "linux-gcc-x64-dll"
        "macos-clang-x64-dll"
        "macos-clang-arm64-dll"
        "uwp-msvc-x64-dll"
    )

    echo "Downloading SDKs from releases..."
    for platform in "${platforms[@]}"; do
        local sdk_name="rebelfork-sdk-${platform}-latest.7z"
        echo "Downloading $sdk_name..."
        if gh release download latest --repo "$github_repository" --pattern "$sdk_name" --dir .;
        then
            echo "Downloaded $sdk_name"
            7z x -y "$sdk_name" "${sdk_name%.7z}/bin/*" -o"$output_dir"
            rm "$sdk_name"
        else
            echo "Warning: Failed to download $sdk_name (may not exist yet)"
        fi
    done
}

# Action to copy cached SDK files from a cached directory
# Usage: ci_build.sh copy-cached-sdk <src_dir> <dst_dir>
# Expects thirdparty-files.txt to exist in src_dir with relative file paths
function action-sanitize-cached-sdk() {
    # Arguments: <sdk_dir>
    if [ ${#arg_positional[@]} -ne 1 ]; then
        echo "Error: sanitize-cached-sdk requires 1 argument: <sdk_dir>"
        exit 1
    fi

    local sdk_dir="${arg_positional[0]}"

    if [[ "$ci_platform" == "windows" || "$ci_platform" == "uwp" ]];
    then
        sdk_dir=$(cygpath "$sdk_dir")
    fi

    if [[ ! -d "$sdk_dir" ]]; then
        echo "Error: cached SDK directory does not exist: $sdk_dir"
        exit 1
    fi

    local removed=0
    local engine_package_paths=(
        "$sdk_dir/share/Urho3D"
        "$sdk_dir/share/Urho3DTools"
    )

    for path in "${engine_package_paths[@]}"; do
        if [[ -e "$path" ]]; then
            echo "Removing cached SDK engine package exports: $path"
            rm -rf "$path"
            removed=1
        fi
    done

    if [[ "$removed" -eq 0 ]]; then
        echo "No cached SDK engine package exports found to remove"
    fi
}

function action-copy-cached-sdk() {
    # Arguments: <src_dir> <dst_dir>
    if [ ${#arg_positional[@]} -ne 2 ]; then
        echo "Error: copy-cached-sdk requires 2 arguments: <src_dir> <dst_dir>"
        exit 1
    fi

    local src="${arg_positional[0]}"
    local dst="${arg_positional[1]}"

    # Convert paths with cygpath on Windows/UWP
    if [[ "$ci_platform" == "windows" || "$ci_platform" == "uwp" ]];
    then
        src=$(cygpath "$src")
        dst=$(cygpath "$dst")
    fi

    mkdir -p "$dst"

    # Copy only files listed in thirdparty-files.txt
    if [[ -f "$src/thirdparty-files.txt" ]];
    then
        echo "Copying files from cached SDK..."
        while IFS= read -r file; do
            if [[ -n "$file" ]];
            then
                mkdir -p "$dst/$(dirname "$file")"
                cp -p "$src/$file" "$dst/$file"
            fi
        done < "$src/thirdparty-files.txt"
        echo "Cached SDK files copied successfully"
    else
        echo "Error: thirdparty-files.txt not found in cached SDK"
        exit 1
    fi
}

function action-setup-emsdk() {
    # Arguments: <emsdk_dir> (optional)
    local emsdk_dir="${arg_positional[0]:-${ci_workspace_dir}/emsdk}"
    emsdk_dir=$(normalize-path "$emsdk_dir")

    if [[ ! -d "$emsdk_dir" ]]; then
        echo "Error: emsdk directory does not exist: $emsdk_dir"
        return 1
    fi

    cd "$emsdk_dir"
    ./emsdk install latest
    ./emsdk activate latest

    if [[ -n "${GITHUB_ENV:-}" ]]; then
        printf 'PATH=%s:%s:%s\n' "$PATH" "$emsdk_dir" "$emsdk_dir/upstream/emscripten" >> "$GITHUB_ENV"
        printf 'EMSDK=%s\n' "$emsdk_dir" >> "$GITHUB_ENV"
    else
        export PATH="$PATH:$emsdk_dir:$emsdk_dir/upstream/emscripten"
        export EMSDK="$emsdk_dir"
    fi
}

function action-stage-project-artifacts() {
    # Arguments: --staging-dir <dir> (optional), --build-dir <dir> (optional), --android-output-root <dir> (optional), --exclude-pdbs <bool> (optional)
    local staging_dir="${arg_staging_dir:-${ARTIFACT_STAGING_DIR:-}}"
    local build_dir="${arg_build_dir:-${project_build_dir:-}}"
    local android_output_root="${arg_android_output_root:-${ANDROID_OUTPUT_ROOT:-}}"
    local exclude_pdbs="${arg_exclude_pdbs:-${EXCLUDE_PDBS:-false}}"
    local found_assets=0

    if [[ -z "$staging_dir" || -z "$build_dir" ]]; then
        echo "Error: stage-project-artifacts requires a staging directory and build directory"
        return 1
    fi

    rm -rf "$staging_dir"
    mkdir -p "$staging_dir"

    if [[ -d "$build_dir/bin" ]]; then
        cp -a "$build_dir/bin" "$staging_dir/bin"
        found_assets=1
    fi

    if [[ -d "$build_dir/lib" ]]; then
        cp -a "$build_dir/lib" "$staging_dir/lib"
        found_assets=1
    fi

    if [[ -n "$android_output_root" ]]; then
        while IFS= read -r android_output; do
            local relative_output=${android_output#"$android_output_root/"}
            mkdir -p "$staging_dir/$(dirname "$relative_output")"
            cp -a "$android_output" "$staging_dir/$relative_output"
            found_assets=1
        done < <(find "$android_output_root" -type f \( -name '*.apk' -o -name '*.aab' \) 2>/dev/null)
    fi

    if is-truthy "$exclude_pdbs"; then
        find "$staging_dir" -type f -name '*.pdb' -delete
    fi

    if [[ "$found_assets" -ne 1 ]]; then
        echo 'No build outputs found for artifact staging.'
        return 1
    fi

    if ! find "$staging_dir" -type f -print -quit | grep -q .; then
        echo 'No build outputs remain after artifact staging.'
        return 1
    fi
}

function action-upload-release-archive() {
    # Arguments: --staging-dir <dir> (optional), --archive-name <name> (optional), --release-tag <tag> (optional), --repository <repo> (optional)
    local staging_dir="${arg_staging_dir:-${ARTIFACT_STAGING_DIR:-}}"
    local release_repository="${arg_repository:-${RELEASE_REPOSITORY:-${GITHUB_REPOSITORY:-}}}"
    local release_tag="${arg_release_tag:-${RELEASE_TAG:-latest}}"
    local archive_stem="${arg_archive_name:-${RELEASE_ASSET_NAME:-}}"
    local archive_file=''

    if [[ -z "$staging_dir" || -z "$archive_stem" || -z "$release_repository" ]]; then
        echo "Error: upload-release-archive requires staging dir, archive name, and repository"
        return 1
    fi

    archive_file="${archive_stem}.7z"

    if [[ ! -d "$staging_dir" ]]; then
        echo 'No build outputs found for release upload.'
        return 1
    fi

    if ! find "$staging_dir" -type f -print -quit | grep -q .; then
        echo 'No build outputs remain for release upload.'
        return 1
    fi

    rm -f "$archive_file"
    7z a -t7z -m0=lzma2 -mx=9 -mfb=64 -md=32m -ms=on "$archive_file" "$staging_dir"/* >/dev/null
    gh release upload "$release_tag" "$archive_file" --repo "$release_repository" --clobber
}

function action-release-sdk() {
    # Arguments: --sdk-dir <dir> (optional), --archive-name <name> (optional), --release-tag <tag> (optional), --repository <repo> (optional)
    local sdk_dir="${arg_sdk_dir:-${ci_sdk_dir:-SDK}}"
    local release_repository="${arg_repository:-${RELEASE_REPOSITORY:-${GITHUB_REPOSITORY:-}}}"
    local release_tag="${arg_release_tag:-${RELEASE_TAG:-latest}}"
    local archive_stem="${arg_archive_name:-${SDK_ARCHIVE_NAME:-rebelfork-sdk-${ci_platform_tag}-latest}}"
    local sdk_parent=''
    local archive_dir=''
    local archive_file="${archive_stem}.7z"

    if [[ -z "$sdk_dir" || -z "$release_repository" ]]; then
        echo "Error: release-sdk requires SDK directory and repository"
        return 1
    fi

    sdk_dir=$(normalize-path "$sdk_dir")
    sdk_parent=$(dirname "$sdk_dir")
    archive_dir="$sdk_parent/$archive_stem"

    if [[ "$sdk_dir" != "$archive_dir" ]]; then
        mv "$sdk_dir" "$archive_dir"
        7z a -t7z -m0=lzma2 -mx=9 -mfb=64 -md=32m -ms=on "$archive_file" "$archive_dir" >/dev/null
        gh release upload "$release_tag" "$archive_file" --repo "$release_repository" --clobber
        mv "$archive_dir" "$sdk_dir"
    else
        7z a -t7z -m0=lzma2 -mx=9 -mfb=64 -md=32m -ms=on "$archive_file" "$archive_dir" >/dev/null
        gh release upload "$release_tag" "$archive_file" --repo "$release_repository" --clobber
    fi
}

function action-refresh-latest-tag() {
    # Arguments: none
    git tag -f latest
    git push -f origin latest
}

# Action to set environment variables
# Usage: ci_build.sh setup-environment
function action-setup-environment() {
    # Arguments: none

    # Parse ci_platform_tag if provided
    # Format: platform-compiler-arch-libtype
    # Examples: windows-msvc-x64-dll, linux-gcc-x64-lib, android-clang-arm64-dll
    if [[ -n "${ci_platform_tag:-}" ]]; then
        IFS='-' read -ra TAG_PARTS <<< "$ci_platform_tag"
        ci_platform="${TAG_PARTS[0]}"
        ci_compiler="${TAG_PARTS[1]}"
        ci_arch="${TAG_PARTS[2]}"
        ci_lib_type="${TAG_PARTS[3]}"
    fi

    # Define env variables
    ci_short_sha=$(echo ${GITHUB_SHA} | cut -c1-8)
    ci_hash_thirdparty=$(cmake -DDIRECTORY_PATH="${ci_source_dir}/Source/ThirdParty" -DHASH_FORMAT=short -P "${ci_source_dir}/CMake/Modules/GetThirdPartyHash.cmake" 2>&1)
    ci_cache_id="${ccache_prefix}-$ci_platform_tag"
    case "$ci_platform" in
        windows|linux|macos)  ci_platform_group='desktop'  ;;
        android|ios)          ci_platform_group='mobile'   ;;
        uwp)                  ci_platform_group='uwp'      ;;
        web)                  ci_platform_group='web'      ;;
    esac

    # Using all available processors may lead to OOM on CI systems
    #ci_number_of_processors=$(( (ci_number_of_processors + 1) / 2 ))
    #if [ "$ci_number_of_processors" -lt 1 ];
    #then
    #    ci_number_of_processors=1
    #fi

    # Save to GitHub Actions environment if available
    if [ -n "$GITHUB_ENV" ];
    then
        echo "ci_number_of_processors=$ci_number_of_processors" >> $GITHUB_ENV
        echo "ci_short_sha=$ci_short_sha" >> $GITHUB_ENV
        echo "ci_cache_id=$ci_cache_id" >> $GITHUB_ENV
        echo "ci_hash_thirdparty=$ci_hash_thirdparty" >> $GITHUB_ENV
        echo "ci_platform_group=$ci_platform_group" >> $GITHUB_ENV
        echo "ci_platform=$ci_platform" >> $GITHUB_ENV
        echo "ci_compiler=$ci_compiler" >> $GITHUB_ENV
        echo "ci_arch=$ci_arch" >> $GITHUB_ENV
        echo "ci_lib_type=$ci_lib_type" >> $GITHUB_ENV
    fi
}

# Invoke requested action.
action-$ci_action
