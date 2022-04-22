CMAKE_VERSION="3.19.3"
VULKAN_SDK_VER="1.2.162.1"

if [ "$TRAVIS_OS_NAME" = "osx" ];  then
  wget --no-check-certificate https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-macos-universal.tar.gz &&
  tar -xzf cmake-${CMAKE_VERSION}-macos-universal.tar.gz
  export PATH=$PWD/cmake-${CMAKE_VERSION}-macos-universal/CMake.app/Contents/bin:$PATH
  cmake --version
  # Download Vulkan SDK
  export VK_SDK_DMG=vulkansdk-macos-$VULKAN_SDK_VER.dmg
  wget -O $VK_SDK_DMG https://sdk.lunarg.com/sdk/download/$VULKAN_SDK_VER/mac/$VK_SDK_DMG?Human=true &&
  hdiutil attach $VK_SDK_DMG
  export VULKAN_SDK=/Volumes/vulkansdk-macos-$VULKAN_SDK_VER
fi

if [ "$TRAVIS_OS_NAME" = "linux" ]; then
  # Link gcc-9 and g++-9 to their standard commands
  sudo ln -s /usr/bin/gcc-9 /usr/local/bin/gcc
  sudo ln -s /usr/bin/g++-9 /usr/local/bin/g++
  # Export CC and CXX to tell cmake which compiler to use
  export CC=/usr/bin/gcc-9
  export CXX=/usr/bin/g++-9
  # Check versions of gcc, g++ and cmake
  gcc -v
  g++ -v
  # Download a recent cmake
  mkdir $HOME/usr
  export PATH="$HOME/usr/bin:$PATH"
  wget https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-Linux-x86_64.sh &&
  chmod +x cmake-${CMAKE_VERSION}-Linux-x86_64.sh &&
  ./cmake-${CMAKE_VERSION}-Linux-x86_64.sh --prefix=$HOME/usr --exclude-subdir --skip-license
  cmake --version
  sudo apt-get update
  sudo apt-get install libx11-dev
  sudo apt-get install mesa-common-dev
  sudo apt-get install mesa-utils
  sudo apt-get install libgl-dev
fi

