#!/usr/bin/env bash

set -e

if [ -e ./nuget.exe ];
then
    NUGET=./nuget.exe
else
    NUGET=nuget
fi

create() {
    local version=""
    local OPTIND opt

    # Parse options
    while getopts "v:h" opt; do
        case "$opt" in
            v)
                version="$OPTARG"
                ;;
            h)
                echo "Usage: $0 package [-v VERSION]"
                echo "  -v VERSION    Specify package version (default: auto-detect from git)"
                return 0
                ;;
            *)
                echo "Usage: $0 package [-v VERSION]" >&2
                return 1
                ;;
        esac
    done

    # Error if version is not provided
    if [ -z "$version" ];
    then
        echo "Error: Version not specified. Use -v option to specify version." >&2
        return 1
    fi

    # Append commit hash for dev versions
    case "$version" in
        *-alpha|*-dev)
            sha_short=$(git rev-parse --short HEAD)
            version="$version-$sha_short"
        ;;
    esac

    echo "Packaging $version"

    cat <<EOF > ./build/rbfx.Urho3DNet.props
<Project>
  <PropertyGroup>
    <Urho3DNetVersion>$version</Urho3DNetVersion>
  </PropertyGroup>
</Project>
EOF

    rm -rf CoreData EditorData Data
    ln -s ../../bin/CoreData CoreData
    ln -s ../../bin/EditorData EditorData
    ln -s ../../bin/Data Data

    $NUGET pack -OutputDirectory ./out -Version "$version" rbfx.CoreData.nuspec
    $NUGET pack -OutputDirectory ./out -Version "$version" rbfx.Data.nuspec
    $NUGET pack -OutputDirectory ./out -Version "$version" rbfx.EditorData.nuspec
    $NUGET pack -OutputDirectory ./out -Version "$version" rbfx.Tools.nuspec
    $NUGET pack -OutputDirectory ./out -Version "$version" rbfx.Tools.runtime.linux-x64.nuspec
    $NUGET pack -OutputDirectory ./out -Version "$version" rbfx.Tools.runtime.osx-arm64.nuspec
    $NUGET pack -OutputDirectory ./out -Version "$version" rbfx.Tools.runtime.win-x64.nuspec
    $NUGET pack -OutputDirectory ./out -Version "$version" rbfx.Urho3DNet.nuspec
    $NUGET pack -OutputDirectory ./out -Version "$version" rbfx.Urho3DNet.runtime.win-x64.nuspec
    $NUGET pack -OutputDirectory ./out -Version "$version" rbfx.Urho3DNet.runtime.linux-x64.nuspec
    $NUGET pack -OutputDirectory ./out -Version "$version" rbfx.Urho3DNet.runtime.osx-arm64.nuspec
    #$NUGET pack -OutputDirectory ./out -Version "$version" rbfx.Urho3DNet.runtime.android-arm64.nuspec
    #$NUGET pack -OutputDirectory ./out -Version "$version" rbfx.Urho3DNet.runtime.android-arm.nuspec
    #$NUGET pack -OutputDirectory ./out -Version "$version" rbfx.Urho3DNet.runtime.android-x64.nuspec
    #$NUGET pack -OutputDirectory ./out -Version "$version" rbfx.Urho3DNet.runtime.uap-x64.nuspec

    rm -r CoreData EditorData Data
}

local-repository() {
    local OPTIND opt

    # Parse options
    while getopts "h" opt; do
        case "$opt" in
            h)
                echo "Usage: $0 local-repo"
                echo "  Create a local NuGet repository from packages in out/"
                return 0
                ;;
            *)
                echo "Usage: $0 local-repo" >&2
                return 1
                ;;
        esac
    done

    rm -rf ./local-repository

    $NUGET add rbfx.CoreData.nuspec                         -source ./local-repository
    $NUGET add rbfx.Data.nuspec                             -source ./local-repository
    $NUGET add rbfx.EditorData.nuspec                       -source ./local-repository
    $NUGET add rbfx.Tools.nuspec                            -source ./local-repository
    $NUGET add rbfx.Tools.runtime.linux-x64.nuspec          -source ./local-repository
    $NUGET add rbfx.Tools.runtime.osx-arm64.nuspec          -source ./local-repository
    $NUGET add rbfx.Tools.runtime.win-x64.nuspec            -source ./local-repository
    $NUGET add rbfx.Urho3DNet.nuspec                        -source ./local-repository
    $NUGET add rbfx.Urho3DNet.runtime.win-x64.nuspec        -source ./local-repository
    $NUGET add rbfx.Urho3DNet.runtime.linux-x64.nuspec      -source ./local-repository
    $NUGET add rbfx.Urho3DNet.runtime.osx-arm64.nuspec      -source ./local-repository
    #$NUGET add rbfx.Urho3DNet.runtime.android-arm64.nuspec  -source ./local-repository
    #$NUGET add rbfx.Urho3DNet.runtime.android-arm.nuspec    -source ./local-repository
    #$NUGET add rbfx.Urho3DNet.runtime.android-x64.nuspec    -source ./local-repository
    #$NUGET add rbfx.Urho3DNet.runtime.uap-x64.nuspec        -source ./local-repository
}

# Main script logic
if [ $# -eq 0 ];
then
    echo "Usage: $0 <action> [options]" >&2
    echo "" >&2
    echo "Actions:" >&2
    echo "  create        Build NuGet packages" >&2
    echo "  local-repo    Create local NuGet repository" >&2
    echo "" >&2
    echo "Use '$0 <action> -h' for action-specific help" >&2
    exit 1
fi

action="$1"
shift

case "$action" in
    create)
        create "$@"
    ;;
    local-repo)
        local-repository "$@"
    ;;
    *)
        echo "Error: Unknown action '$action'" >&2
        echo "Usage: $0 package|local-repo [options]" >&2
        exit 1
    ;;
esac
