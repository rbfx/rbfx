#!/usr/bin/env bash

# ci_build.sh <action> [options] [-- extra cmake args]
#
# Actions: dependencies|generate|build|install|test|cstest|apk|publish-to-itch|
#          release-mobile-artifacts|test-project|download-release|download-nuget-sdks|
#          copy-cached-sdk|setup-environment
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
    echo "  build                     Build the project"
    echo "  install                   Install build artifacts"
    echo "  test                      Run native tests"
    echo "  cstest                    Run C# tests"
    echo "  apk                       Build Android APK"
    echo "  publish-to-itch           Publish to itch.io"
    echo "  release-mobile-artifacts  Release mobile artifacts"
    echo "  test-project              Test external project"
    echo "  download-release          Download and verify release"
    echo "  download-nuget-sdks       Download SDKs for NuGet"
    echo "  copy-cached-sdk           Copy cached SDK files"
    echo "  setup-environment         Gather build information"
    echo ""
    echo "Options:"
    echo "  --build-type TYPE         Build type: dbg or rel (default: rel)"
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
ci_source_dir=$(echo $ci_source_dir | tr "\\" "/" 2>/dev/null)
ci_build_dir=$(echo $ci_build_dir   | tr "\\" "/" 2>/dev/null)
ci_sdk_dir=$(echo $ci_sdk_dir       | tr "\\" "/" 2>/dev/null)
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
    elif [[ "$ci_platform" == "web" || "$ci_platform" == "android" ]];
    then
        # Web / android dependencies
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

    # Handle CMAKE_PREFIX_PATH override
    # Determine CMAKE_FIND_ROOT_PATH vs CMAKE_PREFIX_PATH based on platform
    if [[ "$ci_platform" == "web" ]];
    then
        CMAKE_PREFIX_PATH='CMAKE_FIND_ROOT_PATH'
    else
        CMAKE_PREFIX_PATH='CMAKE_PREFIX_PATH'
    fi

    # TODO: Ideally this should not be necessary.
    sdk_suffix=''
    if [[ "$ci_platform" == "windows" || "$ci_platform" == "uwp" ]];
    then
        sdk_suffix='share'
    fi

    # Generate using CMake preset if available
    ci_cmake_params=(
        --preset "${ci_platform}-${ci_compiler}-${ci_arch}-${ci_lib_type}"
        -B "$ci_build_dir" -S "$ci_source_dir"
        "-DCMAKE_INSTALL_PREFIX=$ci_sdk_dir"
        "-D$CMAKE_PREFIX_PATH=${ci_workspace_dir}/cached-sdk/${sdk_suffix}"
        "-DTRACY_TIMER_FALLBACK=ON"
        $([[ "$ci_compiler" != "msvc" ]] && echo "-DCMAKE_C_COMPILER_LAUNCHER=ccache")
        $([[ "$ci_compiler" != "msvc" ]] && echo "-DCMAKE_CXX_COMPILER_LAUNCHER=ccache")
        ${BUTLER_API_KEY:+"-DURHO3D_COPY_DATA_DIRS=ON"}
        "${arg_extra[@]}"
    )

    echo "${ci_cmake_params[@]}"
    cmake "${ci_cmake_params[@]}"
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
      cd $ci_source_dir/android
      ccache -s
      gradle wrapper                                                       && \
      ./gradlew "${android_build_types[$arg_build_type]}[${android_abi}]"  && \
      ccache -s
    else
      # Default build path using plain CMake.
      # ci_platform:     windows|linux|macos|android|ios|web
      ccache -s
      cmake --build $ci_build_dir --parallel $ci_number_of_processors --config "${types[$arg_build_type]}" && \
      ccache -s
    fi
}

function action-apk() {
    # Arguments: --build-type <dbg|rel>

    declare -A android_apk_types=(
        [dbg]='assembleDebug'
        [rel]='assembleRelease'
    )

    cd $ci_source_dir/android
    gradle wrapper                                   && \
    ./gradlew "${android_apk_types[$arg_build_type]}"
}

