#!/usr/bin/env bash

if [ -e ./nuget.exe ];
then
    NUGET=./nuget.exe
else
    NUGET=nuget
fi

package() {
    mkdir -p ./in
    for platform in Linux MacOS Windows Android UWP;
    do
        if [ ! -e in/$platform ];
        then
            if [ "$GITHUB_ACTIONS" != "" ];
            then
                # CI itself downloads artifacts as unpacked dirs
                mv ./rbfx-$platform* ./in/$platform
            else
                # When testing use zips downloaded from CI
                unzip -n ./rbfx-$platform*.zip -d ./in/$platform
            fi
        fi
    done

    # Use a default version.
    version="0.0.99-dev"

    # Try getting a tag.
    git describe --abbrev=0 --tags 2>/dev/null
    if [ "$?" -ne "0" ];
    then
        # Use a branch name.
        version=$(git rev-parse --abbrev-ref HEAD)
        if [[ "$version" = *"nuget/"* ]];
        then
            version=$(sed 's|^nuget/v\?||g' <<< $version)
        fi
    fi

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

    ln -s ../../bin/CoreData CoreData
    ln -s ../../bin/EditorData EditorData
    ln -s ../../bin/Data Data

    for nuspec in rbfx.*.nuspec;
    do
        $NUGET pack -OutputDirectory ./out -Version $version $nuspec
    done

    rm -r CoreData EditorData Data
}

local-repository() {
    rm -rf ./local-repository
    for pkg in out/rbfx.*.nupkg;
    do
        $NUGET add $pkg -source ./local-repository
    done
}

case "$1" in
    package)
        package
    ;;
    local-repo)
        local-repository
    ;;
    *)
        echo "Usage: $0 package|local-repo"
        exit 1
    ;;
esac
