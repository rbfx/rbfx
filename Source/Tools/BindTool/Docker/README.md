Execute these from docker folder (the folder where tis README.txt is).

To clean unused images and free hard drive space.
```console
docker system prune -a 
```

Run this to extend log size (may be important if you run it on CI)

On linux:
```console
export BUILDKIT_STEP_LOG_MAX_SIZE=1073741824
```

On windows:
```console
SET BUILDKIT_STEP_LOG_MAX_SIZE=1073741824
```

To build the "generateswig" image run
```console
docker build --progress plain -t generateswig -f Dockerfile ./
```

To check that "generateswig" image is available run
```console
docker images
```

You can run docker image now and mount the repository as /rbfx folder in it.

On linux:
```console
docker run -it --mount type=bind,source=`git rev-parse --show-toplevel`,target=/rbfx --rm generateswig bash
```

On windows you can use powershell to get the git root and call mount command.
```shell
pwsh -c "$GitRoot = git rev-parse --show-toplevel; docker run -it --mount type=bind,source=$GitRoot,target=/rbfx --rm generateswig bash"
```

Now you are inside of the docker image.

Run this if you need to configure the rbfx. The output folder is set to a path inside docker container for perfomance.
```console
cmake -DURHO3D_TESTING=OFF -DURHO3D_PROFILING=OFF -DURHO3D_SAMPLES=OFF -DURHO3D_GLOW=ON -DURHO3D_RMLUI=ON -DURHO3D_CSHARP=ON -S /rbfx -B /cmake-build-clang
```

The following commands would generate the Urho3D SWIG files:
```console
cd /rbfx/Source/Urho3D
python3 ../Tools/BindTool/BindTool.py --clang /usr/bin/clang --output /rbfx/Source/Urho3D/CSharp/Swig/generated/Urho3D /cmake-build-clang/Source/Urho3D/GeneratorOptions_Urho3D_Debug.txt BindAll.cpp
```
**Don't blindly commit all the changes produced by the BindTool.py! Unfortunatelly the resulting files need some manual tweaks. Please check what was changed in the files already before commiting them.**