function action-install() {
    # Arguments: --build-type <dbg|rel>, -- <extra_cmake_args...>
    if [[ "$ci_platform" == "android" ]];
    then
        android_abi="${android_arch_map[$ci_arch]:-$ci_arch}"
        cmake --install $ci_source_dir/android/.cxx/${types[$arg_build_type]}/*/$android_abi --config ${types[$arg_build_type]} ${arg_extra[@]}
    else
        cmake --install $ci_build_dir --config "${types[$arg_build_type]}" ${arg_extra[@]}
    fi

    # Copy .NET runtime libraries for executables on windows.
    if [[ "$ci_platform" == "windows" ]];
    then
        copy-runtime-libraries-for-executables "$ci_sdk_dir/bin"
    fi

    # Create deploy directory on Web (only when not installing specific components).
    if [[ "$ci_platform" == "web" && "$arg_build_type" == "rel" && ! " ${arg_extra[@]} " =~ " --component " ]];
    then
        mkdir -p $ci_sdk_dir/deploy/
        cp -r \
            $ci_sdk_dir/bin/${types[$arg_build_type]}/Resources.js        \
            $ci_sdk_dir/bin/${types[$arg_build_type]}/Resources.js.data   \
            $ci_sdk_dir/bin/${types[$arg_build_type]}/Samples.js          \
            $ci_sdk_dir/bin/${types[$arg_build_type]}/Samples.wasm        \
            $ci_sdk_dir/bin/${types[$arg_build_type]}/Samples.html        \
            $ci_sdk_dir/deploy/
    fi
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
    local shared=$([[ "$ci_lib_type" == 'dll' ]] && echo ON || echo OFF)

    # Build cmake args array
    local cmake_args=(-S "$source_dir" -B "$build_dir" -DBUILD_SHARED_LIBS=$shared -DCMAKE_CONFIGURATION_TYPES="${types[dbg]};${types[rel]}")
    local sdk_suffix=''
    local sdk_path=''

    # Platform-specific configuration
    CMAKE_PREFIX_PATH='CMAKE_PREFIX_PATH'
    if [[ "$ci_platform" == "windows" || "$ci_platform" == "uwp" ]];
    then
        local arch=$([[ "$ci_arch" == "x86" ]] && echo Win32 || echo x64)
        cmake_args+=(-G 'Visual Studio 17 2022' -A "$arch")
        sdk_suffix='/share'
        if [[ "$ci_platform" == "uwp" ]];
        then
            cmake_args+=('-DCMAKE_SYSTEM_NAME=WindowsStore' '-DCMAKE_SYSTEM_VERSION=10.0')
        fi
    elif [[ "$ci_platform" == "linux" ]];
    then
        export CC=${ci_compiler}
        export CXX=${ci_compiler/gcc/g++}
        cmake_args+=(-G 'Ninja Multi-Config')
    elif [[ "$ci_platform" == "web" ]];
    then
        cmake_args+=(-G 'Ninja Multi-Config')
        cmake_args+=("-DCMAKE_TOOLCHAIN_FILE=$EMSDK/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake")
        cmake_args+=("-DEMSCRIPTEN_ROOT_PATH=$EMSDK/upstream/emscripten/")
        CMAKE_PREFIX_PATH='CMAKE_FIND_ROOT_PATH'
    elif [[ "$ci_platform" == "ios" ]];
    then
        cmake_args+=('-DCMAKE_TOOLCHAIN_FILE=CMake/Toolchains/IOS.cmake')
        cmake_args+=('-DPLATFORM=SIMULATOR64')
        cmake_args+=('-DDEPLOYMENT_TARGET=12')
    elif [[ "$ci_platform" == "macos" ]];
    then
        case "$ci_arch" in
            x64)    cmake_args+=('-DCMAKE_OSX_ARCHITECTURES=x86_64') ;;
            arm64)  cmake_args+=('-DCMAKE_OSX_ARCHITECTURES=arm64')  ;;
            *)                                                       ;;
        esac
    fi

    # Determine CMAKE_PREFIX_PATH based on mode
    if [[ "$mode" == "sdk" ]];
    then
        sdk_path="${ci_sdk_dir}${sdk_suffix}"
    else
        sdk_path="$ci_source_dir/CMake"
    fi

    # Use CMAKE_FIND_ROOT_PATH for web, CMAKE_PREFIX_PATH for others
    cmake_args+=("-D$CMAKE_PREFIX_PATH=$sdk_path")

    echo "Configuring $project_name with $mode mode..."
    cmake "${cmake_args[@]}"

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

# Action to download and verify a release from GitHub
# Usage: ci_build.sh download-release -- <url> <extract_dir> <id_file_path> <expected_id>
function action-download-release() {
    # Arguments: -- <url> <extract_dir> <id_file_path> <expected_id>
    if [ ${#arg_extra[@]} -ne 4 ];
    then
        echo "Error: download-release requires 4 arguments after --: <url> <extract_dir> <id_file_path> <expected_id>"
        return 1
    fi

    local url="${arg_extra[0]}"
    local extract_dir="${arg_extra[1]}"
    local id_file_path="${arg_extra[2]}"
    local expected_id="${arg_extra[3]}"
    local temp_id_dir="temp-id-$$"

    # Set up automatic cleanup on function exit
    trap 'rm -rf "$temp_id_dir" download.7z' RETURN

    echo "Attempting to download: $url"

    # Download the archive
    if ! curl -fsSL "$url" -o download.7z;
    then
        echo "Failed to download from releases"
        return 1
    fi

    echo "Downloaded successfully"

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
    if ! 7z x -y download.7z >/dev/null;
    then
        echo "Failed to extract archive"
        return 1
    fi

    # Extract the archive filename from URL and remove .7z extension to get the top-level directory name
    local archive_name=$(basename "$url")
    local top_level_dir="${archive_name%.7z}"
    mv "$top_level_dir" "$extract_dir"

    echo "Extraction successful"
    return 0
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
            7z x -y "$sdk_name" -o"$output_dir"
            rm "$sdk_name"
        else
            echo "Warning: Failed to download $sdk_name (may not exist yet)"
        fi
    done
}

# Action to copy cached SDK files from a cached directory
# Usage: ci_build.sh copy-cached-sdk <src_dir> <dst_dir>
# Expects thirdparty-files.txt to exist in src_dir with relative file paths
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
