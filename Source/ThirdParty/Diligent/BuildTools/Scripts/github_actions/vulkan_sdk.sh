VULKAN_SDK_VER="1.3.204.1"

export VK_SDK_DMG=vulkansdk-macos-$VULKAN_SDK_VER.dmg
wget -O $VK_SDK_DMG https://sdk.lunarg.com/sdk/download/$VULKAN_SDK_VER/mac/$VK_SDK_DMG?Human=true && hdiutil attach $VK_SDK_DMG
# https://vulkan.lunarg.com/doc/view/latest/mac/getting_started.html#user-content-install-and-uninstall-from-terminal

export VK_SDK_PATH=~/VulkanSDK
sudo /Volumes/vulkansdk-macos-$VULKAN_SDK_VER/InstallVulkan.app/Contents/MacOS/InstallVulkan --root $VK_SDK_PATH --accept-licenses --default-answer --confirm-command install
if [ ! -f "$VK_SDK_PATH/macOS/lib/libvulkan.dylib" ]; then
    echo "Ubale to find libvulkan.dylib in the SDK."
    exit 1
fi
if [ ! -d "$VK_SDK_PATH/MoltenVK" ]; then
    echo "Ubale to find MoltenVK folder in the SDK."
    exit 1
fi

echo "VULKAN_SDK=$VK_SDK_PATH" >> $GITHUB_ENV
