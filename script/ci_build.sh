#!/usr/bin/env bash

# build.sh <action> ...
# ci_action:       dependencies|generate|build|install|test
# ci_platform:     windows|linux|macos|android|ios|web
# ci_arch:         x86|x64|arm|arm64
# ci_compiler:     msvc|gcc|gcc-*|clang|clang-*|mingw
# ci_build_type:   dbg|rel
# ci_lib_type:     lib|dll
# ci_workspace_dir: actions workspace directory
# ci_source_dir:   source code directory
# ci_build_dir:    cmake cache directory
# ci_sdk_dir:      sdk installation directory

# Parse arguments: action [build_type] [-- extra cmake args]
if [ $# -eq 0 ]; then
    echo "Usage: $0 action [build_type] [-- extra cmake args]"
    exit 1
fi

ci_action=$1; shift;
ci_build_type=""
extra_cmake_args=()

# Check if we have more arguments
if [ $# -gt 0 ]; then
    # Check if next argument is "--" (meaning no build_type)
    if [ "$1" = "--" ]; then
        shift
        extra_cmake_args=("$@")
    else
        # We have a build_type
        ci_build_type="$1"
        shift

        # Check for remaining arguments after build_type
        if [ $# -gt 0 ] && [ "$1" = "--" ]; then
            shift
            extra_cmake_args=("$@")
        fi
    fi
fi

# default values
ci_compiler=${ci_compiler:-"default"}
ci_build_type=${ci_build_type:-"rel"}
ci_lib_type=${ci_lib_type:-"dll"}

# fix paths on windows by replacing \ with /.
ci_source_dir=$(echo $ci_source_dir | tr "\\" "/" 2>/dev/null)
ci_build_dir=$(echo $ci_build_dir   | tr "\\" "/" 2>/dev/null)
ci_sdk_dir=$(echo $ci_sdk_dir       | tr "\\" "/" 2>/dev/null)
ci_source_dir=${ci_source_dir%/};   # remove trailing slash if any

echo "ci_action=$ci_action"
echo "ci_platform=$ci_platform"
echo "ci_arch=$ci_arch"
echo "ci_compiler=$ci_compiler"
echo "ci_build_type=$ci_build_type"
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
if [[ "$ci_platform" == "web" ]]; then types[rel]='Release'; fi

declare -A android_build_types=(
    [dbg]='buildCMakeDebug'
    [rel]='buildCMakeRelWithDebInfo'
)

declare -A android_apk_types=(
    [dbg]='assembleDebug'
    [rel]='assembleRelease'
)

generators_windows_mingw=('-G' 'Ninja Multi-Config')
generators_windows=('-G' 'Visual Studio 17 2022')
generators_uwp=('-G' 'Visual Studio 17 2022' '-DCMAKE_SYSTEM_NAME=WindowsStore' '-DCMAKE_SYSTEM_VERSION=10.0' '-DURHO3D_PACKAGING=ON')
generators_linux=('-G' 'Ninja Multi-Config')
generators_web=('-G' 'Ninja Multi-Config')
generators_macos=()
generators_ios=()

toolchains_ios=(
    '-DCMAKE_TOOLCHAIN_FILE=CMake/Toolchains/IOS.cmake'
    '-DPLATFORM=SIMULATOR64'
    '-DDEPLOYMENT_TARGET=12'
)
toolchains_web=(
    "-DCMAKE_TOOLCHAIN_FILE=$EMSDK/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake"
    "-DEMSCRIPTEN_ROOT_PATH=$EMSDK/upstream/emscripten/"
    '-DURHO3D_PROFILING=OFF'
)

lib_types_lib=('-DBUILD_SHARED_LIBS=OFF')
lib_types_dll=('-DBUILD_SHARED_LIBS=ON')

# !! ccache only supports GCC precompiled headers. https://ccache.dev/manual/latest.html#_precompiled_headers
quirks_mingw=(
    '-DURHO3D_PROFILING=OFF'
    '-DURHO3D_CSHARP=OFF'
    '-DURHO3D_TESTING=OFF'
    '-DURHO3D_PCH=OFF'
)
quirks_msvc=(
    '-DURHO3D_PCH=OFF'
)
quirks_ios=(
    '-DURHO3D_CSHARP=OFF'
)
quirks_android=(
    '-DURHO3D_CSHARP=OFF'
)
quirks_web=(
    '-DURHO3D_PROFILING=OFF'
    '-DURHO3D_CSHARP=OFF'
    '-DCI_WEB_BUILD=ON'
    '-DURHO3D_NO_EDITOR_PLAYER_EXE=ON'
)
quirks_dll=('-DURHO3D_CSHARP=ON')
quirks_windows_msvc_x64=('-A' 'x64')
quirks_windows_msvc_x86=('-A' 'Win32')
quirks_uwp_msvc_arm=('-A' 'ARM')
quirks_uwp_msvc_arm64=('-A' 'ARM64')
quirks_clang=('-DTRACY_TIMER_FALLBACK=ON')                  # Tracy, includes macos and ios
quirks_macos_x86=('-DCMAKE_OSX_ARCHITECTURES=i386')
quirks_macos_x64=('-DCMAKE_OSX_ARCHITECTURES=x86_64')
quirks_linux_x86=(
    '-DCMAKE_C_FLAGS=-m32'
    '-DCMAKE_CXX_FLAGS=-m32'
)
quirks_linux_x64=(
    '-DCMAKE_C_FLAGS=-m64'
    '-DCMAKE_CXX_FLAGS=-m64'
)
quirks_linux_clang_x64=(
    '-DURHO3D_PCH=OFF' # Keep PCH disabled somewhere to catch missing includes
)

if [[ "$ci_platform" == "macos" || "$ci_platform" == "ios" ]];
then
    NUMBER_OF_PROCESSORS=$(sysctl -n hw.ncpu)
else
    NUMBER_OF_PROCESSORS=$(nproc)
fi

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
        if [[ "$dep" =~ (vcruntime.+dll)|(msvcp.+dll)|(D3DCOMPILER.*dll) ]]; then
            local depName=$(basename "$dep")
            if [ "$dep" != "$dir/$depName" ]  && [[ ! -f "$dir/$depName" ]]; then
                echo "Depends on $dep, making a copy to $dir"
                cp "$dep" "$dir"
            fi
        fi
    done
}

function action-dependencies() {
    # Make tools executable.
    # TODO: This should not be necessary, but for some reason installed tools lose executable flag.
    if [[ -e $ci_workspace_dir/host-sdk ]];
    then
      find $ci_workspace_dir/host-sdk/bin/ -type f -exec chmod +x {} \;
    fi

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
        sudo apt-get install -y ninja-build ccache xvfb "${dev_packages[@]}"
    elif [[ "$ci_platform" == "web" || "$ci_platform" == "android" ]];
    then
        # Web / android dependencies
        sudo apt-get update
        sudo apt-get install -y --no-install-recommends uuid-dev ninja-build ccache
    elif [[ "$ci_platform" == "macos" || "$ci_platform" == "ios" ]];
    then
        # iOS/MacOS dependencies
        brew install pkg-config ccache bash
    else
        # Windows/UWP dependencies
        choco install -y ccache zip
        if [[ "$ci_platform" == "uwp" ]];
        then
            choco install -y windows-sdk-10-version-2104-all
        fi
    fi
}

function action-generate() {
    # Change a default compiler.
    case "$ci_platform-$ci_compiler" in
        linux-clang*)
            export CC=${ci_compiler}            # clang or clang-XX
            export CXX=${ci_compiler}++         # clang++ or clang-XX++
            ;;
        linux-gcc*)
            export CC=${ci_compiler}            # gcc or gcc-XX
            export CXX=${ci_compiler/gcc/g++}   # g++ or g++-XX
            ;;
    esac

    # Generate.
    ci_cmake_params=()
    v="generators_${ci_platform}_${ci_compiler}[@]";        ci_cmake_params+=("${!v}")
    if [[ -z "${!v}" ]];
    then
        v="generators_${ci_platform}[@]";                   ci_cmake_params+=("${!v}")
    fi
    v="toolchains_${ci_platform}[@]";                       ci_cmake_params+=("${!v}")
    v="lib_types_${ci_lib_type}[@]";                        ci_cmake_params+=("${!v}")
    v="quirks_${ci_compiler}[@]";                           ci_cmake_params+=("${!v}")
    v="quirks_${ci_lib_type}[@]";                           ci_cmake_params+=("${!v}")
    v="quirks_${ci_platform}_${ci_compiler}_${ci_arch}[@]"; ci_cmake_params+=("${!v}")
    v="quirks_${ci_platform}_${ci_arch}[@]";                ci_cmake_params+=("${!v}")
    v="quirks_${ci_platform}_${ci_build_type}[@]";          ci_cmake_params+=("${!v}")
    v="quirks_${ci_platform}[@]";                           ci_cmake_params+=("${!v}")

    ci_cmake_params+=(
        "-DCMAKE_INSTALL_PREFIX=$ci_sdk_dir"
        "-DURHO3D_NETFX=net$DOTNET_VERSION"
    )

    if [[ "$ci_platform" == "web" ]]; then
        CMAKE_PREFIX_PATH='CMAKE_FIND_ROOT_PATH'
    else
        CMAKE_PREFIX_PATH='CMAKE_PREFIX_PATH'
    fi

    ci_cmake_params+=("-D$CMAKE_PREFIX_PATH=${ci_workspace_dir}/cached-sdk;${ci_workspace_dir}/host-sdk")

    if [[ "$ci_compiler" != "msvc" ]];
    then
        ci_cmake_params+=(
            "-DCMAKE_C_COMPILER_LAUNCHER=ccache"
            "-DCMAKE_CXX_COMPILER_LAUNCHER=ccache"
        )
    fi

    if [[ -n "${BUTLER_API_KEY}" ]]; then
        echo "BUTLER_API_KEY detected. Enabling URHO3D_COPY_DATA_DIRS option."
        ci_cmake_params+=(
            "-DURHO3D_COPY_DATA_DIRS=ON"
        )
    fi

    # Precise configuration types control
    ci_cmake_params+=(
        "-DCMAKE_CONFIGURATION_TYPES=${types[dbg]};${types[rel]}"
    )

    # Add any extra CMake arguments passed after --
    ci_cmake_params+=("${extra_cmake_args[@]}")

    ci_cmake_params+=(-B $ci_build_dir -S "$ci_source_dir")

    echo "${ci_cmake_params[@]}"
    cmake "${ci_cmake_params[@]}"
}

