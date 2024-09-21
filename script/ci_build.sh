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

ci_action=$1; shift;
ci_cmake_params_user="$@"

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

declare -A android_types=(
    [dbg]='assembleDebug'
    [rel]='assembleRelease'
)

generators_windows_mingw=('-G' 'MinGW Makefiles')
generators_windows=('-G' 'Visual Studio 17 2022')
generators_uwp=('-G' 'Visual Studio 17 2022' '-DCMAKE_SYSTEM_NAME=WindowsStore' '-DCMAKE_SYSTEM_VERSION=10.0' '-DURHO3D_PACKAGING=ON')
generators_linux=('-G' 'Ninja')
generators_web=('-G' 'Ninja')
generators_macos=()
generators_ios=()

toolchains_ios=(
    '-DCMAKE_TOOLCHAIN_FILE=CMake/Toolchains/IOS.cmake'
    '-DPLATFORM=SIMULATOR64'
    '-DDEPLOYMENT_TARGET=11'
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
)
quirks_web_dbg=(
    '-DURHO3D_PLAYER=OFF'
)
quirks_dll=('-DURHO3D_CSHARP=ON')
quirks_windows_msvc_x64=('-A' 'x64')
quirks_windows_msvc_x86=('-A' 'Win32')
quirks_uwp_msvc_arm=('-A' 'ARM')
quirks_uwp_msvc_arm64=('-A' 'ARM64')
quirks_clang=('-DTRACY_NO_PARALLEL_ALGORITHMS=ON')                  # Includes macos and ios
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

# Find msbuild.exe
MSBUILD=msbuild
if [[ "$ci_platform" == "windows" || "$ci_platform" == "uwp" ]];
then
    MSBUILD=$(vswhere -products '*' -requires Microsoft.Component.MSBuild -property installationPath -latest)
    MSBUILD=$(echo $MSBUILD | tr "\\" "/" 2>/dev/null)    # Fix slashes
    MSBUILD=$(echo $MSBUILD | sed "s/://" 2>/dev/null)    # Remove :
    MSBUILD="/$MSBUILD/MSBuild/Current/Bin/MSBuild.exe"
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
        sudo apt-get install -y --no-install-recommends uuid-dev ninja-build ccache
    elif [[ "$ci_platform" == "macos" || "$ci_platform" == "ios" ]];
    then
        # iOS/MacOS dependencies
        brew install pkg-config ccache
    elif [[ "$ci_platform" == "windows" || "$ci_platform" == "uwp" ]];
    then
        # Windows dependencies
        choco install -y ccache
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
        "-DCMAKE_BUILD_TYPE=${types[$ci_build_type]}"
        "-DCMAKE_INSTALL_PREFIX=$ci_sdk_dir"
        "-DURHO3D_NETFX=net$DOTNET_VERSION"
    )

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

    ci_cmake_params+=(${ci_cmake_params_user[@]})
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
      cmake --build $ci_build_dir --config "${types[$ci_build_type]}" -- -r -maxcpucount:$NUMBER_OF_PROCESSORS -p:TrackFileAccess=false -p:UseMultiToolTask=true -p:CLToolPath=$ccache_path && \
      $ccache_path/ccache.exe -s
    elif [[ "$ci_platform" == "android" ]];
    then
      # Custom platform build paths used only on android.
      cd $ci_source_dir/android
      ccache -s
      gradle wrapper                                && \
      ./gradlew "${android_types[$ci_build_type]}"  && \
      ccache -s
    else
      # Default build path using plain CMake.
      # ci_platform:     windows|linux|macos|android|ios|web
      if [[ "$ci_platform" == "macos" || "$ci_platform" == "ios" ]];
      then
        NUMBER_OF_PROCESSORS=$(sysctl -n hw.ncpu)
      else
        NUMBER_OF_PROCESSORS=$(nproc)
      fi
      ccache -s
      cmake --build $ci_build_dir --parallel $NUMBER_OF_PROCESSORS --config "${types[$ci_build_type]}" && \
      ccache -s
    fi
}

function action-install() {
    cmake --install $ci_build_dir --config "${types[$ci_build_type]}"

    # Copy .NET runtime libraries for executables on windows.
    if [[ "$ci_platform" == "windows" ]]; then
        copy-runtime-libraries-for-executables "$ci_sdk_dir/bin"
    fi

    # Create deploy directory on Web.
    if [[ "$ci_platform" == "web" ]];
    then
        mkdir $ci_sdk_dir/deploy
        cp -r \
            $ci_sdk_dir/bin/Resources.js        \
            $ci_sdk_dir/bin/Resources.js.data   \
            $ci_sdk_dir/bin/Samples.js          \
            $ci_sdk_dir/bin/Samples.wasm        \
            $ci_sdk_dir/bin/Samples.html        \
            $ci_sdk_dir/deploy
    fi
}

function action-test() {
    cd $ci_build_dir
    ctest --output-on-failure -C "${types[$ci_build_type]}"
}

function action-cstest() {
    cd "$ci_build_dir/bin/${types[$ci_build_type]}"
    # We don't want to fail C# tests if build was without C# support.
    test_file = "Urho3DNet.Tests.dll"
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

# Invoke requested action.
action-$ci_action