function action-build() {
    if [[ "$ci_compiler" == "msvc" ]];
    then
      # Custom compiler build paths used only on windows.
      ccache_path=$(realpath /c/ProgramData/chocolatey/lib/ccache/tools/ccache-*)
      cp $ccache_path/ccache.exe $ccache_path/cl.exe  # https://github.com/ccache/ccache/wiki/MS-Visual-Studio
      $ccache_path/ccache.exe -s
      cmake --build $ci_build_dir --config "${types[$ci_build_type]}" -- -r -maxcpucount:$NUMBER_OF_PROCESSORS -p:TrackFileAccess=false -p:UseMultiToolTask=true -p:CLToolPath=$ccache_path \
        '-p:ObjectFileName=$(IntDir)%(FileName).obj' -p:DebugInformationFormat=OldStyle && \
      $ccache_path/ccache.exe -s
    elif [[ "$ci_platform" == "android" ]];
    then
      # Custom platform build paths used only on android.
      cd $ci_source_dir/android
      ccache -s
      gradle wrapper                                                  && \
      ./gradlew "${android_build_types[$ci_build_type]}[${ci_arch}]"  && \
      ccache -s
    else
      # Default build path using plain CMake.
      # ci_platform:     windows|linux|macos|android|ios|web
      ccache -s
      cmake --build $ci_build_dir --parallel $NUMBER_OF_PROCESSORS --config "${types[$ci_build_type]}" && \
      ccache -s
    fi
}

function action-apk() {
    cd $ci_source_dir/android
    gradle wrapper                                   && \
    ./gradlew "${android_apk_types[$ci_build_type]}"
}

function action-install() {
    if [[ "$ci_platform" == "android" ]]; then
        cmake --install $ci_source_dir/android/.cxx/${types[$ci_build_type]}/*/$ci_arch --config ${types[$ci_build_type]} ${extra_cmake_args[@]}
    else
        cmake --install $ci_build_dir --config "${types[$ci_build_type]}" ${extra_cmake_args[@]}
    fi

    # Copy .NET runtime libraries for executables on windows.
    if [[ "$ci_platform" == "windows" ]]; then
        copy-runtime-libraries-for-executables "$ci_sdk_dir/bin"
    fi

    # Create deploy directory on Web (only when not installing specific components).
    if [[ "$ci_platform" == "web" && "$ci_build_type" == "rel" && ! " ${extra_cmake_args[@]} " =~ " --component " ]];
    then
        mkdir -p $ci_sdk_dir/deploy/
        cp -r \
            $ci_sdk_dir/bin/${types[$ci_build_type]}/Resources.js        \
            $ci_sdk_dir/bin/${types[$ci_build_type]}/Resources.js.data   \
            $ci_sdk_dir/bin/${types[$ci_build_type]}/Samples.js          \
            $ci_sdk_dir/bin/${types[$ci_build_type]}/Samples.wasm        \
            $ci_sdk_dir/bin/${types[$ci_build_type]}/Samples.html        \
            $ci_sdk_dir/deploy/
    fi
}

function action-test() {
    cd $ci_build_dir
    ctest --output-on-failure -C "${types[$ci_build_type]}" -j $NUMBER_OF_PROCESSORS
}

function action-cstest() {
    cd "$ci_build_dir/bin/${types[$ci_build_type]}"
    # We don't want to fail C# tests if build was without C# support.
    test_file="Urho3DNet.Tests.dll"
    if test -f "$test_file"; then
        dotnet test $test_file
    fi
}

function action-publish-to-itch() {
    if [[ -z "${BUTLER_API_KEY}" ]]; then
        echo "No BUTLER_API_KEY detected. Can't publish to itch.io."
        return 0
    fi

    if [[ "$ci_platform" == "windows" ]]; then
        copy-runtime-libraries-for-executables "$ci_build_dir/bin"
    fi

    butler push "$ci_build_dir/bin" "rebelfork/rebelfork:$ci_platform-$ci_arch-$ci_lib_type-$ci_compiler-$ci_build_type"
}

function action-release-mobile-artifacts() {
    if [[ -z "${GH_TOKEN}" ]]; then
        echo "No GH_TOKEN detected. Can't release artifacts."
        return 1
    fi

    # Determine file pattern and zip options based on platform
    local pattern zip_opts
    case "$ci_platform" in
        android)
            pattern="*.apk"
            zip_opts="-q"
            ;;
        ios)
            pattern="*.app"
            zip_opts="-r -q"
            ;;
        *)
            echo "⚠ Warning: action-release-mobile-artifacts called for non-mobile platform: $ci_platform"
            return 1
            ;;
    esac

    # Find and release the artifact
    local artifact=$(find . -name "$pattern" $([[ "$ci_platform" == "ios" ]] && echo "-type d") | head -n 1)
    if [ -n "$artifact" ]; then
        local zip_name="rebelfork-bin-${BUILD_ID}-latest.zip"
        zip $zip_opts "$zip_name" "$artifact"
        gh release upload latest "$zip_name" --repo "${GITHUB_REPOSITORY}" --clobber
        echo "✓ Released $zip_name"
    else
        echo "⚠ Warning: No $pattern file found"
        return 1
    fi
}

function action-test-project() {
    # Usage: test-project <project_name> <mode>
    # project_name: empty-project or sample-project
    # mode: sdk or source
    local project_name=$1
    local mode=$2

    local source_dir="$ci_workspace_dir/$project_name"
    local build_dir="$ci_workspace_dir/${project_name}-${mode}-build"
    local shared=$([[ "$ci_lib_type" == 'dll' ]] && echo ON || echo OFF)

    # Build cmake args array
    local cmake_args=(-S "$source_dir" -B "$build_dir" -DBUILD_SHARED_LIBS=$shared -DCMAKE_CONFIGURATION_TYPES="Debug;RelWithDebInfo")
    local sdk_suffix=''
    local sdk_path=''

    if [[ "$ci_platform" == "windows" ]];
    then
        sdk_suffix='/share'
    fi

    # Determine CMAKE_PREFIX_PATH based on mode
    if [[ "$mode" == "sdk" ]];
    then
        sdk_path="${ci_sdk_dir}${sdk_suffix}"
    else
        sdk_path="$ci_source_dir/CMake"
    fi

    # Platform-specific configuration
    if [[ "$ci_platform" == "windows" ]];
    then
        local arch=$([[ "$ci_arch" == "x86" ]] && echo Win32 || echo x64)
        cmake_args+=(-A "$arch")
    elif [[ "$ci_platform" == "linux" ]];
    then
        export CC=${ci_compiler}
        export CXX=${ci_compiler/gcc/g++}
        cmake_args+=(-G 'Ninja Multi-Config')
    fi

    cmake_args+=("-DCMAKE_PREFIX_PATH=$sdk_path;${ci_workspace_dir}/host-sdk")

    echo "Configuring $project_name with $mode mode..."
    cmake "${cmake_args[@]}"

    # Only build for SDK mode
    if [[ "$mode" == "sdk" ]];
    then
        echo "Building $project_name..."
        cmake --build "$build_dir" --config Debug
        cmake --build "$build_dir" --config RelWithDebInfo
    else
        echo "Skipping build for $project_name in source mode as it would take too long."
    fi
}

# Function to download and verify release from GitHub
# Usage: download_github_release <url> <extract_dir> <id_file_path> <expected_id>
# Returns: 0 if successful, 1 if failed or ID mismatch
download_github_release() {
    local url="$1"
    local extract_dir="$2"
    local id_file_path="$3"
    local expected_id="$4"

    echo "Attempting to download: $url"

    # Download the zip
    if ! curl -fsSL "$url" -o download.zip; then
        echo "Failed to download from releases"
        return 1
    fi

    echo "Downloaded successfully"

    # Extract id file to verify version
    local temp_id_dir="temp-id-$$"
    if ! unzip -q -j download.zip "$id_file_path" -d "$temp_id_dir" 2>/dev/null; then
        echo "Could not extract ID file: $id_file_path"
        rm -f download.zip
        return 1
    fi

    local id_filename=$(basename "$id_file_path")
    local cached_id=$(cat "$temp_id_dir/$id_filename")
    echo "Cached ID: $cached_id"
    echo "Expected ID: $expected_id"

    if [[ "$cached_id" != "$expected_id" ]]; then
        echo "ID mismatch! Download is outdated."
        rm -rf "$temp_id_dir"
        rm -f download.zip
        return 1
    fi

    echo "ID matches, extracting..."
    if ! unzip -q download.zip; then
        echo "Failed to extract archive"
        rm -rf "$temp_id_dir"
        rm -f download.zip
        return 1
    fi

    # Extract the zip filename from URL and remove .zip extension to get the top-level directory name
    local zip_name=$(basename "$url")
    local top_level_dir="${zip_name%.zip}"
    mv "$top_level_dir" "$extract_dir"

    rm -rf "$temp_id_dir"
    rm -f download.zip
    echo "Extraction successful"
    return 0
}

# Action to download a release from GitHub
# Usage: ci_build.sh download-release -- <url> <extract_dir> <id_file_path> <expected_id>
function action-download-release() {
    if [ ${#extra_cmake_args[@]} -ne 4 ]; then
        echo "Error: download-release requires 4 arguments: url extract_dir id_file_path expected_id"
        return 1
    fi
    download_github_release "${extra_cmake_args[0]}" "${extra_cmake_args[1]}" "${extra_cmake_args[2]}" "${extra_cmake_args[3]}"
}

# Action to gather build information and set environment variables
# Usage: ci_build.sh gather-info [tools|build]
function action-gather-info() {
    # Define env variables
    SHORT_SHA=$(echo ${GITHUB_SHA} | cut -c1-8)
    HASH_THIRDPARTY=$(cmake -DDIRECTORY_PATH="${ci_source_dir}/Source/ThirdParty" -DHASH_FORMAT=short -P "${ci_source_dir}/CMake/Modules/GetThirdPartyHash.cmake" 2>&1)
    HASH_TOOLS=$(cmake -DDIRECTORY_PATH="${ci_source_dir}/Source/Tools" -DHASH_FORMAT=short -P "${ci_source_dir}/CMake/Modules/GetThirdPartyHash.cmake" 2>&1)
    BUILD_ID="${ci_platform}-${ci_compiler}-${ci_lib_type}-${ci_arch}"
    ARTIFACT_ID_TOOLS="$HASH_TOOLS-$HASH_THIRDPARTY"
    CACHE_ID="${ccache_prefix}-$BUILD_ID"
    case "$ci_platform" in
        windows|linux|macos)  PLATFORM_GROUP='desktop' ;;
        android|ios)          PLATFORM_GROUP='mobile' ;;
        uwp)                  PLATFORM_GROUP='uwp' ;;
        web)                  PLATFORM_GROUP='web' ;;
    esac

    # Determine number of processors
    case "$ci_platform" in
        linux|android|web)
            NUMBER_OF_PROCESSORS=$(nproc)
            TOOLS_RELEASE_NAME="rebelfork-tools-linux-gcc-lib-x64.zip"
            ;;
        macos|ios)
            NUMBER_OF_PROCESSORS=$(sysctl -n hw.ncpu)
            TOOLS_RELEASE_NAME="rebelfork-tools-macos-clang-lib-x64.zip"
            ;;
        windows|uwp)
            NUMBER_OF_PROCESSORS=${NUMBER_OF_PROCESSORS:-1} # Already set on windows
            TOOLS_RELEASE_NAME="rebelfork-tools-windows-msvc-lib-x64.zip"
            ;;
    esac

    NUMBER_OF_PROCESSORS=$(( (NUMBER_OF_PROCESSORS + 1) / 2 ))
    if [ "$NUMBER_OF_PROCESSORS" -lt 1 ]; then
        NUMBER_OF_PROCESSORS=1
    fi

    # Save to GitHub Actions environment if available
    if [ -n "$GITHUB_ENV" ]; then
        echo "SHORT_SHA=$SHORT_SHA" >> $GITHUB_ENV
        echo "BUILD_ID=$BUILD_ID" >> $GITHUB_ENV
        echo "PLATFORM_GROUP=$PLATFORM_GROUP" >> $GITHUB_ENV
        echo "CACHE_ID=$CACHE_ID" >> $GITHUB_ENV
        echo "HASH_THIRDPARTY=$HASH_THIRDPARTY" >> $GITHUB_ENV
        echo "HASH_TOOLS=$HASH_TOOLS" >> $GITHUB_ENV
        echo "ARTIFACT_ID_TOOLS=$ARTIFACT_ID_TOOLS" >> $GITHUB_ENV
        echo "ARTIFACT_ID_SDK=$ARTIFACT_ID_SDK" >> $GITHUB_ENV
        echo "TOOLS_RELEASE_NAME=$TOOLS_RELEASE_NAME" >> $GITHUB_ENV
        echo "NUMBER_OF_PROCESSORS=$NUMBER_OF_PROCESSORS" >> $GITHUB_ENV
    fi
}

# Invoke requested action.
action-$ci_action
